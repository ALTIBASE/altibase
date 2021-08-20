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
 * $Id$
 **********************************************************************/
#include <cm.h>
#include <sdDef.h>
#include <sdi.h>
#include <sdiLob.h>
#include <sdm.h>
#include <sdf.h>
#include <sda.h>
#include <qci.h>
#include <qcm.h>
#include <qcmProc.h>
#include <qcg.h>
#include <qmn.h>
#include <qdsd.h>
#include <sdl.h>
#include <sdlStatement.h>
#include <sdlStatementManager.h>
#include <sdSql.h>
#include <qdk.h>
#include <sdmShardPin.h>
#include <sduProperty.h>
#include <sduVersion.h>
#include <qmxShard.h>
#include <sdiZookeeper.h>
#include <sdo.h>    /* TASK-7219 Shard Transformer Refactoring */

#include <qcd.h>

extern iduFixedTableDesc gShardConnectionInfoTableDesc;
extern iduFixedTableDesc gShardMetaNodeInfoTableDesc;
extern iduFixedTableDesc gShardDataNodeInfoTableDesc;
extern iduFixedTableDesc gZookeeperInfoTableDesc;

static sdiAnalyzeInfo    gAnalyzeInfoForAllNodes;

/* BUG-45899 */
static SChar * gNonShardQueryReason[] = { SDI_NON_SHARD_QUERY_REASONS, NULL };

/* BUG-46039 SLCN(Shard Linker Change Number) -> SMN(Shard Meta Number) */
static ULong             gSMNForMetaNode = SDI_NULL_SMN;

/* BUG-46090 Meta Node SMN 전파 */
static ULong             gSMNForDataNode = SDI_NULL_SMN;

/* PROJ-2701 Sharding online data rebuild */
static UInt              gShardUserID = ID_UINT_MAX;

// SHARD_STATUS 0,1 DEFAULT : 0
static UInt              gShardStatus = 0; 

static sdiDatabaseInfo   gShardDBInfo;

static void shardCoordFixCtrlCallback( void * aSession, SInt * aCount, UInt aEnterOrExit )
{
    switch ( aEnterOrExit )
    {
        case SDI_SHARD_COORD_FIX_CTRL_EVENT_ENTER :
            if ( *aCount == 0 )
            {
                qci::mSessionCallback.mPauseShareTransFix( aSession );
            }
            ++(*aCount);
            break;

        case SDI_SHARD_COORD_FIX_CTRL_EVENT_EXIT :
            --(*aCount);
            if ( *aCount == 0 )
            {
                qci::mSessionCallback.mResumeShareTransFix( aSession );
            }
            break;

        default :
            IDE_DASSERT( 0 );
            break;
    }
}

static inline idBool isNameInDoubleQuotation( qcNamePosition    * aNamePosition )
{
    SInt      i = 0;
    SChar   * sNodeName = aNamePosition->stmtText + aNamePosition->offset;
    idBool    sIsDoubleQuoted = ID_FALSE;

    for ( i = 0; i < aNamePosition->size; i++ )
    {
        if ( sNodeName[i] == '"' )
        {
            sIsDoubleQuoted = ID_TRUE;
            break;
        }
    }

    return sIsDoubleQuoted;
}

static inline UInt removeDoulbeQuotationFromNodeName( qcShardNodes     * aNodeName,
                                                      SChar            * aBuffer,
                                                      SInt               aBufferLength )
{
    SChar           * sName = aNodeName->namePos.stmtText + aNodeName->namePos.offset;
    SInt              sNameLength = aNodeName->namePos.size;
    SInt              sNameOffset = 0;
    SInt              sBufferOffset = 0;

    while ( ( sNameOffset < sNameLength ) &&
            ( sBufferOffset < aBufferLength - 1 ) )
    {
        if ( sName[sNameOffset] != '"' )
        {
            aBuffer[sBufferOffset] = sName[sNameOffset];
            sBufferOffset++;
        }

        sNameOffset++;
    }

    aBuffer[sBufferOffset] = '\0';

    return sBufferOffset;
}

static inline idBool isMatchedNodeName( qcShardNodes     * aNodeName1,
                                        const SChar      * aNodeName2 )
{
    idBool            sIsSame = ID_FALSE;
    SChar             sNodeName1Buf[SDI_NODE_NAME_MAX_SIZE + 1];
    UInt              sNodeName1BufLength = 0;

    if ( isNameInDoubleQuotation( &(aNodeName1->namePos) ) == ID_FALSE )
    {
        sIsSame = QC_IS_STR_CASELESS_MATCHED( aNodeName1->namePos, aNodeName2 );
    }
    else
    {
        sNodeName1BufLength = removeDoulbeQuotationFromNodeName( aNodeName1,
                                                                sNodeName1Buf,
                                                                SDI_NODE_NAME_MAX_SIZE + 1 );

        sIsSame = ( idlOS::strMatch( sNodeName1Buf,
                                     sNodeName1BufLength,
                                     aNodeName2,
                                     idlOS::strlen( aNodeName2 ) ) == 0 )
                  ? ID_TRUE : ID_FALSE;
    }
    
    return sIsSame;
}
static inline idBool needShardStmtPartialRollback( qciStmtType aStmtKind,
                                                   idBool      aFromSDSE )
{
    idBool      sNeedShardStmtPartialRollback = ID_FALSE;

    if ( ( ( ( aStmtKind == QCI_STMT_INSERT ) ||
             ( aStmtKind == QCI_STMT_UPDATE ) ||
             ( aStmtKind == QCI_STMT_DELETE ) ) &&
           ( aFromSDSE == ID_FALSE ) )
         ||
         ( aStmtKind == QCI_STMT_SELECT_FOR_UPDATE ) )
    {
        sNeedShardStmtPartialRollback = ID_TRUE;
    }
    else
    {
        sNeedShardStmtPartialRollback = ID_FALSE;
    }

    return sNeedShardStmtPartialRollback;
}

static void processExecuteError( qcStatement       * aStatement,
                                 sdiClientInfo     * aClientInfo,
                                 sdiDataNode       * aDataNode,
                                 idBool              aFromSDSE )
{
    UInt                  i = 0;
    sdiConnectInfo      * sConnectInfo = aClientInfo->mConnectInfo;
    idBool                sIsAutoCommit;
    idBool                sNeedShardStmtPartialRollback;
   
    sIsAutoCommit = QCG_GET_SESSION_IS_AUTOCOMMIT( aStatement );
    sNeedShardStmtPartialRollback = needShardStmtPartialRollback( aStatement->myPlan->parseTree->stmtKind,
                                                                  aFromSDSE ) ;

    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, aDataNode++ )
    {
        switch( aDataNode->mState )
        {
            case SDI_NODE_STATE_EXECUTED:
                if ( sIsAutoCommit == ID_TRUE )
                {
                    sdl::appendResultInfoToErrorMessage( sConnectInfo,
                                                         aStatement->myPlan->parseTree->stmtKind );
                }
                else
                {
                    if ( sNeedShardStmtPartialRollback == ID_TRUE )
                    {
                        if ( sdl::shardStmtPartialRollback( sConnectInfo, 
                                                            &(sConnectInfo->mLinkFailure) ) 
                             != IDE_SUCCESS )
                        {
                            IDE_ERRLOG( IDE_SD_0 );

                            sdi::setTransactionBroken( sIsAutoCommit,
                                                       (void*)QCG_GET_DATABASE_LINK_SESSION( aStatement ),
                                                       QC_SMI_STMT(aStatement)->mTrans );
                        }
                    }
                }
                break;

            case SDI_NODE_STATE_EXECUTE_SELECTED:
                aDataNode->mState = SDI_NODE_STATE_EXECUTE_CANDIDATED;
                break;

            default:
                break;
        }
    }
}

IDE_RC sdi::initialize()
{
    sdmShardPinMgr::initialize();
    idlOS::memset(&gShardDBInfo, 0, ID_SIZEOF(sdiDatabaseInfo));
    gShardDBInfo.mDBName = NULL;            /* smiGetDBName()  after smiStartupMeta phase */
    gShardDBInfo.mTransTblSize = 0;         /* smiGetTransTblSize after smiStartupPreProcess phase */

    gShardDBInfo.mDBCharSet = NULL;         /* mtc::getDBCharSet(); */
    gShardDBInfo.mNationalCharSet = NULL;   /* mtc::getNationalCharSet(); */
    gShardDBInfo.mDBVersion = NULL;         /* smVersionString */
    gShardDBInfo.mMajorVersion = SHARD_MAJOR_VERSION;
    gShardDBInfo.mMinorVersion = SHARD_MINOR_VERSION;
    /* shard patch version should not be compare, for rolling patch */
    gShardDBInfo.mPatchVersion = SHARD_PATCH_VERSION;

    #ifdef ALTI_CFG_OS_LINUX
    IDE_TEST(sdiZookeeper::initializeStatic() != IDE_SUCCESS);
    #endif
    
    /* PROJ-2728 Sharding LOB */
    smiLob::setShardLobModule(
                        sdiLob::open,
                        sdiLob::read,
                        sdiLob::write,
                        sdiLob::erase,
                        sdiLob::trim,
                        sdiLob::prepare4Write,
                        sdiLob::finishWrite,
                        sdiLob::getLobInfo,
                        sdiLob::writeLog4CursorOpen,
                        sdiLob::close );
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

void sdi::setShardDBInfo()
{
    gShardDBInfo.mDBName = smiGetDBName();
    gShardDBInfo.mTransTblSize = smiGetTransTblSize();

    gShardDBInfo.mDBCharSet = mtc::getDBCharSet();
    gShardDBInfo.mNationalCharSet = mtc::getNationalCharSet();
    gShardDBInfo.mDBVersion = (SChar*)smVersionString;
}

IDE_RC sdi::validateCreateDB(SChar         * aDBName,
                             SChar         * aDBCharSet,
                             SChar         * aNationalCharSet)
{
    PDL_UNUSED_ARG(aDBName);
    PDL_UNUSED_ARG(aDBCharSet);
    PDL_UNUSED_ARG(aNationalCharSet);

    if ( SDU_SHARD_ENABLE == 1 )
    {
        /* R2HA
         * first zookeeper connect.
         * check sharded database arguments
         * database_name, database charset, national charset compare
         * if there are different, database creation fails
         * if there is no sharded database, this work is first add sharded database
         * */
        ideLog::log(IDE_SD_31,"[DEBUG] validateCreateDB call");
    }

    return IDE_SUCCESS;
}

IDE_RC sdi::validateDropDB(SChar    * /*aDBName*/)
{
    if ( SDU_SHARD_ENABLE == 1 )
    {
        /* first zookeeper connect.
         * check sharded database
         * if there is the database in the sharded database, then the database drop fails.
         * */
        ideLog::log(IDE_SD_31,"[DEBUG] validateDropDB call");
    }

    return IDE_SUCCESS;
}

IDE_RC sdi::loadMetaNodeInfo()
{
    sdiGlobalMetaInfo   sMetaNodeInfo = { SDI_NULL_SMN };
    UInt                sShardUserID = ID_UINT_MAX;

    if( gShardDBInfo.mDBName == NULL )
    {    
        setShardDBInfo();
    }

    IDE_TEST_CONT( isShardEnable() != ID_TRUE, NO_SHARD_ENABLE );
    IDE_TEST_CONT( checkMetaCreated() != ID_TRUE, NO_SHARD_ENABLE );

    /* ShardUserID를 세팅한다. */
    IDE_TEST_CONT( sdm::getShardUserID( &sShardUserID ) != IDE_SUCCESS, NO_SHARD_ENABLE );
    setShardUserID( sShardUserID );

    /* Global Shard Meta Info */
    IDE_TEST( sdm::getGlobalMetaInfo( &sMetaNodeInfo ) != IDE_SUCCESS );
    setSMNCacheForMetaNode( sMetaNodeInfo.mShardMetaNumber );

    /* PROJ-2701 Sharding online data rebuild
     * start-up시에 dataSMN은 초기화 되어있다.
     * 이를 dataSMN을 metaSMN과 동일하게 설정한다.
     *
     * BUGBUG data rebuild수행중 장애시에, 해당 node가 내려갔다 올라오면(failback)
     *        dataSMN이 하나 낮은 상태로 설정 될 것이다.
     *        다른 살아있던 노드들 중 가장 큰 dataSMN으로 설정되도록 해야한다.
     */
    setSMNForDataNode( sMetaNodeInfo.mShardMetaNumber );

    /* Local Shard Meta Info */
    sdmShardPinMgr::loadShardPinInfo();


    IDE_EXCEPTION_CONT( NO_SHARD_ENABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void sdi::finalize()
{
    (void)sdiZookeeper::finalizeStatic();
    sdmShardPinMgr::finalize();
}

UInt sdi::getShardUserID()
{
    return gShardUserID;
}

void sdi::setShardUserID( UInt aShardUserID )
{
    gShardUserID = aShardUserID;
}

ULong sdi::getSMNForMetaNode()
{
    return (ULong)idCore::acpAtomicGet64( &gSMNForMetaNode );
}

void sdi::setSMNCacheForMetaNode( ULong aNewSMN )
{
    (void)idCore::acpAtomicSet64( &gSMNForMetaNode, aNewSMN );
}

UInt sdi::getShardStatus()
{
    return gShardStatus;
}

void sdi::setShardStatus( UInt aShardStatus )
{
    gShardStatus = aShardStatus;
}


void sdi::loadShardPinInfo()
{
    sdmShardPinMgr::loadShardPinInfo();
}

IDE_RC sdi::getIncreasedSMNForMetaNode( smiTrans * aTrans,
                                        ULong    * aNewSMN )
{
    sdiGlobalMetaInfo   sMetaNodeInfo = { SDI_NULL_SMN };
    smiStatement        sSmiStmt;
    idBool              sIsStmtBegun  = ID_FALSE;

    IDE_TEST( sSmiStmt.begin( aTrans->getStatistics(),
                              aTrans->getStatement(),
                              ( SMI_STATEMENT_UNTOUCHABLE |
                                SMI_STATEMENT_MEMORY_CURSOR ) )
              != IDE_SUCCESS );
    sIsStmtBegun = ID_TRUE;

    /*
     * PROJ-2701 Online data rebuild
     *
     * // IDE_TEST( sdm::increaseShardMetaNumberCore( &sSmiStmt ) != IDE_SUCCESS );
     *
     * GLOBAL_META_INFO_의 shard_meta_number에 대한 increase시점은
     * Tx의 첫 shard meta 변경 시로 이동한다.
     */
    IDE_TEST( sdm::getGlobalMetaInfoCore( &sSmiStmt, &sMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsStmtBegun = ID_FALSE;

    *aNewSMN = sMetaNodeInfo.mShardMetaNumber;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsStmtBegun == ID_TRUE )
    {
        (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC sdi::reloadSMNForDataNode( smiStatement * aSmiStmt )
{
    sdiGlobalMetaInfo   sMetaNodeInfo = { SDI_NULL_SMN };

    IDE_TEST_CONT( isShardEnable() != ID_TRUE, NO_SHARD_ENABLE );
    IDE_TEST_CONT( checkMetaCreated() != ID_TRUE, NO_SHARD_ENABLE );

    if ( aSmiStmt != NULL )
    {
        IDE_TEST( sdm::getGlobalMetaInfoCore( aSmiStmt,
                                              &sMetaNodeInfo ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sdm::getGlobalMetaInfo( &sMetaNodeInfo ) != IDE_SUCCESS );
    }

    setSMNForDataNode( sMetaNodeInfo.mShardMetaNumber );


    IDE_EXCEPTION_CONT( NO_SHARD_ENABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

ULong sdi::getSMNForDataNode()
{
    return (ULong)idCore::acpAtomicGet64( &gSMNForDataNode );
}

void sdi::setSMNForDataNode( ULong aNewSMN )
{
    (void)idCore::acpAtomicSet64( &gSMNForDataNode, aNewSMN );
}

IDE_RC sdi::addExtMT_Module( void )
{
    if ( isShardEnable() == ID_TRUE )
    {
        IDE_TEST( mtc::addExtFuncModule( (mtfModule**)
                                         sdf::extendedFunctionModules )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( mtc::addExtFuncModule( (mtfModule**)
                                     sdf::extendedFunctionModules2 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::initSystemTables( void )
{
    if ( isShardEnable() == ID_TRUE )
    {
        // initialize fixed table
        IDU_FIXED_TABLE_DEFINE_RUNTIME( gShardMetaNodeInfoTableDesc );
        IDU_FIXED_TABLE_DEFINE_RUNTIME( gShardDataNodeInfoTableDesc );
        IDU_FIXED_TABLE_DEFINE_RUNTIME( gShardConnectionInfoTableDesc );
        IDU_FIXED_TABLE_DEFINE_RUNTIME( gZookeeperInfoTableDesc );

        // initialize gAnalyzeInfoForAllNodes
        SDI_INIT_ANALYZE_INFO( &gAnalyzeInfoForAllNodes );

        /* TASK-7219 Shard Transformer Refactoring */
        gAnalyzeInfoForAllNodes.mIsShardQuery        = ID_FALSE;
        gAnalyzeInfoForAllNodes.mNonShardQueryReason = SDI_SHARD_KEYWORD_EXISTS;
    }
    else
    {
        IDU_FIXED_TABLE_DEFINE_RUNTIME( gShardDataNodeInfoTableDesc );
    }

    return IDE_SUCCESS;
}

IDE_RC sdi::checkStmtTypeBeforeAnalysis( qcStatement * aStatement )
{
    return sda::checkStmtTypeBeforeAnalysis( aStatement );
}

// PROJ-2727
IDE_RC sdi::checkDCLStmt( qcStatement * aStatement )
{
    return sda::checkDCLStmt( aStatement );
}

IDE_RC sdi::analyze( qcStatement * aStatement,
                     ULong         aSMN )
{
    IDU_FIT_POINT_FATAL( "sdi::analyze::__FT__" );

    IDE_TEST( sda::analyze( aStatement,
                            aSMN )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::setAnalysisResultForInsert( qcStatement    * aStatement,
                                        sdiAnalyzeInfo * aAnalyzeInfo,
                                        sdiObjectInfo  * aShardObjInfo )
{
    sdiValueInfo    * sValueInfo = NULL;
    SDI_INIT_ANALYZE_INFO(aAnalyzeInfo);

    // analyzer를 통하지 않고 직접 analyze 정보를 생성한다.
    if ( aShardObjInfo->mTableInfo.mSplitMethod != SDI_SPLIT_SOLO )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdiValueInfo),
                                                 (void**)&aAnalyzeInfo->mValuePtrArray )
                  != IDE_SUCCESS );

        IDE_TEST( sda::allocAndSetShardHostValueInfo( aStatement,
                                                      aShardObjInfo->mTableInfo.mKeyColOrder,
                                                      &sValueInfo )
                  != IDE_SUCCESS );

        aAnalyzeInfo->mValuePtrCount = 1;
        aAnalyzeInfo->mValuePtrArray[0] = sValueInfo;

        aAnalyzeInfo->mIsShardQuery = ID_TRUE;
        aAnalyzeInfo->mSplitMethod = aShardObjInfo->mTableInfo.mSplitMethod;
        aAnalyzeInfo->mKeyDataType = aShardObjInfo->mTableInfo.mKeyDataType;
    }
    else
    {
        aAnalyzeInfo->mValuePtrCount = 0;
        aAnalyzeInfo->mIsShardQuery = ID_TRUE;
        aAnalyzeInfo->mSplitMethod = aShardObjInfo->mTableInfo.mSplitMethod;
        aAnalyzeInfo->mKeyDataType = aShardObjInfo->mTableInfo.mKeyDataType;
    }

    aAnalyzeInfo->mSubKeyExists = aShardObjInfo->mTableInfo.mSubKeyExists;
    

    if ( aAnalyzeInfo->mSubKeyExists == ID_TRUE )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdiValueInfo*),
                                                 (void**)&aAnalyzeInfo->mSubValuePtrArray )
                  != IDE_SUCCESS );

        IDE_TEST( sda::allocAndSetShardHostValueInfo( aStatement,
                                                      aShardObjInfo->mTableInfo.mSubKeyColOrder,
                                                      &sValueInfo )
                  != IDE_SUCCESS );

        aAnalyzeInfo->mSubValuePtrCount = 1;
        aAnalyzeInfo->mSubValuePtrArray[0] = sValueInfo;

        aAnalyzeInfo->mSubSplitMethod = aShardObjInfo->mTableInfo.mSubSplitMethod;
        aAnalyzeInfo->mSubKeyDataType = aShardObjInfo->mTableInfo.mSubKeyDataType;
    }
    else
    {
        // Already initialized
        // Nothing to do.
    }

    aAnalyzeInfo->mDefaultNodeId = aShardObjInfo->mTableInfo.mDefaultNodeId;

    IDE_TEST( allocAndCopyRanges( aStatement,
                                  &aAnalyzeInfo->mRangeInfo,
                                  &aShardObjInfo->mRangeInfo )
              != IDE_SUCCESS );

    // PROJ-2685 online rebuild
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                  ID_SIZEOF(sdiTableInfoList),
                  (void**) &(aAnalyzeInfo->mTableInfoList) )
              != IDE_SUCCESS );

    aAnalyzeInfo->mTableInfoList->mTableInfo = &(aShardObjInfo->mTableInfo);
    aAnalyzeInfo->mTableInfoList->mNext = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::setAnalysisResultForDML( qcStatement    * aStatement,
                                     sdiAnalyzeInfo * aAnalyzeInfo,
                                     sdiObjectInfo  * aShardObjInfo )
{
    SDI_INIT_ANALYZE_INFO(aAnalyzeInfo);

    // analyzer를 통하지 않고 직접 analyze 정보를 생성한다.
    aAnalyzeInfo->mValuePtrCount = 0;

    aAnalyzeInfo->mIsShardQuery = ID_TRUE;
    aAnalyzeInfo->mValuePtrArray = NULL;
    aAnalyzeInfo->mSplitMethod = aShardObjInfo->mTableInfo.mSplitMethod;
    aAnalyzeInfo->mKeyDataType = aShardObjInfo->mTableInfo.mKeyDataType;

    aAnalyzeInfo->mSubKeyExists = aShardObjInfo->mTableInfo.mSubKeyExists;

    if ( aAnalyzeInfo->mSubKeyExists == ID_TRUE )
    {
        aAnalyzeInfo->mSubValuePtrCount = 0;
        aAnalyzeInfo->mSubValuePtrArray = NULL;

        aAnalyzeInfo->mSubSplitMethod = aShardObjInfo->mTableInfo.mSubSplitMethod;
        aAnalyzeInfo->mSubKeyDataType = aShardObjInfo->mTableInfo.mSubKeyDataType;
    }
    else
    {
        // Already initialized
        // Nothing to do.
    }

    aAnalyzeInfo->mDefaultNodeId = aShardObjInfo->mTableInfo.mDefaultNodeId;

    IDE_TEST( allocAndCopyRanges( aStatement,
                                  &aAnalyzeInfo->mRangeInfo,
                                  &aShardObjInfo->mRangeInfo )
              != IDE_SUCCESS );

    // PROJ-2685 online rebuild
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                  ID_SIZEOF(sdiTableInfoList),
                  (void**) &(aAnalyzeInfo->mTableInfoList) )
              != IDE_SUCCESS );

    aAnalyzeInfo->mTableInfoList->mTableInfo = &(aShardObjInfo->mTableInfo);
    aAnalyzeInfo->mTableInfoList->mNext = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::setAnalysisResultForTable( qcStatement    * aStatement,
                                       sdiAnalyzeInfo * aAnalyzeInfo,
                                       sdiObjectInfo  * aShardObjInfo )
{
    SDI_INIT_ANALYZE_INFO(aAnalyzeInfo);

    // analyzer를 통하지 않고 직접 analyze 정보를 생성한다.
    aAnalyzeInfo->mIsShardQuery = ID_TRUE;
    aAnalyzeInfo->mSplitMethod = aShardObjInfo->mTableInfo.mSplitMethod;
    aAnalyzeInfo->mKeyDataType = aShardObjInfo->mTableInfo.mKeyDataType;

    aAnalyzeInfo->mSubKeyExists = aShardObjInfo->mTableInfo.mSubKeyExists;
    aAnalyzeInfo->mSubSplitMethod = aShardObjInfo->mTableInfo.mSubSplitMethod;
    aAnalyzeInfo->mSubKeyDataType = aShardObjInfo->mTableInfo.mSubKeyDataType;

    aAnalyzeInfo->mDefaultNodeId = aShardObjInfo->mTableInfo.mDefaultNodeId;

    IDE_TEST( allocAndCopyRanges( aStatement,
                                  &aAnalyzeInfo->mRangeInfo,
                                  &aShardObjInfo->mRangeInfo )
              != IDE_SUCCESS );

    // PROJ-2685 online rebuild
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                  ID_SIZEOF(sdiTableInfoList),
                  (void**) &(aAnalyzeInfo->mTableInfoList) )
              != IDE_SUCCESS );

    aAnalyzeInfo->mTableInfoList->mTableInfo = &(aShardObjInfo->mTableInfo);
    aAnalyzeInfo->mTableInfoList->mNext = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::copyAnalyzeInfo( qcStatement    * aStatement,
                             sdiAnalyzeInfo * aAnalyzeInfo,
                             sdiAnalyzeInfo * aSrcAnalyzeInfo )
{
    /*
     * sdiAnalyzeInfo 전체를 memcpy한 후,
     * pointer variable인 mRangeInfo->mRanges, mValue, mSubValue 에 대해서는
     * 별도로 memory를 alloc하고 복사한다.
     */
    idlOS::memcpy( (void*)aAnalyzeInfo,
                   (void*)aSrcAnalyzeInfo,
                   ID_SIZEOF(sdiAnalyzeInfo) );

    aAnalyzeInfo->mRangeInfo.mCount = 0;
    aAnalyzeInfo->mRangeInfo.mRanges = NULL;
    aAnalyzeInfo->mValuePtrCount = 0;
    aAnalyzeInfo->mValuePtrArray = NULL;
    aAnalyzeInfo->mSubValuePtrCount = 0;
    aAnalyzeInfo->mSubValuePtrArray = NULL;
    aAnalyzeInfo->mTableInfoList = NULL;

    IDE_TEST( allocAndCopyRanges( aStatement,
                                  &aAnalyzeInfo->mRangeInfo,
                                  &aSrcAnalyzeInfo->mRangeInfo )
              != IDE_SUCCESS );

    IDE_TEST( allocAndCopyValues( aStatement,
                                  &aAnalyzeInfo->mValuePtrArray,
                                  &aAnalyzeInfo->mValuePtrCount,
                                  aSrcAnalyzeInfo->mValuePtrArray,
                                  aSrcAnalyzeInfo->mValuePtrCount )
              != IDE_SUCCESS );

    IDE_TEST( allocAndCopyValues( aStatement,
                                  &aAnalyzeInfo->mSubValuePtrArray,
                                  &aAnalyzeInfo->mSubValuePtrCount,
                                  aSrcAnalyzeInfo->mSubValuePtrArray,
                                  aSrcAnalyzeInfo->mSubValuePtrCount )
              != IDE_SUCCESS );

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( allocAndCopyTableInfoList( aStatement,
                                         aSrcAnalyzeInfo->mTableInfoList,
                                         &( aAnalyzeInfo->mTableInfoList ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

sdiAnalyzeInfo * sdi::getAnalysisResultForAllNodes()
{
    return & gAnalyzeInfoForAllNodes;
}

idBool sdi::checkMetaCreated()
{
    smiTrans       sTrans;
    smiStatement   sSmiStmt;
    smiStatement * sDummySmiStmt = NULL;
    UInt           sStage = 0;

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sTrans.begin( &sDummySmiStmt, NULL ) != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              ( SMI_STATEMENT_UNTOUCHABLE |
                                SMI_STATEMENT_MEMORY_CURSOR ) )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( sdm::isShardMetaCreated( &sSmiStmt ) != ID_TRUE );

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sTrans.commit() != IDE_SUCCESS );
    sStage = 1;

    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    return ID_TRUE;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 3:
            ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
            /* fall through */
        case 2:
            ( void )sTrans.rollback();
            /* fall through */
        case 1:
            ( void )sTrans.destroy( NULL );
            /* fall through */
        default:
            break;
    }

    IDE_CLEAR();

    return ID_FALSE;
}

IDE_RC sdi::getExternalNodeInfo( sdiNodeInfo * aNodeInfo,
                                 ULong         aSMN )
{
    smiTrans       sTrans;
    smiStatement   sSmiStmt;
    smiStatement * sDummySmiStmt;
    UInt           sStage = 0;

    ULong          sDataSMNForMetaInfo = SDI_NULL_SMN;

    IDE_DASSERT( aNodeInfo != NULL );

    /* init */
    aNodeInfo->mCount = 0;

    /* PROJ-2446 ONE SOURCE MM 에서 statistics정보를 넘겨 받아야 한다.
     * 추후 작업 */
    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sTrans.begin( &sDummySmiStmt, NULL )
              != IDE_SUCCESS );
    sStage = 2;

    /* PROJ-2701 Online data rebuild */
    if ( getSMNForMetaNode() < aSMN )
    {
        IDE_TEST( waitAndSetSMNForMetaNode( NULL,
                                            sDummySmiStmt,
                                            ( SMI_STATEMENT_UNTOUCHABLE | SMI_STATEMENT_MEMORY_CURSOR ),
                                            aSMN,
                                            &sDataSMNForMetaInfo )
                  != IDE_SUCCESS );

        IDE_DASSERT( aSMN == sDataSMNForMetaInfo );
    }
    else
    {
        sDataSMNForMetaInfo = aSMN;
    }

    IDE_TEST( sSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              (SMI_STATEMENT_UNTOUCHABLE |
                               SMI_STATEMENT_MEMORY_CURSOR) )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( sdm::getExternalNodeInfo( &sSmiStmt,
                                        aNodeInfo,
                                        sDataSMNForMetaInfo ) != IDE_SUCCESS );

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sTrans.commit() != IDE_SUCCESS );
    sStage = 1;

    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 3:
            ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
            /* fall through */
        case 2:
            ( void )sTrans.rollback();
            /* fall through */
        case 1:
            ( void )sTrans.destroy( NULL );
            /* fall through */
        default:
            break;
    }

    ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. errorcode 0x%05"ID_XINT32_FMT" %s\n",
                           E_ERROR_CODE(ideGetErrorCode()),
                           ideGetErrorMsg(ideGetErrorCode()));
    return IDE_FAILURE;
}

IDE_RC sdi::getInternalNodeInfo( smiTrans    * aTrans,
                                 sdiNodeInfo * aNodeInfo,
                                 idBool        aIsShardMetaChanged,
                                 ULong         aSMN )
{
    smiTrans       sTrans;
    smiStatement   sSmiStmt;
    smiStatement * sDummySmiStmt = NULL;
    UShort         sTransStage = 0;
    idBool         sIsStmtBegun = ID_FALSE;

    ULong          sDataSMNForMetaInfo = SDI_NULL_SMN;
    idvSQL       * sStatistics = NULL;
    smiStatement * sSmiStmtForMetaInfo = NULL;

    IDE_DASSERT( aNodeInfo != NULL );

    /* init */
    aNodeInfo->mCount = 0;

    if ( aTrans == NULL )
    {
        IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
        sTransStage = 1;

        IDE_TEST( sTrans.begin( &sDummySmiStmt, NULL )
                  != IDE_SUCCESS );
        sTransStage = 2;

        sStatistics = NULL;
        sSmiStmtForMetaInfo = sDummySmiStmt;        
    }
    else
    {
        IDE_TEST_RAISE( aTrans->isBegin() == ID_FALSE, ERR_INVALID_TRANS );

        sStatistics = aTrans->getStatistics();
        sSmiStmtForMetaInfo = aTrans->getStatement();
    }

    // PROJ-2701 Online data rebuild
    if ( aIsShardMetaChanged == ID_TRUE )
    {
        /* Shard Meta를 변경한 Transaction이므로 aSMN에 해당하는 Internal Node Info를 얻을 수 있다. */
        sDataSMNForMetaInfo = aSMN;
    }
    else if ( getSMNForMetaNode() < aSMN )
    {
        IDE_TEST( waitAndSetSMNForMetaNode( sStatistics,
                                            sSmiStmtForMetaInfo,
                                            ( SMI_STATEMENT_UNTOUCHABLE | SMI_STATEMENT_MEMORY_CURSOR ),
                                            aSMN,
                                            &sDataSMNForMetaInfo )
                  != IDE_SUCCESS );

        IDE_DASSERT( aSMN == sDataSMNForMetaInfo );
    }
    else
    {
        sDataSMNForMetaInfo = aSMN;
    }

    IDE_TEST( sSmiStmt.begin( sStatistics,
                              sSmiStmtForMetaInfo,
                              (SMI_STATEMENT_UNTOUCHABLE |
                               SMI_STATEMENT_MEMORY_CURSOR) )
              != IDE_SUCCESS );
    sIsStmtBegun = ID_TRUE;

    IDE_TEST( sdm::getInternalNodeInfo( &sSmiStmt,
                                        aNodeInfo,
                                        sDataSMNForMetaInfo )
              != IDE_SUCCESS );

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsStmtBegun = ID_FALSE;

    if ( aTrans == NULL )
    {
        IDE_TEST( sTrans.commit() != IDE_SUCCESS );
        sTransStage = 1;

        sTransStage = 0;
        IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_TRANS )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::getInternalNodeInfo",
                                  "Invalid transaction" ) );
    }
    IDE_EXCEPTION_END;    

    if ( sIsStmtBegun == ID_TRUE )
    {
        (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
    }
    else
    {
        /* Nothing to do */
    }

    switch ( sTransStage )
    {
        case 2:
            ( void )sTrans.rollback();
            /* fall through */
        case 1:
            ( void )sTrans.destroy( NULL );
            /* fall through */
        default:
            break;
    }

    ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. errorcode 0x%05"ID_XINT32_FMT" %s\n",
                           E_ERROR_CODE(ideGetErrorCode()),
                           ideGetErrorMsg(ideGetErrorCode()));
    return IDE_FAILURE;
}

/* PROJ-2655 Composite shard key */
IDE_RC sdi::getRangeIndexByValue( qcTemplate     * aTemplate,
                                  mtcTuple       * aShardKeyTuple,
                                  sdiAnalyzeInfo * aShardAnalysis,
                                  UShort           aValueIndex,
                                  sdiValueInfo   * aValue,
                                  sdiRangeIndex  * aRangeIndex,
                                  UShort         * aRangeIndexCount,
                                  idBool         * aHasDefaultNode,
                                  idBool           aIsSubKey )
{
    IDU_FIT_POINT_FATAL( "sdi::getRangeIndexByValue::__FT__" );

    IDE_TEST( sda::getRangeIndexByValue( aTemplate,
                                         aShardKeyTuple,
                                         aShardAnalysis,
                                         aValueIndex,
                                         aValue,
                                         aRangeIndex,
                                         aRangeIndexCount,
                                         aHasDefaultNode,
                                         aIsSubKey )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::checkValuesSame( qcTemplate   * aTemplate,
                             mtcTuple     * aShardKeyTuple,
                             UInt           aKeyDataType,
                             sdiValueInfo * aValue1,
                             sdiValueInfo * aValue2,
                             idBool       * aIsSame )
{
    IDU_FIT_POINT_FATAL( "sdi::checkValuesSame::__FT__" );

    IDE_TEST( sda::checkValuesSame( aTemplate,
                                    aShardKeyTuple,
                                    aKeyDataType,
                                    aValue1,
                                    aValue2,
                                    aIsSame )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::validateNodeNames( qcStatement  * aStatement,
                               qcShardNodes * aNodeNames )
{
    qcShardNodes     * sNodeName;
    qcShardNodes     * sNodeName2;
    qcuSqlSourceInfo   sqlInfo;

    for ( sNodeName = aNodeNames;
          sNodeName != NULL;
          sNodeName = sNodeName->next )
    {
        // length 검사
        if ( ( sNodeName->namePos.size <= 0 ) ||
             ( sNodeName->namePos.size > SDI_NODE_NAME_MAX_SIZE ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sNodeName->namePos );
            IDE_RAISE( ERR_INVALID_NODE_NAME );
        }
        else
        {
            // Nothing to do.
        }

        for ( sNodeName2 = sNodeName->next;
              sNodeName2 != NULL;
              sNodeName2 = sNodeName2->next )
        {
            // duplicate 검사
            if ( QC_IS_NAME_CASELESS_MATCHED( sNodeName->namePos,
                                              sNodeName2->namePos ) == ID_TRUE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sNodeName2->namePos );
                IDE_RAISE( ERR_DUP_NODE_NAME );
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_NODE_NAME )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_INVALID_NODE_NAME,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_DUP_NODE_NAME );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_DUPLICATED_NODE_NAME,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::allocAndCopyRanges( qcStatement  * aStatement,
                                sdiRangeInfo * aTo,
                                sdiRangeInfo * aFrom )
{   
    IDE_DASSERT( aTo   != NULL);

    if ( aFrom != NULL )
    {
        if ( aFrom->mCount > 0 )
        {
            IDE_TEST_RAISE( aFrom->mRanges == NULL, ERR_NULL_SRC_RANGES );

            if ( aFrom->mCount > aTo->mCount )
            {
                IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdiRange) *
                                                         aFrom->mCount,
                                                         (void**) & aTo->mRanges )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST_RAISE( aTo->mRanges == NULL, ERR_NULL_DEST_RANGES );
            }

            idlOS::memcpy( aTo->mRanges,
                           aFrom->mRanges,
                           ID_SIZEOF(sdiRange) * aFrom->mCount );
        }
        else
        {
            aTo->mRanges = NULL;
        }

        aTo->mCount = aFrom->mCount;
    }
    else
    {
        aTo->mCount = 0;
        aTo->mRanges = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_SRC_RANGES )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::allocAndCopyRanges",
                                  "The range-of-source is null." ) );
    }
    IDE_EXCEPTION( ERR_NULL_DEST_RANGES )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::allocAndCopyRanges",
                                  "The range-of-destination is null." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::allocAndCopyValues( qcStatement          * aStatement,
                                sdiValueInfo *      ** aTo,
                                UShort               * aToCount,
                                sdiValueInfo        ** aFrom,
                                UShort                 aFromCount )
{
    UInt                   i = 0;    
    UInt                   sSize = 0;
    idBool                 sNewArray = ID_FALSE;
    sdiValueInfo        ** sValueInfoPtrArray = NULL;
    sdiValueInfo         * sValueInfo = NULL;

    IDE_DASSERT( aTo != NULL );

    if ( aFromCount > 0 )
    {
        IDE_TEST_RAISE( aFrom == NULL, ERR_NULL_SRC_VALUE );

        if ( aFromCount > *aToCount )
        {
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdiValueInfo*) *
                                                     aFromCount,
                                                     (void**)&sValueInfoPtrArray )
                      != IDE_SUCCESS );
            sNewArray = ID_TRUE;
        }
        else
        {
            IDE_TEST_RAISE( *aTo == NULL, ERR_NULL_DEST_VALUE );
            sValueInfoPtrArray = *aTo;
        }

        for ( i = 0; i < aFromCount; i++ )
        {
            sSize = sda::calculateValueInfoSize( aFrom[i]->mDataValueType,
                                                 &(aFrom[i]->mValue) );

            if ( ( sSize <= ID_SIZEOF( sdiValueInfo ) ) &&
                 ( sNewArray == ID_FALSE ) )
            {
                idlOS::memcpy( sValueInfoPtrArray[i],
                               aFrom[i],
                               ID_SIZEOF(sdiValueInfo) );
            }
            else
            {
                IDE_TEST( sda::allocAndCopyValue( aStatement,
                                                  aFrom[i],
                                                  &sValueInfo )
                          != IDE_SUCCESS );

                sValueInfoPtrArray[i] = sValueInfo;
            }
        }

        *aTo = sValueInfoPtrArray;
        *aToCount = aFromCount;
    }
    else
    {
        *aTo = NULL;
        *aToCount = aFromCount;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_SRC_VALUE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::allocAndCopyValues",
                                  "The value-of-source is null." ) );
    }
    IDE_EXCEPTION( ERR_NULL_DEST_VALUE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::allocAndCopyValues",
                                  "The value-of-destination is null." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::checkShardLinker( qcStatement * aStatement )
{
    ULong       sSMN      = SDI_NULL_SMN;
    ULong       sLastSMN  = SDI_NULL_SMN;
    qciStmtType sStmtType = aStatement->myPlan->parseTree->stmtKind;

    /* TASK-7219 Non-shard DML */
    idBool      sIsPartialCoord = ID_FALSE;

    if ( ( qci::getStartupPhase() == QCI_STARTUP_SERVICE ) &&
         ( aStatement->session->mQPSpecific.mClientInfo == NULL ) )
    {
        sIsPartialCoord = sdi::isPartialCoordinator( aStatement );
        
        if ( ( sdi::isShardCoordinator( aStatement ) == ID_TRUE ) ||
             ( sIsPartialCoord == ID_TRUE ) )
        {
            // shardCoordinator using sessionSMN 
            sSMN = QCG_GET_SESSION_SHARD_META_NUMBER( aStatement );
        }
        else
        {
            /* PROJ-2745 rebuild coordinator 없어졌다. */
            if ( QCG_GET_SESSION_IS_SHARD_INTERNAL_LOCAL_OPERATION( aStatement ) != ID_TRUE ) 
            {
                IDE_TEST_RAISE( sdi::getShardStatus() != 1, ERR_ABORT_SDI_SHARD_NOT_JOIN );
            }

            // rebuildCoordinator using dataSMN
            sSMN = sdi::getSMNForDataNode();
        }
        
        sLastSMN = QCG_GET_LAST_SESSION_SHARD_META_NUMBER( aStatement );

        if ( sSMN != SDI_NULL_SMN )
        {
            // PROJ-2727
            if (( sStmtType == QCI_STMT_SET_SESSION_PROPERTY ) ||
                ( sStmtType == QCI_STMT_SET_SYSTEM_PROPERTY ))
            {
                IDE_TEST( makeShardSession( aStatement->session,
                                            QCG_GET_DATABASE_LINK_SESSION( aStatement ),
                                            NULL, // DCL not exist Trans
                                            ID_FALSE,
                                            sSMN,
                                            sLastSMN,
                                            SDI_REBUILD_SMN_PROPAGATE,
                                            sIsPartialCoord )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( makeShardSession( aStatement->session,
                                            QCG_GET_DATABASE_LINK_SESSION( aStatement ),
                                            ( QC_SMI_STMT( aStatement ) )->getTrans(),
                                            ID_FALSE,
                                            sSMN,
                                            sLastSMN,
                                            SDI_REBUILD_SMN_PROPAGATE,
                                            sIsPartialCoord )
                          != IDE_SUCCESS );

            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }
    
    // BUG-47765    
    IDE_TEST( copyPropertyFlagToCoordPropertyFlag( aStatement->session,
                                                   aStatement->session->mQPSpecific.mClientInfo )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( aStatement->session->mQPSpecific.mClientInfo == NULL,
                    ERR_SHARD_LINKER_NOT_INITIALIZED );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_LINKER_NOT_INITIALIZED )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_SHARD_LINKER_NOT_INITIALIZED ) );
    }
    // R2HA BUG-48367
    IDE_EXCEPTION( ERR_ABORT_SDI_SHARD_NOT_JOIN )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_SHARD_NOT_JOIN ) );
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::checkShardLinkerWithoutSQL( qcStatement * aStatement, ULong aSMN, smiTrans * aTrans )
{
    ULong sLastSMN  = SDI_NULL_SMN;

    if ( ( qci::getStartupPhase() == QCI_STARTUP_SERVICE ) &&
         ( aStatement->session->mQPSpecific.mClientInfo == NULL ) )
    {
        IDE_TEST_RAISE( aSMN == SDI_NULL_SMN, ERR_SMN_ARG_ERR );

        sLastSMN = QCG_GET_LAST_SESSION_SHARD_META_NUMBER( aStatement );

        IDE_TEST( makeShardSession( aStatement->session,
                                    QCG_GET_DATABASE_LINK_SESSION( aStatement ),
                                    aTrans,
                                    ID_FALSE,
                                    aSMN,
                                    sLastSMN,
                                    SDI_REBUILD_SMN_PROPAGATE,
                                    ID_FALSE /* aIsPartialCoord */ )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( copyPropertyFlagToCoordPropertyFlag( aStatement->session,
                                                   aStatement->session->mQPSpecific.mClientInfo )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( aStatement->session->mQPSpecific.mClientInfo == NULL,
                    ERR_SHARD_LINKER_NOT_INITIALIZED );
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_LINKER_NOT_INITIALIZED )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_SHARD_LINKER_NOT_INITIALIZED ) );
    }
    IDE_EXCEPTION( ERR_SMN_ARG_ERR )
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,"sdi::checkShardLinkerInternal", "aSMN == 0" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::checkShardLinkerWithNodeList( qcStatement * aStatement, 
                                          ULong         aSMN, 
                                          smiTrans    * aTrans, 
                                          iduList     * aNodeList )
{
    if ( qci::getStartupPhase() == QCI_STARTUP_SERVICE )
    {
        IDE_TEST_RAISE( aSMN == ID_ULONG(0), ERR_SMN_ARG_ERR );

        if ( aStatement->session->mQPSpecific.mClientInfo == NULL )
        {
            if( SDU_SHARD_ZOOKEEPER_TEST == 1 )
            {                    
                IDE_TEST( initializeSessionWithNodeList( aStatement->session,
                                                         QCG_GET_DATABASE_LINK_SESSION( aStatement ),
                                                         aTrans,
                                                         ID_FALSE,
                                                         aSMN,
                                                         aNodeList )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( initializeSession( aStatement->session,
                                             QCG_GET_DATABASE_LINK_SESSION( aStatement ),
                                             aTrans,
                                             ID_FALSE,
                                             aSMN )
                          != IDE_SUCCESS );
            }
            
        }
        else
        {
            finalizeSession( aStatement->session );
            
            if( SDU_SHARD_ZOOKEEPER_TEST == 1 )
            {
                    
                IDE_TEST( initializeSessionWithNodeList( aStatement->session,
                                                         QCG_GET_DATABASE_LINK_SESSION( aStatement ),
                                                         aTrans,
                                                         ID_FALSE,
                                                         aSMN,
                                                         aNodeList )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( initializeSession( aStatement->session,
                                             QCG_GET_DATABASE_LINK_SESSION( aStatement ),
                                             aTrans,
                                             ID_FALSE,
                                             aSMN )
                          != IDE_SUCCESS );
            }                
        }
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( copyPropertyFlagToCoordPropertyFlag( aStatement->session,
                                                   aStatement->session->mQPSpecific.mClientInfo )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( aStatement->session->mQPSpecific.mClientInfo == NULL,
                    ERR_SHARD_LINKER_NOT_INITIALIZED );
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_LINKER_NOT_INITIALIZED )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_SHARD_LINKER_NOT_INITIALIZED ) );
    }
    IDE_EXCEPTION( ERR_SMN_ARG_ERR )
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,"sdi::checkShardLinkerInternal", "aSMN == 0" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

sdiConnectInfo * sdi::findConnect( sdiClientInfo * aClientInfo,
                                   UInt            aNodeId )
{
    sdiConnectInfo * sConnectInfo = NULL;
    UInt             i;

    sConnectInfo = aClientInfo->mConnectInfo;

    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++ )
    {
        if ( sConnectInfo->mNodeId == aNodeId )
        {
            return sConnectInfo;
        }
        else
        {
            // Nothing to do.
        }
    }

    return NULL;
}

idBool sdi::findBindParameter( sdiAnalyzeInfo * aAnalyzeInfo )
{
    idBool  sFound = ID_FALSE;
    UInt    i;

    for ( i = 0; i < aAnalyzeInfo->mValuePtrCount; i++ )
    {
        if ( ( aAnalyzeInfo->mValuePtrArray[i]->mType == 0 ) ||
             ( aAnalyzeInfo->mValuePtrArray[i]->mType == 2 ) /* TASK-7219 Transformed out ref column bind */
             )
        {
            // a bind parameter exists
            sFound = ID_TRUE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( aAnalyzeInfo->mSubKeyExists == ID_TRUE )
    {
        for ( i = 0; i < aAnalyzeInfo->mSubValuePtrCount; i++ )
        {
            if ( ( aAnalyzeInfo->mSubValuePtrArray[i]->mType == 0 ) ||
                 ( aAnalyzeInfo->mSubValuePtrArray[i]->mType == 2 ) /* TASK-7219 Transformed out ref column bind */
                 )
            {
                // a bind parameter exists
                sFound = ID_TRUE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    return sFound;
}

idBool sdi::findRangeInfo( sdiRangeInfo * aRangeInfo,
                           UInt           aNodeId )
{
    return sda::findRangeInfo( aRangeInfo,
                               aNodeId );
}

IDE_RC sdi::getProcedureInfo( qcStatement      * aStatement,
                              UInt               aUserID,
                              qcNamePosition     aUserName,
                              qcNamePosition     aProcName,
                              sdiObjectInfo   ** aShardObjInfo )
{
    qsOID   sProcOID;
    SChar   sProcName[QCI_MAX_OBJECT_NAME_LEN +1] = {0,};
    qsxProcInfo     * sProcInfo = NULL;

    QC_STR_COPY( sProcName, aProcName );
    IDE_TEST( qcmProc::getProcExistWithEmptyByNamePtr( aStatement,
                                                       aUserID,
                                                       sProcName,
                                                       aProcName.size,
                                                       &sProcOID )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sProcOID == QS_EMPTY_OID, ERR_NOT_EXIST_OBJECT );

    IDE_TEST( qsxProc::getProcInfo( sProcOID, &sProcInfo ) != IDE_SUCCESS );
    IDE_TEST( sdi::getProcedureInfoWithPlanTree( aStatement,
                                                 aUserID,
                                                 aUserName,
                                                 aProcName,
                                                 sProcInfo->planTree,
                                                 aShardObjInfo ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION( ERR_NOT_EXIST_OBJECT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_NOT_EXIST_PROC_SQLTEXT,
                                  sProcName ) );
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

// TASK-7244 Set shard split method to PSM info
IDE_RC sdi::getProcedureShardSplitMethod( qcStatement    * aStatement,
                                          UInt             aUserID,
                                          qcNamePosition   aUserName,
                                          qcNamePosition   aProcName,
                                          sdiSplitMethod * aShardSplitMethod )
{
    smiStatement      sSmiStmt;
    idBool            sIsBeginStmt = ID_FALSE;
    SChar             sUserName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar             sProcName[QC_MAX_OBJECT_NAME_LEN + 1];
    sdiTableInfo      sShardTableInfo;
    idBool            sIsTableFound = ID_FALSE;

    sdiGlobalMetaInfo   sMetaNodeInfo = { ID_ULONG(0) };

    if ( ( ( qciMisc::isStmtDML( aStatement->myPlan->parseTree->stmtKind ) == ID_TRUE ) ||
           ( qciMisc::isStmtSP( aStatement->myPlan->parseTree->stmtKind ) == ID_TRUE )  ||
           ( qciMisc::isStmtDDL( aStatement->myPlan->parseTree->stmtKind ) == ID_TRUE ) ) &&
         ( aUserID != QC_SYSTEM_USER_ID ) &&
         ( aUserID != getShardUserID() ) )
    {
        IDE_DASSERT( QC_IS_NULL_NAME( aProcName ) == ID_FALSE );

        if ( QC_IS_NULL_NAME( aUserName ) == ID_TRUE )
        {
            IDE_TEST(qciMisc::getUserName(aStatement, aUserID, sUserName) != IDE_SUCCESS);
            QC_STR_COPY( sProcName, aProcName );
        }
        else
        {
            QC_STR_COPY( sUserName, aUserName );
            QC_STR_COPY( sProcName, aProcName );
        }

        IDE_TEST( sdm::getGlobalMetaInfo( &sMetaNodeInfo ) != IDE_SUCCESS );

        // 별도의 stmt를 열어 항상 최신의 view를 본다.
        // (shard meta는 일반 memory table이므로 normal로 열되,
        // 상위 stmt가 untouchable일 수 있으므로 self를 추가한다.)
        IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                                  QC_SMI_STMT( aStatement ),
                                  SMI_STATEMENT_NORMAL |
                                  SMI_STATEMENT_SELF_TRUE |
                                  SMI_STATEMENT_ALL_CURSOR )
                  != IDE_SUCCESS );
        sIsBeginStmt = ID_TRUE;

        if ( sdm::getTableInfo( &sSmiStmt,
                                sUserName,
                                sProcName,
                                sMetaNodeInfo.mShardMetaNumber,
                                &sShardTableInfo,
                                &sIsTableFound ) == IDE_SUCCESS )
        {
            if ( sIsTableFound == ID_TRUE )
            {
                *aShardSplitMethod = sShardTableInfo.mSplitMethod;
            }
            else
            {
                *aShardSplitMethod = SDI_SPLIT_NONE;
            }
        }
        else
        {
            *aShardSplitMethod = SDI_SPLIT_NONE;
        }

        sIsBeginStmt = ID_FALSE;
        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    }
    else
    {
        *aShardSplitMethod = SDI_SPLIT_NONE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsBeginStmt == ID_TRUE )
    {
        (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
    }

    return IDE_FAILURE;
}

IDE_RC sdi::getProcedureInfoWithPlanTree( qcStatement      * aStatement,
                                          UInt               aUserID,
                                          qcNamePosition     aUserName,
                                          qcNamePosition     aProcName,
                                          qsProcParseTree  * aProcPlanTree,
                                          sdiObjectInfo   ** aShardObjInfo )
{
/***********************************************************************
 *
 * Description : PROJ-2598 Shard Meta
 *
 * Implementation :
 *
 ***********************************************************************/

    smiStatement      sSmiStmt;
    idBool            sIsBeginStmt = ID_FALSE;
    qsVariableItems * sParaDecls;
    SChar             sUserName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar             sProcName[QC_MAX_OBJECT_NAME_LEN + 1];
    sdiTableInfo      sShardTableInfo;
    sdiObjectInfo   * sShardObjInfo = NULL;
    idBool            sExistKey = ID_FALSE;
    idBool            sExistSubKey = ID_FALSE;
    idBool            sIsTableFound = ID_FALSE;
    UInt              sKeyType;
    UInt              sSubKeyType;
    UInt              i = 0;

    /*
     * PROJ-2701 Online data rebuild
     *
     * sSMN[0] : sessionSMN
     * sSMN[1] : dataSMN
     */
    ULong  sSMN[2] = { SDI_NULL_SMN, SDI_NULL_SMN };
    ULong  sSMNForShardObj = SDI_NULL_SMN;

    if ( ( ( qciMisc::isStmtDML( aStatement->myPlan->parseTree->stmtKind ) == ID_TRUE ) ||
           ( qciMisc::isStmtSP( aStatement->myPlan->parseTree->stmtKind ) == ID_TRUE )  ||
           ( qciMisc::isStmtDDL( aStatement->myPlan->parseTree->stmtKind ) == ID_TRUE ) ) &&
         ( aUserID != QC_SYSTEM_USER_ID ) &&
         ( aUserID != getShardUserID() ) &&
         ( *aShardObjInfo == NULL ) )
    {
        IDE_DASSERT( QC_IS_NULL_NAME( aProcName ) == ID_FALSE );

        if ( QC_IS_NULL_NAME( aUserName ) == ID_TRUE )
        {
            idlOS::snprintf( sUserName, ID_SIZEOF(sUserName),
                             "%s",
                             QCG_GET_SESSION_USER_NAME( aStatement ) );
            QC_STR_COPY( sProcName, aProcName );
        }
        else
        {
            QC_STR_COPY( sUserName, aUserName );
            QC_STR_COPY( sProcName, aProcName );
        }

        // PROJ-2701 Online date rebuild
        if ( ( aStatement->session->mQPSpecific.mFlag & QC_SESSION_ALTER_META_MASK )
             == QC_SESSION_ALTER_META_DISABLE )
        {
            sSMN[0] = QCG_GET_SESSION_SHARD_META_NUMBER( aStatement );

            if ( sSMN[0] != SDI_NULL_SMN )
            {
                IDE_TEST( waitAndSetSMNForMetaNode( aStatement->mStatistics,
                                                    QC_SMI_STMT( aStatement ),
                                                    ( SMI_STATEMENT_NORMAL |
                                                      SMI_STATEMENT_SELF_TRUE |
                                                      SMI_STATEMENT_ALL_CURSOR ),
                                                    SDI_NULL_SMN,
                                                    &sSMN[1] )
                          != IDE_SUCCESS );
            }
            else
            {
                // sSMNCount = 0;
                // Sharding을 쓰지 않음.
                // Nothing to do.
            }
        }
        else
        {
            // At shard meta creation phase ( Initial SMN )
            // Nothing to do.
        }

        // 별도의 stmt를 열어 항상 최신의 view를 본다.
        // (shard meta는 일반 memory table이므로 normal로 열되,
        // 상위 stmt가 untouchable일 수 있으므로 self를 추가한다.)
        IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                                  QC_SMI_STMT( aStatement ),
                                  SMI_STATEMENT_NORMAL |
                                  SMI_STATEMENT_SELF_TRUE |
                                  SMI_STATEMENT_ALL_CURSOR )
                  != IDE_SUCCESS );
        sIsBeginStmt = ID_TRUE;

        // SessionSMN ~ dataSMN 사이의 shardObjInfo를 생성한다.
        for ( sSMNForShardObj = sSMN[0]; sSMNForShardObj <= sSMN[1]; sSMNForShardObj++ )
        {
            if ( sdm::getTableInfo( &sSmiStmt,
                                    sUserName,
                                    sProcName,
                                    sSMNForShardObj,
                                    &sShardTableInfo,
                                    &sIsTableFound ) == IDE_SUCCESS )
            {
                if ( sIsTableFound == ID_TRUE )
                {
                    if ( ( sShardTableInfo.mObjectType == 'P' ) &&
                         ( sShardTableInfo.mSplitMethod != SDI_SPLIT_NONE ) )
                    {
                        IDE_TEST_RAISE( ( sdi::getSplitType( sShardTableInfo.mSplitMethod )
                                          == SDI_SPLIT_TYPE_DIST  ) &&
                                        ( sShardTableInfo.mKeyColumnName[0] == '\0' ),
                                        ERR_NO_SHARD_KEY_COLUMN );

                        // keyFlags를 추가생성한다.
                        IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                                      ID_SIZEOF( sdiObjectInfo ) + aProcPlanTree->paraDeclCount,
                                      (void**) & sShardObjInfo )
                                  != IDE_SUCCESS );

                        // set shard table info
                        idlOS::memcpy( (void*)&(sShardObjInfo->mTableInfo),
                                       (void*)&sShardTableInfo,
                                       ID_SIZEOF( sdiTableInfo ) );

                        // set key flags
                        idlOS::memset( sShardObjInfo->mKeyFlags, 0x00, aProcPlanTree->paraDeclCount );

                        // set SMN
                        sShardObjInfo->mSMN = sSMNForShardObj;

                        if ( getSplitType( sShardTableInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
                        {
                            // Hash, Range or List-based sharding 의 shard key column을 찾는다.
                            for ( sParaDecls = aProcPlanTree->paraDecls, i = 0;
                                  sParaDecls != NULL;
                                  sParaDecls = sParaDecls->next, i++ )
                            {
                                if ( idlOS::strMatch( sParaDecls->name.stmtText + sParaDecls->name.offset,
                                                      sParaDecls->name.size,
                                                      sShardTableInfo.mKeyColumnName,
                                                      idlOS::strlen(sShardTableInfo.mKeyColumnName) ) == 0 )
                                {
                                    IDE_TEST_RAISE( sParaDecls->itemType != QS_VARIABLE,
                                                    ERR_INVALID_SHARD_KEY_TYPE );
                                    IDE_TEST_RAISE( ((qsVariables*)sParaDecls)->inOutType != QS_IN,
                                                    ERR_INVALID_SHARD_KEY_TYPE );

                                    sKeyType = aProcPlanTree->paramModules[i]->id;
                                    IDE_TEST_RAISE( isSupportDataType( sKeyType ) == ID_FALSE,
                                                    ERR_INVALID_SHARD_KEY_TYPE );

                                    sShardObjInfo->mTableInfo.mKeyDataType = sKeyType;
                                    sShardObjInfo->mTableInfo.mKeyColOrder = (UShort)i;
                                    sShardObjInfo->mKeyFlags[i] = 1;
                                    sExistKey = ID_TRUE;


                                    if ( ( sShardTableInfo.mSubKeyExists == ID_FALSE ) ||
                                         ( sExistSubKey == ID_TRUE ) )
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        // Nothing to do.
                                    }
                                }
                                else
                                {
                                    // Nothing to do.
                                }

                                /* PROJ-2655 Composite shard key */
                                if ( sShardTableInfo.mSubKeyExists == ID_TRUE )
                                {
                                    if ( idlOS::strMatch( sParaDecls->name.stmtText + sParaDecls->name.offset,
                                                          sParaDecls->name.size,
                                                          sShardTableInfo.mSubKeyColumnName,
                                                          idlOS::strlen(sShardTableInfo.mSubKeyColumnName) ) == 0 )
                                    {
                                        sExistSubKey = ID_TRUE;
                                        IDE_TEST_RAISE( sParaDecls->itemType != QS_VARIABLE,
                                                        ERR_INVALID_SHARD_KEY_TYPE );
                                        IDE_TEST_RAISE( ((qsVariables*)sParaDecls)->inOutType != QS_IN,
                                                        ERR_INVALID_SHARD_KEY_TYPE );

                                        sSubKeyType = aProcPlanTree->paramModules[i]->id;
                                        IDE_TEST_RAISE( isSupportDataType( sSubKeyType ) == ID_FALSE,
                                                        ERR_INVALID_SHARD_KEY_TYPE );

                                        sShardObjInfo->mTableInfo.mSubKeyDataType = sSubKeyType;
                                        sShardObjInfo->mTableInfo.mSubKeyColOrder = (UShort)i;
                                        sShardObjInfo->mKeyFlags[i] = 2;

                                        if ( sExistKey == ID_TRUE )
                                        {
                                            break;
                                        }
                                        else
                                        {
                                            // Nothing to do.
                                        }
                                    }
                                    else
                                    {
                                        // Nothing to do.
                                    }
                                }
                                else
                                {
                                    // Nothing to do.
                                }
                            }

                            IDE_TEST_RAISE( sExistKey == ID_FALSE, ERR_NOT_EXIST_SHARD_KEY );

                            IDE_TEST_RAISE( ( ( sShardTableInfo.mSubKeyExists == ID_TRUE ) &&
                                              ( sExistSubKey == ID_FALSE ) ),
                                            ERR_NOT_EXIST_SUB_SHARD_KEY );
                        }
                        else
                        {
                            // Nothing to do.
                        }

                        // set shard range info
                        IDE_TEST( sdm::getRangeInfo( aStatement,
                                                     &sSmiStmt,
                                                     sShardObjInfo->mSMN,
                                                     &(sShardObjInfo->mTableInfo),
                                                     &(sShardObjInfo->mRangeInfo),
                                                     ID_TRUE )
                                  != IDE_SUCCESS );

                        if ( sShardObjInfo->mSMN == sSMN[1] )
                        {
                            // default node도 없고 range 정보도 없다면 에러
                            IDE_TEST_RAISE(
                                ( sShardObjInfo->mTableInfo.mDefaultNodeId == SDI_NODE_NULL_ID ) &&
                                ( sShardObjInfo->mRangeInfo.mCount == 0 ),
                                ERR_INCOMPLETE_RANGE_SET );
                        }
                        else
                        {
                            // Nothing to do.
                        }

                        sShardObjInfo->mNext = *aShardObjInfo;
                        *aShardObjInfo = sShardObjInfo;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }
        }

        sIsBeginStmt = ID_FALSE;
        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INCOMPLETE_RANGE_SET )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_INCOMPLETE_RANGE_SET,
                                  sUserName,
                                  sProcName ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_SHARD_KEY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_KEY_COLUMN_NOT_EXIST,
                                  sUserName,
                                  sProcName,
                                  sShardTableInfo.mKeyColumnName ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_SUB_SHARD_KEY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_KEY_COLUMN_NOT_EXIST,
                                  sUserName,
                                  sProcName,
                                  sShardTableInfo.mSubKeyColumnName ) );
    }
    IDE_EXCEPTION( ERR_INVALID_SHARD_KEY_TYPE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_UNSUPPORTED_SHARD_KEY_COLUMN_TYPE,
                                  sUserName,
                                  sProcName,
                                  ( sExistSubKey == ID_TRUE )? sShardTableInfo.mSubKeyColumnName:
                                                               sShardTableInfo.mKeyColumnName ) );
    }
    IDE_EXCEPTION( ERR_NO_SHARD_KEY_COLUMN )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::getProcedureInfo",
                                  "There is no shard key column for the shard procedure" ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsBeginStmt == ID_TRUE )
    {
        (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC sdi::getTableInfo( qcStatement    * aStatement,
                          qcmTableInfo   * aTableInfo,
                          sdiObjectInfo ** aShardObjInfo )
{
    IDE_TEST( sdi::getTableInfo( aStatement,
                                 aTableInfo,
                                 aShardObjInfo, 
                                 ID_TRUE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::validateObjectSMN( qcStatement    * aStatement,
                               sdiObjectInfo * aShardObjInfo )
{
    ULong sDataSMN = SDI_NULL_SMN;

    IDE_TEST( waitAndSetSMNForMetaNode( aStatement->mStatistics,
                                        QC_SMI_STMT( aStatement ),
                                        ( SMI_STATEMENT_NORMAL |
                                        SMI_STATEMENT_SELF_TRUE |
                                        SMI_STATEMENT_ALL_CURSOR ),
                                        SDI_NULL_SMN,
                                        &sDataSMN )
            != IDE_SUCCESS );
    IDE_TEST_RAISE(sDataSMN != aShardObjInfo->mSMN, ERR_SMN_IS_INVALID);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SMN_IS_INVALID )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::validateObjectSMN",
                                  "ERR_SMN_IS_INVALID " ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::getTableInfo( qcStatement    * aStatement,
                          qcmTableInfo   * aTableInfo,
                          sdiObjectInfo ** aShardObjInfo,
                          idBool           aIsRangeMerge )
{
/***********************************************************************
 *
 * Description : PROJ-2598 Shard Meta
 *
 * Implementation :
 *
 ***********************************************************************/

    smiStatement     sSmiStmt;
    idBool           sIsBeginStmt = ID_FALSE;
    sdiTableInfo     sShardTableInfo;
    sdiObjectInfo  * sShardObjInfo = NULL;
    idBool           sExistKey = ID_FALSE;
    idBool           sExistSubKey = ID_FALSE;
    idBool           sIsTableFound = ID_FALSE;
    UInt             sKeyType;
    UInt             sSubKeyType;
    UInt             i = 0;

    /*
     * PROJ-2701 Online data rebuild
     *
     * sSMN[0] : sessionSMN
     * sSMN[1] : dataSMN
     */
    ULong  sSMN[2] = { SDI_NULL_SMN, SDI_NULL_SMN };
    ULong  sSMNForShardObj = SDI_NULL_SMN;

    if ( ( ( qciMisc::isStmtDML( aStatement->myPlan->parseTree->stmtKind ) == ID_TRUE ) ||
           ( qciMisc::isStmtSP( aStatement->myPlan->parseTree->stmtKind ) == ID_TRUE ) ||
           ( qciMisc::isStmtDDL( aStatement->myPlan->parseTree->stmtKind ) == ID_TRUE ))  &&
         ( aTableInfo->tableOwnerID != getShardUserID() ) &&
         ( aTableInfo->tableType == QCM_USER_TABLE ) &&
         ( *aShardObjInfo == NULL ) )
    {
        // PROJ-2701 Online date rebuild
        if ( ( aStatement->session->mQPSpecific.mFlag & QC_SESSION_ALTER_META_MASK )
             == QC_SESSION_ALTER_META_DISABLE )
        {
            sSMN[0] = QCG_GET_SESSION_SHARD_META_NUMBER( aStatement );

            if ( sSMN[0] != SDI_NULL_SMN )
            {
                IDE_TEST( waitAndSetSMNForMetaNode( aStatement->mStatistics,
                                                    QC_SMI_STMT( aStatement ),
                                                    ( SMI_STATEMENT_NORMAL |
                                                      SMI_STATEMENT_SELF_TRUE |
                                                      SMI_STATEMENT_ALL_CURSOR ),
                                                    SDI_NULL_SMN,
                                                    &sSMN[1] )
                          != IDE_SUCCESS );
            }
            else
            {
                // sSMNCount = 0;
                // Sharding을 쓰지 않음.
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do. ( Initial SMN for shard meta creation )
            // sMetaNodeInfo = { ID_ULONG(0) };
        }
        
        // 별도의 stmt를 열어 항상 최신의 view를 본다.
        // (shard meta는 일반 memory table이므로 normal로 열되,
        // 상위 stmt가 untouchable일 수 있으므로 self를 추가한다.)
        IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                                  QC_SMI_STMT( aStatement ),
                                  SMI_STATEMENT_NORMAL |
                                  SMI_STATEMENT_SELF_TRUE |
                                  SMI_STATEMENT_ALL_CURSOR )
                  != IDE_SUCCESS );
        sIsBeginStmt = ID_TRUE;

        // SessionSMN ~ dataSMN 사이의 shardObjInfo를 생성한다.
        for ( sSMNForShardObj = sSMN[0]; sSMNForShardObj <= sSMN[1]; sSMNForShardObj++ )
        {
            if ( sdm::getTableInfo( &sSmiStmt,
                                    aTableInfo->tableOwnerName,
                                    aTableInfo->name,
                                    sSMNForShardObj,
                                    &sShardTableInfo,
                                    &sIsTableFound ) == IDE_SUCCESS )
            {
                if ( sIsTableFound == ID_TRUE )
                {
                    if ( ( sShardTableInfo.mObjectType == 'T' ) &&
                         ( sShardTableInfo.mSplitMethod != SDI_SPLIT_NONE ) )
                    {
                        IDE_TEST_RAISE( ( getSplitType( sShardTableInfo.mSplitMethod )
                                          == SDI_SPLIT_TYPE_DIST ) &&
                                        ( sShardTableInfo.mKeyColumnName[0] == '\0' ),
                                        ERR_NO_SHARD_KEY_COLUMN );

                        // keyFlags를 추가생성한다.
                        IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                                      ID_SIZEOF( sdiObjectInfo ) + aTableInfo->columnCount,
                                      (void**) & sShardObjInfo )
                                  != IDE_SUCCESS );

                        // set shard table info
                        idlOS::memcpy( (void*)&(sShardObjInfo->mTableInfo),
                                       (void*)&sShardTableInfo,
                                       ID_SIZEOF( sdiTableInfo ) );

                        // set key flags
                        idlOS::memset( sShardObjInfo->mKeyFlags, 0x00, aTableInfo->columnCount );

                        // set SMN
                        sShardObjInfo->mSMN = sSMNForShardObj;

                        if ( getSplitType( sShardTableInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
                        {
                            // Hash, Range or List-based sharding 의 shard key column을 찾는다.
                            for ( i = 0; i < aTableInfo->columnCount; i++ )
                            {
                                if ( idlOS::strMatch( aTableInfo->columns[i].name,
                                                      idlOS::strlen(aTableInfo->columns[i].name),
                                                      sShardTableInfo.mKeyColumnName,
                                                      idlOS::strlen(sShardTableInfo.mKeyColumnName) ) == 0 )
                                {
                                    sKeyType = aTableInfo->columns[i].basicInfo->module->id;
                                    IDE_TEST_RAISE( isSupportDataType( sKeyType ) == ID_FALSE,
                                                    ERR_INVALID_SHARD_KEY_TYPE );

                                    sShardObjInfo->mTableInfo.mKeyDataType = sKeyType;
                                    sShardObjInfo->mTableInfo.mKeyColOrder = (UShort)i;
                                    sShardObjInfo->mKeyFlags[i] = 1;
                                    sExistKey = ID_TRUE;

                                    if ( ( sShardTableInfo.mSubKeyExists == ID_FALSE ) ||
                                         ( sExistSubKey == ID_TRUE ) )
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        // Nothing to do.
                                    }
                                }
                                else
                                {
                                    // Nothing to do.
                                }

                                /* PROJ-2655 Composite shard key */
                                if ( sShardTableInfo.mSubKeyExists == ID_TRUE )
                                {   
                                    if ( idlOS::strMatch( aTableInfo->columns[i].name,
                                                          idlOS::strlen(aTableInfo->columns[i].name),
                                                          sShardTableInfo.mSubKeyColumnName,
                                                          idlOS::strlen(sShardTableInfo.mSubKeyColumnName) ) == 0 )
                                    {   
                                        sExistSubKey = ID_TRUE;

                                        sSubKeyType = aTableInfo->columns[i].basicInfo->module->id;
                                        IDE_TEST_RAISE( isSupportDataType( sSubKeyType ) == ID_FALSE,
                                                        ERR_INVALID_SHARD_KEY_TYPE );

                                        sShardObjInfo->mTableInfo.mSubKeyDataType = sSubKeyType;
                                        sShardObjInfo->mTableInfo.mSubKeyColOrder = (UShort)i;
                                        sShardObjInfo->mKeyFlags[i] = 2;

                                        if ( sExistKey == ID_TRUE )
                                        {
                                            break;
                                        }
                                        else
                                        {
                                            // Nothing to do.
                                        }
                                    }
                                    else
                                    {
                                        // Nothing to do.
                                    }
                                }
                                else
                                {
                                    // Nothing to do.
                                }
                            }

                            IDE_TEST_RAISE( sExistKey == ID_FALSE, ERR_NOT_EXIST_SHARD_KEY );

                            IDE_TEST_RAISE( ( ( sShardTableInfo.mSubKeyExists == ID_TRUE ) &&
                                              ( sExistSubKey == ID_FALSE ) ),
                                            ERR_NOT_EXIST_SUB_SHARD_KEY );
                        }
                        else
                        {
                            // Nothing to do.
                        }

                        // set shard range info
                        IDE_TEST( sdm::getRangeInfo( aStatement,
                                                     &sSmiStmt,
                                                     sShardObjInfo->mSMN,
                                                     &(sShardObjInfo->mTableInfo),
                                                     &(sShardObjInfo->mRangeInfo),
                                                     aIsRangeMerge )
                                  != IDE_SUCCESS );

                        if ( sShardObjInfo->mSMN == sSMN[1] )
                        {
                            // default node도 없고 range 정보도 없다면 에러
                            IDE_TEST_RAISE(
                                ( sShardObjInfo->mTableInfo.mDefaultNodeId == SDI_NODE_NULL_ID ) &&
                                ( sShardObjInfo->mRangeInfo.mCount == 0 ),
                                ERR_INCOMPLETE_RANGE_SET );
                        }
                        else
                        {
                            // Nothing to do.
                        }

                        sShardObjInfo->mNext = *aShardObjInfo;
                        *aShardObjInfo = sShardObjInfo;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }
        }

        sIsBeginStmt = ID_FALSE;
        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INCOMPLETE_RANGE_SET )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_INCOMPLETE_RANGE_SET,
                                  aTableInfo->tableOwnerName,
                                  aTableInfo->name ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_SHARD_KEY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_KEY_COLUMN_NOT_EXIST,
                                  aTableInfo->tableOwnerName,
                                  aTableInfo->name,
                                  sShardTableInfo.mKeyColumnName ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_SUB_SHARD_KEY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_KEY_COLUMN_NOT_EXIST,
                                  aTableInfo->tableOwnerName,
                                  aTableInfo->name,
                                  sShardTableInfo.mSubKeyColumnName ) );
    }
    IDE_EXCEPTION( ERR_INVALID_SHARD_KEY_TYPE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_UNSUPPORTED_SHARD_KEY_COLUMN_TYPE,
                                  aTableInfo->tableOwnerName,
                                  aTableInfo->name,
                                  ( sExistSubKey == ID_TRUE )? sShardTableInfo.mSubKeyColumnName:
                                                               sShardTableInfo.mKeyColumnName ) );
    }
    IDE_EXCEPTION( ERR_NO_SHARD_KEY_COLUMN )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::getTableInfo",
                                  "There is no shard key column for the shard table" ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsBeginStmt == ID_TRUE )
    {
        (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

// PROJ-2638
void sdi::initOdbcLibrary()
{
    (void)sdl::initOdbcLibrary();
}

void sdi::finiOdbcLibrary()
{
    sdl::initOdbcLibrary();
}

IDE_RC sdi::allocLinkerParam( sdiClientInfo  * aClientInfo,
                              sdiConnectInfo * aConnectInfo )
{
    UShort           i;
    sdiLinkerParam * sLinkerParam = NULL;;

    IDE_DASSERT( SDI_LINKER_PARAM_IS_USED( aConnectInfo ) == ID_FALSE );
    IDE_TEST_RAISE( SDI_LINKER_PARAM_IS_USED( aConnectInfo ) == ID_TRUE,
                    ErrAlreadyAllocated );

    aConnectInfo->mLinkerParam = NULL;

    for ( i = 0; i < SDI_NODE_MAX_COUNT; ++i )
    {
        sLinkerParam = &aClientInfo->mLinkerParamBuffer[i];

        if ( sLinkerParam->mUsed == SDI_LINKER_PARAM_NOT_USED )
        {
            /* Found unused. */
            sLinkerParam->mUsed = SDI_LINKER_PARAM_USED;
           
            aConnectInfo->mLinkerParam = sLinkerParam;
            break;
        }
    }

    IDE_TEST_RAISE( aConnectInfo->mLinkerParam == NULL,
                    ErrAllocFail );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ErrAlreadyAllocated )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::allocLinkerParam",
                                  "already allocated linker-param" ) );
    }
    IDE_EXCEPTION( ErrAllocFail )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::allocLinkerParam",
                                  "fail to alloc linker-param" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void sdi::freeLinkerParam( sdiConnectInfo * aConnectInfo )
{
    sdiLinkerParam * sLinkerParam = NULL;

    IDE_DASSERT( SDI_LINKER_PARAM_IS_USED( aConnectInfo ) == ID_TRUE );

    sLinkerParam = aConnectInfo->mLinkerParam;

    if ( sLinkerParam != NULL )
    {
        aConnectInfo->mLinkerParam = NULL;

        idlOS::memset( &sLinkerParam->param, 0, ID_SIZEOF(sLinkerParam->param) );

        sLinkerParam->mUsed = SDI_LINKER_PARAM_NOT_USED;
    }
}

void sdi::setLinkerParam( sdiConnectInfo * aConnectInfo )
{
    sdiLinkerParam * sLinkerParam = NULL;

    if ( SDI_LINKER_PARAM_IS_USED( aConnectInfo ) == ID_TRUE )
    {
        sLinkerParam = aConnectInfo->mLinkerParam;

        sLinkerParam->param.mMessageCallback.mFunction = sdi::printMessage;
        sLinkerParam->param.mMessageCallback.mArgument = aConnectInfo;
    }
}

IDE_RC sdi::initializeSession( qcSession * aSession,
                               void      * aDkiSession,
                               smiTrans  * aTrans,
                               idBool      aIsShardMetaChanged,
                               ULong       aConnectSMN )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation : set connect info for data node
 *
 ***********************************************************************/

    sdiNodeInfo       sNodeInfo;
    sdiClientInfo   * sClientInfo       = NULL;
    sdiConnectInfo  * sConnectInfo      = NULL;
    sdiNode         * sDataNode         = NULL;
    static SChar      sNullName[1]      = { '\0' };
    SChar           * sSessionNodeName  = NULL;
    SChar           * sUserName         = NULL;
    static SChar      sNullPassword[1]  = { '\0' };
    SChar           * sUserPassword     = NULL;
    UInt              sPasswordLen      = 0;
    sdiShardPin       sShardPin         = SDI_SHARD_PIN_INVALID;
    UInt              sIsShardClient    = ID_FALSE;
    idBool            sIsUserAutoCommit = ID_TRUE;
    idBool            sIsMetaAutoCommit = ID_TRUE;
    UChar             sPlanAttr         = (UChar)ID_FALSE;
    UShort            i                 = 0;
    sdiCoordinatorType sCoordinatorType = SDI_COORDINATOR_SHARD;
    UInt              sGTXLevel         = 0;
    idBool            sIsGTx            = ID_FALSE;
    UInt              sState            = 0;
    sdiInternalOperation sInternalLocalOperation = SDI_INTERNAL_OP_NOT;

    IDE_DASSERT( aSession->mQPSpecific.mClientInfo == NULL );

    if ( aSession->mMmSession != NULL )
    {
        sSessionNodeName  = qci::mSessionCallback.mGetShardNodeName( aSession->mMmSession );
        sUserName         = qci::mSessionCallback.mGetUserName( aSession->mMmSession );
        sUserPassword     = qci::mSessionCallback.mGetUserPassword( aSession->mMmSession );
        sShardPin         = qci::mSessionCallback.mGetShardPIN( aSession->mMmSession );
        sIsShardClient    = qci::mSessionCallback.mIsShardClient( aSession->mMmSession );
        sPlanAttr         = qci::mSessionCallback.mGetExplainPlan( aSession->mMmSession );
        sIsUserAutoCommit = qci::mSessionCallback.mIsAutoCommit( aSession->mMmSession );
        sIsMetaAutoCommit = sIsUserAutoCommit;
        sGTXLevel         = qci::mSessionCallback.mGetGTXLevel( aSession->mMmSession );
        sIsGTx            = qci::mSessionCallback.mIsGTx( aSession->mMmSession );
        sInternalLocalOperation = (sdiInternalOperation)
            qci::mSessionCallback.mGetShardInternalLocalOperation( aSession->mMmSession );
    }
    else
    {
        sSessionNodeName  = sNullName;
        sUserName         = sNullName;
        sUserPassword     = sNullPassword;
        sPlanAttr         = (UChar)ID_FALSE;
        sIsUserAutoCommit = ID_TRUE;
        sIsMetaAutoCommit = sIsUserAutoCommit;

        IDE_DASSERT( 0 );
    }

    // Get nodeInfo for internal session connectSMN
    IDE_TEST( getInternalNodeInfo( aTrans,
                                   &sNodeInfo,
                                   aIsShardMetaChanged,
                                   aConnectSMN )
              != IDE_SUCCESS );

    if ( sSessionNodeName[0] != '\0' )
    {
        sCoordinatorType = SDI_COORDINATOR_RESHARD;
    }

    if ( sNodeInfo.mCount != 0 )
    {
        IDE_TEST( iduMemMgr::calloc( IDU_MEM_QSN,  // BUGBUG check memory
                                     1,
                                     ID_SIZEOF(sdiClientInfo),
                                     (void **)&sClientInfo )
                  != IDE_SUCCESS );

        sClientInfo->mTargetShardMetaNumber = aConnectSMN;

        /* BUG-45967 Data Node의 Shard Session 정리 */
        sClientInfo->mCount         = sNodeInfo.mCount;

        sClientInfo->mShardCoordFixCtrlCtx.mSession = aSession->mMmSession;
        sClientInfo->mShardCoordFixCtrlCtx.mCount   = 0;
        sClientInfo->mShardCoordFixCtrlCtx.mFunc    = shardCoordFixCtrlCallback;

        sClientInfo->mTransactionLevel = sGTXLevel;

        /* PROJ-2733-DistTxInfo */
        initDistTxInfo( sClientInfo );

        if ( sCoordinatorType == SDI_COORDINATOR_RESHARD )
        {
            sClientInfo->mGCTxInfo.mSessionTypeString = (UChar *)"RESHARD-COORD";
        }
        else
        {
            sClientInfo->mGCTxInfo.mSessionTypeString =
                qci::mSessionCallback.mGetSessionTypeString( aSession->mMmSession );
        }

        sState = 1;

        sConnectInfo = sClientInfo->mConnectInfo;
        sDataNode = sNodeInfo.mNodes;

        for ( i = 0; i < sNodeInfo.mCount; i++, sConnectInfo++, sDataNode++ )
        {
            IDE_DASSERT( sConnectInfo->mLinkerParam == NULL );

            if ( SDI_LINKER_PARAM_IS_USED( sConnectInfo ) == ID_FALSE )
            {
                IDE_TEST( allocLinkerParam( sClientInfo,
                                            sConnectInfo )
                          != IDE_SUCCESS );
            }

            IDE_DASSERT( SDI_LINKER_PARAM_IS_USED( sConnectInfo ) == ID_TRUE )

            setLinkerParam( sConnectInfo );


            // qcSession
            sConnectInfo->mSession = aSession;
            // dkiSession
            sConnectInfo->mDkiSession             = aDkiSession;
            sConnectInfo->mCoordinatorType        = sCoordinatorType;       /* DK 에서 remote tx 생성시 사용 */
            sConnectInfo->mShardPin               = sShardPin;
            sConnectInfo->mIsShardClient          = (sdiShardClient)sIsShardClient;
            sConnectInfo->mShardMetaNumber        = aConnectSMN;
            sConnectInfo->mRebuildShardMetaNumber = aConnectSMN;
            sConnectInfo->mFailoverTarget         = SDI_FAILOVER_NOT_USED;
            sConnectInfo->mFailoverSuspend.mSuspendType  = SDI_FAILOVER_SUSPEND_NONE;
            sConnectInfo->mFailoverSuspend.mNewErrorCode = idERR_IGNORE_NoError;

            sdl::clearInternalConnectResult( sConnectInfo );

            sdi::initAffectedRow( sConnectInfo );

            idlOS::memcpy( &(sConnectInfo->mNodeInfo),
                           sDataNode,
                           ID_SIZEOF( sdiNode ) );

            if ( SDU_SHARD_TEST_ENABLE == 0 )
            {
                idlOS::strncpy( sConnectInfo->mUserName,
                                sUserName,
                                QCI_MAX_OBJECT_NAME_LEN );
                sConnectInfo->mUserName[ QCI_MAX_OBJECT_NAME_LEN ] = '\0';
                idlOS::strncpy( sConnectInfo->mUserPassword,
                                sUserPassword,
                                IDS_MAX_PASSWORD_LEN );
                sConnectInfo->mUserPassword[ IDS_MAX_PASSWORD_LEN ] = '\0';
            }
            else
            {
                idlOS::strncpy( sConnectInfo->mUserName,
                                sDataNode->mNodeName,
                                SDI_NODE_NAME_MAX_SIZE );
                sConnectInfo->mUserName[ SDI_NODE_NAME_MAX_SIZE ] = '\0';
                idlOS::strncpy( sConnectInfo->mUserPassword,
                                sDataNode->mNodeName,
                                SDI_NODE_NAME_MAX_SIZE );
                sConnectInfo->mUserPassword[ SDI_NODE_NAME_MAX_SIZE ] = '\0';

                sPasswordLen = idlOS::strlen( sConnectInfo->mUserPassword );
                sdi::charXOR( sConnectInfo->mUserPassword, sPasswordLen );
            }

            sConnectInfo->mConnectType = sDataNode->mConnectType;

            // node id와 node name은 미리 설정한다.
            sConnectInfo->mNodeId = sDataNode->mNodeId;
            idlOS::strncpy( sConnectInfo->mNodeName,
                            sDataNode->mNodeName,
                            SDI_NODE_NAME_MAX_SIZE );
            sConnectInfo->mNodeName[ SDI_NODE_NAME_MAX_SIZE ] = '\0';

            // connect를 위한 세션정보
            sConnectInfo->mFlag = 0;
            sConnectInfo->mPlanAttr = sPlanAttr;

            // Two Phase Commit인 경우, Meta Connection은 Non-Autocommit이어야 한다.
            if ( ( sIsGTx == ID_TRUE ) && ( sIsMetaAutoCommit == ID_TRUE ) )
            {
                sIsMetaAutoCommit = ID_FALSE;
            }
            else
            {
                /* Nothing to do */
            }

            sConnectInfo->mFlag &= ~SDI_CONNECT_USER_AUTOCOMMIT_MODE_MASK;
            sConnectInfo->mFlag |= ( ( sIsUserAutoCommit == ID_TRUE ) ?
                                     SDI_CONNECT_USER_AUTOCOMMIT_MODE_ON :
                                     SDI_CONNECT_USER_AUTOCOMMIT_MODE_OFF );

            sConnectInfo->mFlag &= ~SDI_CONNECT_COORD_AUTOCOMMIT_MODE_MASK;
            sConnectInfo->mFlag |= ( ( sIsMetaAutoCommit == ID_TRUE ) ?
                                     SDI_CONNECT_COORD_AUTOCOMMIT_MODE_ON :
                                     SDI_CONNECT_COORD_AUTOCOMMIT_MODE_OFF );

            sConnectInfo->mInternalOP = sInternalLocalOperation;

            // xid
            xidInitialize( sConnectInfo );

            sConnectInfo->mCoordPropertyFlag = 0;
        }

        sState = 0;

        aSession->mQPSpecific.mClientInfo = sClientInfo;
    }
    else
    {
        // Nothing to do.
    }

    /* 여기서 부터 에러 발생 금지 */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 1:
            for ( i = 0; i < sNodeInfo.mCount; ++i )
            {
                if ( SDI_LINKER_PARAM_IS_USED( &sClientInfo->mConnectInfo[ i ] ) == ID_TRUE )
                {
                    sdi::freeLinkerParam( &sClientInfo->mConnectInfo[ i ] );
                }
            }
            /* fall through */

        default:
            break;
    }

    if ( sClientInfo != NULL )
    {
        (void)iduMemMgr::free( sClientInfo );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

void sdi::freeClientInfo( qcSession * aSession )
{
    sdiClientInfo * sClientInfo = aSession->mQPSpecific.mClientInfo;

    if ( sClientInfo != NULL )
    {
        (void)iduMemMgr::free( sClientInfo );

        aSession->mQPSpecific.mClientInfo = NULL;
    }
    else
    {
        // Nothing to do.
    }
}

IDE_RC sdi::initializeSessionWithNodeList( qcSession * aSession,
                                           void      * aDkiSession,
                                           smiTrans  * aTrans,
                                           idBool      aIsShardMetaChanged,
                                           ULong       aConnectSMN,
                                           iduList   * aNodeList )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation : set connect info for data node
 *
 ***********************************************************************/

    sdiNodeInfo       sNodeInfo;
    sdiNodeInfo       sAliveNodeInfo;
    sdiClientInfo   * sClientInfo       = NULL;
    sdiConnectInfo  * sConnectInfo      = NULL;
    sdiNode         * sDataNode         = NULL;
    static SChar      sNullName[1]      = { '\0' };
    SChar           * sSessionNodeName  = NULL;
    SChar           * sUserName         = NULL;
    static SChar      sNullPassword[1]  = { '\0' };
    SChar           * sUserPassword     = NULL;
    UInt              sPasswordLen      = 0;
    sdiShardPin       sShardPin         = SDI_SHARD_PIN_INVALID;
    UInt              sIsShardClient    = ID_FALSE;
    idBool            sIsUserAutoCommit = ID_TRUE;
    idBool            sIsMetaAutoCommit = ID_TRUE;
    UChar             sPlanAttr         = (UChar)ID_FALSE;
    UShort            i                 = 0;
    sdiCoordinatorType sCoordinatorType = SDI_COORDINATOR_SHARD;
    UInt              sGTXLevel         = 0;
    idBool            sIsGTx            = ID_FALSE;
    UInt              sState            = 0;
    sdiInternalOperation sInternalLocalOperation = SDI_INTERNAL_OP_NOT;

    IDE_DASSERT( aSession->mQPSpecific.mClientInfo == NULL );

    if ( aSession->mMmSession != NULL )
    {
        sSessionNodeName  = qci::mSessionCallback.mGetShardNodeName( aSession->mMmSession );
        sUserName         = qci::mSessionCallback.mGetUserName( aSession->mMmSession );
        sUserPassword     = qci::mSessionCallback.mGetUserPassword( aSession->mMmSession );
        sShardPin         = qci::mSessionCallback.mGetShardPIN( aSession->mMmSession );
        sIsShardClient    = qci::mSessionCallback.mIsShardClient( aSession->mMmSession );
        sPlanAttr         = qci::mSessionCallback.mGetExplainPlan( aSession->mMmSession );
        sIsUserAutoCommit = qci::mSessionCallback.mIsAutoCommit( aSession->mMmSession );
        sIsMetaAutoCommit = sIsUserAutoCommit;
        sGTXLevel         = qci::mSessionCallback.mGetGTXLevel( aSession->mMmSession );
        sIsGTx            = qci::mSessionCallback.mIsGTx( aSession->mMmSession );
        sInternalLocalOperation = (sdiInternalOperation)
            qci::mSessionCallback.mGetShardInternalLocalOperation( aSession->mMmSession );
    }
    else
    {
        sSessionNodeName  = sNullName;
        sUserName         = sNullName;
        sUserPassword     = sNullPassword;
        sPlanAttr         = (UChar)ID_FALSE;
        sIsUserAutoCommit = ID_TRUE;
        sIsMetaAutoCommit = sIsUserAutoCommit;

        IDE_DASSERT( 0 );
    }

    // Get nodeInfo for internal session connectSMN
    IDE_TEST( getInternalNodeInfo( aTrans,
                                   &sNodeInfo,
                                   aIsShardMetaChanged,
                                   aConnectSMN )
              != IDE_SUCCESS );

    IDE_TEST( filterNodeInfo( &sNodeInfo,
                              &sAliveNodeInfo,
                              aNodeList )
              != IDE_SUCCESS );
    
    if ( sSessionNodeName[0] != '\0' )
    {
        sCoordinatorType = SDI_COORDINATOR_RESHARD;
    }

    if ( sAliveNodeInfo.mCount != 0 )
    {
        IDE_TEST( iduMemMgr::calloc( IDU_MEM_QSN,  // BUGBUG check memory
                                     1,
                                     ID_SIZEOF(sdiClientInfo),
                                     (void **)&sClientInfo )
                  != IDE_SUCCESS );

        sClientInfo->mTargetShardMetaNumber = aConnectSMN;

        /* BUG-45967 Data Node의 Shard Session 정리 */
        sClientInfo->mCount         = sAliveNodeInfo.mCount;

        sClientInfo->mShardCoordFixCtrlCtx.mSession = aSession->mMmSession;
        sClientInfo->mShardCoordFixCtrlCtx.mCount   = 0;
        sClientInfo->mShardCoordFixCtrlCtx.mFunc    = shardCoordFixCtrlCallback;

        /* BUG-46100 Session SMN Update */
        sClientInfo->mTransactionLevel = sGTXLevel;

        /* PROJ-2733-DistTxInfo */
        initDistTxInfo( sClientInfo );

        if ( sCoordinatorType == SDI_COORDINATOR_RESHARD )
        {
            sClientInfo->mGCTxInfo.mSessionTypeString = (UChar *)"RESHARD-COORD";
        }
        else
        {
            sClientInfo->mGCTxInfo.mSessionTypeString =
                qci::mSessionCallback.mGetSessionTypeString( aSession->mMmSession );
        }

        sState = 1;

        sConnectInfo = sClientInfo->mConnectInfo;
        sDataNode = sAliveNodeInfo.mNodes;

        for ( i = 0; i < sAliveNodeInfo.mCount; i++, sConnectInfo++, sDataNode++ )
        {
            IDE_DASSERT( sConnectInfo->mLinkerParam == NULL );

            if ( SDI_LINKER_PARAM_IS_USED( sConnectInfo ) == ID_FALSE )
            {
                IDE_TEST( allocLinkerParam( sClientInfo,
                                            sConnectInfo )
                          != IDE_SUCCESS );
            }

            IDE_DASSERT( SDI_LINKER_PARAM_IS_USED( sConnectInfo ) == ID_TRUE )

            setLinkerParam( sConnectInfo );


            // qcSession
            sConnectInfo->mSession = aSession;
            // dkiSession
            sConnectInfo->mDkiSession             = aDkiSession;
            sConnectInfo->mCoordinatorType        = sCoordinatorType;       /* DK 에서 remote tx 생성시 사용 */
            sConnectInfo->mShardPin               = sShardPin;
            sConnectInfo->mIsShardClient          = (sdiShardClient)sIsShardClient;
            sConnectInfo->mShardMetaNumber        = aConnectSMN;
            sConnectInfo->mRebuildShardMetaNumber = aConnectSMN;
            sConnectInfo->mFailoverTarget         = SDI_FAILOVER_NOT_USED;
            sConnectInfo->mFailoverSuspend.mSuspendType  = SDI_FAILOVER_SUSPEND_NONE;
            sConnectInfo->mFailoverSuspend.mNewErrorCode = idERR_IGNORE_NoError;

            sdl::clearInternalConnectResult( sConnectInfo );

            sdi::initAffectedRow( sConnectInfo );

            idlOS::memcpy( &(sConnectInfo->mNodeInfo),
                           sDataNode,
                           ID_SIZEOF( sdiNode ) );

            if ( SDU_SHARD_TEST_ENABLE == 0 )
            {
                idlOS::strncpy( sConnectInfo->mUserName,
                                sUserName,
                                QCI_MAX_OBJECT_NAME_LEN );
                sConnectInfo->mUserName[ QCI_MAX_OBJECT_NAME_LEN ] = '\0';
                idlOS::strncpy( sConnectInfo->mUserPassword,
                                sUserPassword,
                                IDS_MAX_PASSWORD_LEN );
                sConnectInfo->mUserPassword[ IDS_MAX_PASSWORD_LEN ] = '\0';
            }
            else
            {
                idlOS::strncpy( sConnectInfo->mUserName,
                                sDataNode->mNodeName,
                                SDI_NODE_NAME_MAX_SIZE );
                sConnectInfo->mUserName[ SDI_NODE_NAME_MAX_SIZE ] = '\0';
                idlOS::strncpy( sConnectInfo->mUserPassword,
                                sDataNode->mNodeName,
                                SDI_NODE_NAME_MAX_SIZE );
                sConnectInfo->mUserPassword[ SDI_NODE_NAME_MAX_SIZE ] = '\0';

                sPasswordLen = idlOS::strlen( sConnectInfo->mUserPassword );
                sdi::charXOR( sConnectInfo->mUserPassword, sPasswordLen );
            }

            sConnectInfo->mConnectType = sDataNode->mConnectType;

            // node id와 node name은 미리 설정한다.
            sConnectInfo->mNodeId = sDataNode->mNodeId;
            idlOS::strncpy( sConnectInfo->mNodeName,
                            sDataNode->mNodeName,
                            SDI_NODE_NAME_MAX_SIZE );
            sConnectInfo->mNodeName[ SDI_NODE_NAME_MAX_SIZE ] = '\0';

            // connect를 위한 세션정보
            sConnectInfo->mFlag = 0;
            sConnectInfo->mPlanAttr = sPlanAttr;

            // Two Phase Commit인 경우, Meta Connection은 Non-Autocommit이어야 한다.
            if ( ( sIsGTx == ID_TRUE ) && ( sIsMetaAutoCommit == ID_TRUE ) )
            {
                sIsMetaAutoCommit = ID_FALSE;
            }
            else
            {
                /* Nothing to do */
            }

            sConnectInfo->mFlag &= ~SDI_CONNECT_USER_AUTOCOMMIT_MODE_MASK;
            sConnectInfo->mFlag |= ( ( sIsUserAutoCommit == ID_TRUE ) ?
                                     SDI_CONNECT_USER_AUTOCOMMIT_MODE_ON :
                                     SDI_CONNECT_USER_AUTOCOMMIT_MODE_OFF );

            sConnectInfo->mFlag &= ~SDI_CONNECT_COORD_AUTOCOMMIT_MODE_MASK;
            sConnectInfo->mFlag |= ( ( sIsMetaAutoCommit == ID_TRUE ) ?
                                     SDI_CONNECT_COORD_AUTOCOMMIT_MODE_ON :
                                     SDI_CONNECT_COORD_AUTOCOMMIT_MODE_OFF );

            sConnectInfo->mInternalOP = sInternalLocalOperation;

            // xid
            xidInitialize( sConnectInfo );

            sConnectInfo->mCoordPropertyFlag = 0;
        }

        sState = 0;

        aSession->mQPSpecific.mClientInfo = sClientInfo;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 1:
            for ( i = 0; i < sAliveNodeInfo.mCount; ++i )
            {
                if ( SDI_LINKER_PARAM_IS_USED( &sClientInfo->mConnectInfo[ i ] ) == ID_TRUE )
                {
                    sdi::freeLinkerParam( &sClientInfo->mConnectInfo[ i ] );
                }
            }
            /* fall through */

        default:
            break;
    }

    if ( sClientInfo != NULL )
    {
        (void)iduMemMgr::free( sClientInfo );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}


void sdi::finalizeSession( qcSession * aSession )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation : disconnect & freeConnect for data node
 *
 ***********************************************************************/

    sdiClientInfo  * sClientInfo = NULL;
    sdiConnectInfo * sConnectInfo = NULL;
    UShort           i = 0;

    IDE_DASSERT( aSession != NULL );

    sClientInfo = aSession->mQPSpecific.mClientInfo;

    if ( sClientInfo != NULL )
    {
        IDE_DASSERT( sClientInfo->mShardCoordFixCtrlCtx.mCount == 0 );

        sConnectInfo = sClientInfo->mConnectInfo;

        for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
        {
            /* Close shard connection. */
            qdkCloseShardConnection( sConnectInfo ); 

            if ( SDI_LINKER_PARAM_IS_USED( sConnectInfo ) == ID_TRUE )
            {
                sdi::freeLinkerParam( sConnectInfo );
            }
        }

        freeClientInfo( aSession );
    }
    else
    {
        // Nothing to do.
    }
}

IDE_RC sdi::updateSession( qcSession * aSession,
                           void      * aDkiSession,
                           smiTrans  * aTrans,
                           idBool      aIsShardMetaChanged,
                           ULong       aNewSMN )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation : Merge connect info for data node
 *                  and merge current SMN and NEW SMN
 *
 ***********************************************************************/

    sdiNodeInfo          sNodeInfo4Added;
    sdiNodeInfo          sNodeInfo4NewSMN;
    sdiClientInfo      * sClientInfo       = NULL;
    sdiConnectInfo     * sConnectInfo      = NULL;
    sdiNode            * sDataNode         = NULL;
    static SChar         sNullName[1]      = { '\0' };
    SChar              * sSessionNodeName  = NULL;
    SChar              * sUserName         = NULL;
    static SChar         sNullPassword[1]  = { '\0' };
    SChar              * sUserPassword     = NULL;
    UInt                 sPasswordLen      = 0;
    sdiShardPin          sShardPin         = SDI_SHARD_PIN_INVALID;
    UInt                 sIsShardClient    = ID_FALSE;
    idBool               sIsUserAutoCommit = ID_TRUE;
    idBool               sIsMetaAutoCommit = ID_TRUE;
    UChar                sPlanAttr         = (UChar)ID_FALSE;
    UShort               i                 = 0;
    UShort               j                 = 0;
    sdiCoordinatorType   sCoordinatorType  = SDI_COORDINATOR_SHARD;
    idBool               sIsGTx            = ID_FALSE;
    UShort               sNewNodeCount     = 0;
    UInt                 sState            = 0;

    sClientInfo = aSession->mQPSpecific.mClientInfo;
    IDE_DASSERT( sClientInfo != NULL );

    IDE_TEST_CONT( sClientInfo->mTargetShardMetaNumber == aNewSMN,
                   END_OF_FUNC );

    // Get nodeInfo for internal session connectSMN
    IDE_TEST( getInternalNodeInfo( aTrans,
                                   &sNodeInfo4NewSMN,
                                   aIsShardMetaChanged,
                                   aNewSMN )
              != IDE_SUCCESS );

    sNodeInfo4Added.mCount = 0;

    /*     Old Node Info (Current) : 1 2 3
     *     New Node Info           :   2   4 5
     * ---------------------------------------
     *   Added Node Info          =>       4 5
     */
    IDE_TEST( getAddedNodeInfo( sClientInfo,
                                &sNodeInfo4NewSMN,
                                &sNodeInfo4Added )
              != IDE_SUCCESS );


    sNewNodeCount = sClientInfo->mCount + sNodeInfo4Added.mCount;

    if ( sNodeInfo4Added.mCount != 0 )
    {
        if ( aSession->mMmSession != NULL )
        {
            sSessionNodeName  = qci::mSessionCallback.mGetShardNodeName( aSession->mMmSession );
            sUserName         = qci::mSessionCallback.mGetUserName( aSession->mMmSession );
            sUserPassword     = qci::mSessionCallback.mGetUserPassword( aSession->mMmSession );
            sShardPin         = qci::mSessionCallback.mGetShardPIN( aSession->mMmSession );
            sIsShardClient    = qci::mSessionCallback.mIsShardClient( aSession->mMmSession );
            sPlanAttr         = qci::mSessionCallback.mGetExplainPlan( aSession->mMmSession );
            sIsUserAutoCommit = qci::mSessionCallback.mIsAutoCommit( aSession->mMmSession );
            sIsMetaAutoCommit = sIsUserAutoCommit;
            sIsGTx            = qci::mSessionCallback.mIsGTx( aSession->mMmSession );
        }
        else
        {
            sSessionNodeName  = sNullName;
            sUserName         = sNullName;
            sUserPassword     = sNullPassword;
            sPlanAttr         = (UChar)ID_FALSE;
            sIsUserAutoCommit = ID_TRUE;
            sIsMetaAutoCommit = sIsUserAutoCommit;

            IDE_DASSERT( 0 );
        }

        if ( sSessionNodeName[0] != '\0' )
        {
            sCoordinatorType = SDI_COORDINATOR_RESHARD;
        }

        IDE_DASSERT( sNewNodeCount > sClientInfo->mCount );

        /* set Added node info */
        sState = 1;

        i = sClientInfo->mCount;
        sConnectInfo = &sClientInfo->mConnectInfo[ i ];
        sDataNode = sNodeInfo4Added.mNodes;

        for ( ; i < sNewNodeCount; ++i, ++sConnectInfo, ++sDataNode )
        {
            IDE_DASSERT( sConnectInfo->mLinkerParam == NULL );

            if ( SDI_LINKER_PARAM_IS_USED( sConnectInfo ) == ID_FALSE )
            {
                IDE_TEST( allocLinkerParam( sClientInfo,
                                            sConnectInfo )
                          != IDE_SUCCESS );
            }

            IDE_DASSERT( SDI_LINKER_PARAM_IS_USED( sConnectInfo ) == ID_TRUE )

            setLinkerParam( sConnectInfo );


            // qcSession
            sConnectInfo->mSession = aSession;
            // dkiSession
            sConnectInfo->mDkiSession                    = aDkiSession;
            sConnectInfo->mCoordinatorType               = sCoordinatorType;       /* DK 에서 remote tx 생성시 사용 */
            sConnectInfo->mShardPin                      = sShardPin;
            sConnectInfo->mIsShardClient                 = (sdiShardClient)sIsShardClient;
            sConnectInfo->mShardMetaNumber               = aNewSMN;
            sConnectInfo->mRebuildShardMetaNumber        = aNewSMN;
            sConnectInfo->mFailoverTarget                = SDI_FAILOVER_NOT_USED;
            sConnectInfo->mFailoverSuspend.mSuspendType  = SDI_FAILOVER_SUSPEND_NONE;
            sConnectInfo->mFailoverSuspend.mNewErrorCode = idERR_IGNORE_NoError;

            sdl::clearInternalConnectResult( sConnectInfo );

            sdi::initAffectedRow( sConnectInfo );

            idlOS::memcpy( &(sConnectInfo->mNodeInfo),
                           sDataNode,
                           ID_SIZEOF( sdiNode ) );

            if ( SDU_SHARD_TEST_ENABLE == 0 )
            {
                idlOS::strncpy( sConnectInfo->mUserName,
                                sUserName,
                                QCI_MAX_OBJECT_NAME_LEN );
                sConnectInfo->mUserName[ QCI_MAX_OBJECT_NAME_LEN ] = '\0';
                idlOS::strncpy( sConnectInfo->mUserPassword,
                                sUserPassword,
                                IDS_MAX_PASSWORD_LEN );
                sConnectInfo->mUserPassword[ IDS_MAX_PASSWORD_LEN ] = '\0';
            }
            else
            {
                idlOS::strncpy( sConnectInfo->mUserName,
                                sDataNode->mNodeName,
                                SDI_NODE_NAME_MAX_SIZE );
                sConnectInfo->mUserName[ SDI_NODE_NAME_MAX_SIZE ] = '\0';
                idlOS::strncpy( sConnectInfo->mUserPassword,
                                sDataNode->mNodeName,
                                SDI_NODE_NAME_MAX_SIZE );
                sConnectInfo->mUserPassword[ SDI_NODE_NAME_MAX_SIZE ] = '\0';

                sPasswordLen = idlOS::strlen( sConnectInfo->mUserPassword );
                sdi::charXOR( sConnectInfo->mUserPassword, sPasswordLen );
            }

            sConnectInfo->mConnectType = sDataNode->mConnectType;

            // node id와 node name은 미리 설정한다.
            sConnectInfo->mNodeId = sDataNode->mNodeId;
            idlOS::strncpy( sConnectInfo->mNodeName,
                            sDataNode->mNodeName,
                            SDI_NODE_NAME_MAX_SIZE );
            sConnectInfo->mNodeName[ SDI_NODE_NAME_MAX_SIZE ] = '\0';

            // connect를 위한 세션정보
            sConnectInfo->mPlanAttr = sPlanAttr;

            // Two Phase Commit인 경우, Meta Connection은 Non-Autocommit이어야 한다.
            if ( ( sIsGTx == ID_TRUE ) && ( sIsMetaAutoCommit == ID_TRUE ) )
            {
                sIsMetaAutoCommit = ID_FALSE;
            }
            else
            {
                /* Nothing to do */
            }

            sConnectInfo->mFlag &= ~SDI_CONNECT_USER_AUTOCOMMIT_MODE_MASK;
            sConnectInfo->mFlag |= ( ( sIsUserAutoCommit == ID_TRUE ) ?
                                     SDI_CONNECT_USER_AUTOCOMMIT_MODE_ON :
                                     SDI_CONNECT_USER_AUTOCOMMIT_MODE_OFF );

            sConnectInfo->mFlag &= ~SDI_CONNECT_COORD_AUTOCOMMIT_MODE_MASK;
            sConnectInfo->mFlag |= ( ( sIsMetaAutoCommit == ID_TRUE ) ?
                                     SDI_CONNECT_COORD_AUTOCOMMIT_MODE_ON :
                                     SDI_CONNECT_COORD_AUTOCOMMIT_MODE_OFF );
            // xid
            xidInitialize( sConnectInfo );

            sConnectInfo->mCoordPropertyFlag = 0;
        }

        /* 여기서 부터 에러 발생 금지 */
        sState = 0;

        sClientInfo->mCount = sNewNodeCount;
    }


    /* 여기서 부터 에러 발생 금지 */

    for ( j = 0 ; j < sNodeInfo4NewSMN.mCount; j++ )
    {
        sConnectInfo = sClientInfo->mConnectInfo;
        for ( i = 0 ; i < sClientInfo->mCount; i++, sConnectInfo++ )
        {
            if ( sNodeInfo4NewSMN.mNodes[j].mNodeId ==
                 sConnectInfo->mNodeInfo.mNodeId )
            {
                sConnectInfo->mNodeInfo.mSMN = aNewSMN;
                break;
            }
        }
    }

    sClientInfo->mTargetShardMetaNumber = aNewSMN;

    IDE_EXCEPTION_CONT( END_OF_FUNC );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 1:
            i = sClientInfo->mCount;
            for ( ; i < sNewNodeCount; ++i )
            {
                if ( SDI_LINKER_PARAM_IS_USED( &sClientInfo->mConnectInfo[ i ] ) == ID_TRUE )
                {
                    sdi::freeLinkerParam( &sClientInfo->mConnectInfo[ i ] );
                }
            }
            /* fall through */

        default:
            break;
    }

    return IDE_FAILURE;
}

void sdi::cleanupShardRebuildSession( qcSession * aSession, idBool * aRemoved )
{
    sdiClientInfo  * sClientInfo    = NULL;
    sdiConnectInfo * sConnectInfo   = NULL;
    ULong            sTargetSMN     = SDI_NULL_SMN; 
    SInt             i, j;

    *aRemoved = ID_FALSE;

    sClientInfo = aSession->mQPSpecific.mClientInfo;

    if ( sClientInfo != NULL )
    {
        sTargetSMN = sClientInfo->mTargetShardMetaNumber;

        sConnectInfo = sClientInfo->mConnectInfo;

        for ( i = 0 ; i < sClientInfo->mCount; )
        {
            if ( sClientInfo->mConnectInfo[ i ].mNodeInfo.mSMN == sTargetSMN )
            {
                IDE_DASSERT( SDI_LINKER_PARAM_IS_USED( sConnectInfo ) == ID_TRUE )

                if ( SDI_LINKER_PARAM_IS_USED( sConnectInfo ) == ID_TRUE )
                {
                    setLinkerParam( sConnectInfo );
                }

                ++i;
                ++sConnectInfo;
            }
            else
            {
                /* PROJ-2745
                 *  노드가 제거되면서 mConnectInfo 에 기존에 사용하던 정보가 남아있게되면
                 *  이후 Rebuild 에 의해 호출되는 sdi::updateSession 에서
                 *  Garbage 정보를 보게 된다.
                 *  mClientInfo 와 mConnectInfo 는 calloc 으로 생성하여
                 *  초기화가 되어 있다는 가정으로 사용하고 있기 때문에
                 *  노드 제거 후에 memset 으로 초기화 해준다.
                 *  유지보수를 용이하게 위한 코드이며 실용적이지 못하다.
                 *
                 *  mConnectInfo:
                 *   Remove Node: NODE1
                 *   mCount = 3
                 *  +-----------------------------------------+
                 *  | NODE1 | NODE2 | NODE3 | ZERO | ZERO ... |
                 *  +-----------------------------------------+
                 */

                /* Close shard connection. */
                qdkCloseShardConnection( sConnectInfo ); 

                IDE_DASSERT ( SDI_LINKER_PARAM_IS_USED( sConnectInfo ) == ID_TRUE );
                if ( SDI_LINKER_PARAM_IS_USED( sConnectInfo ) == ID_TRUE )
                {
                    sdi::freeLinkerParam( sConnectInfo );
                }

                --sClientInfo->mCount;

                for ( j = i; j < sClientInfo->mCount; j++ )
                {
                    idlOS::memcpy( &sClientInfo->mConnectInfo[ j ],
                                   &sClientInfo->mConnectInfo[ j + 1 ],
                                   ID_SIZEOF( sdiConnectInfo ) );
                }

                /* 
                 *   mCount = 2
                 *  +-----------------------------------------+
                 *  | NODE2 | NODE3 | NODE3 | ZERO | ZERO ... |
                 *  +-----------------------------------------+
                 */

                IDE_DASSERT( j == sClientInfo->mCount );

                idlOS::memset( &sClientInfo->mConnectInfo[ j ],
                               0,
                               ID_SIZEOF( sdiConnectInfo ) );

                /* 
                 *   mCount = 2
                 *  +-----------------------------------------+
                 *  | NODE2 | NODE3 | ZERO  | ZERO | ZERO ... |
                 *  +-----------------------------------------+
                 */
                *aRemoved = ID_TRUE;
            }
        }

        if ( sClientInfo->mCount == 0 )
        {
            freeClientInfo( aSession );
        }
    }
}

IDE_RC sdi::allocConnect( sdiConnectInfo * aConnectInfo )
{
    if ( aConnectInfo->mDbc == NULL )
    {
        IDE_TEST( sdl::allocConnect( aConnectInfo ) != IDE_SUCCESS );

        aConnectInfo->mFlag &= ~SDI_CONNECT_PLANATTR_CHANGE_MASK;
        aConnectInfo->mFlag |= SDI_CONNECT_PLANATTR_CHANGE_FALSE;

        // xid info 초기화
        xidInitialize( aConnectInfo );
    }
    else
    {
        // 이전 connection을 재사용하는 경우
        if ( ( aConnectInfo->mFlag & SDI_CONNECT_TRANSACTION_END_MASK )
             == SDI_CONNECT_TRANSACTION_END_TRUE )
        {
            aConnectInfo->mFlag &= ~SDI_CONNECT_TRANSACTION_END_MASK;
            aConnectInfo->mFlag |= SDI_CONNECT_TRANSACTION_END_FALSE;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    freeConnectImmediately( aConnectInfo );

    return IDE_FAILURE;
}

void sdi::freeConnectImmediately( sdiConnectInfo * aConnectInfo )
{
    if ( aConnectInfo->mDbc != NULL )
    {
        (void)sdl::disconnectLocal( aConnectInfo );

        if ( aConnectInfo->mSession != NULL )
        {
            if ( aConnectInfo->mSession->mMmSession != NULL )
            {
                qci::mSessionCallback.mFreeShardStmt( aConnectInfo->mSession->mMmSession,
                                                      aConnectInfo->mNodeInfo.mNodeId,
                                                      SQL_DROP );
            }
        }

        (void)sdl::freeConnect( aConnectInfo );

        aConnectInfo->mDbc = NULL;
        aConnectInfo->mLinkFailure = ID_FALSE;
    }
    else
    {
        // Nothing to do.
    }
}

IDE_RC sdi::checkNode( sdiConnectInfo * aConnectInfo )
{
    IDE_TEST_RAISE( aConnectInfo->mDbc == NULL, ERR_NULL_DBC );

    IDE_TEST( sdl::checkNode( aConnectInfo,
                              &(aConnectInfo->mLinkFailure) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_DBC )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_EXECUTE_NULL_DBC,
                                  aConnectInfo->mNodeName,
                                  "checkNode" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool sdi::checkNeedFailover( sdiConnectInfo * aConnectInfo )
{
    idBool  sNeedFailover;

    IDE_TEST_CONT( aConnectInfo->mDbc == NULL, SET_FALSE );

    IDE_TEST_CONT( sdl::checkNeedFailover( aConnectInfo,
                                           &sNeedFailover )
                   != IDE_SUCCESS, SET_FALSE );

    return sNeedFailover;

    IDE_EXCEPTION_CONT( SET_FALSE );

    return ID_FALSE;
}

IDE_RC sdi::endPendingTran( sdiConnectInfo * aConnectInfo,
                            idBool           aIsCommit )
{
    IDE_TEST_RAISE( aConnectInfo->mDbc == NULL, ERR_NULL_DBC );

    IDE_TEST( sdl::endPendingTran( aConnectInfo,
                                   &(aConnectInfo->mXID),
                                   aIsCommit,
                                   &(aConnectInfo->mLinkFailure) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_DBC )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_EXECUTE_NULL_DBC,
                                  aConnectInfo->mNodeName,
                                  "commit pending" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::commit( sdiConnectInfo * aConnectInfo )
{
    IDE_TEST_RAISE( aConnectInfo->mDbc == NULL, ERR_NULL_DBC );

    IDE_TEST( sdl::commit( aConnectInfo,
                           aConnectInfo->mSession->mQPSpecific.mClientInfo,
                           &(aConnectInfo->mLinkFailure) )
              != IDE_SUCCESS );

    aConnectInfo->mTouchCount = 0;
    aConnectInfo->mFlag &= ~SDI_CONNECT_TRANSACTION_END_MASK;
    aConnectInfo->mFlag |= SDI_CONNECT_TRANSACTION_END_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_DBC )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_EXECUTE_NULL_DBC,
                                  aConnectInfo->mNodeName,
                                  "commit" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::rollback( sdiConnectInfo * aConnectInfo,
                      const SChar    * aSavepoint )
{
    sdlRemoteStmt sRemoteStmt;

    sdlStatementManager::initRemoteStmt( &sRemoteStmt );

    IDE_TEST_RAISE( aConnectInfo->mDbc == NULL, ERR_NULL_DBC );

    IDE_TEST( sdl::rollback( aConnectInfo,
                             aSavepoint,
                             aConnectInfo->mSession->mQPSpecific.mClientInfo,
                             &(aConnectInfo->mLinkFailure) )
              != IDE_SUCCESS );

    aConnectInfo->mTouchCount = 0;
    aConnectInfo->mFlag &= ~SDI_CONNECT_TRANSACTION_END_MASK;
    aConnectInfo->mFlag |= SDI_CONNECT_TRANSACTION_END_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_DBC )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_EXECUTE_NULL_DBC,
                                  aConnectInfo->mNodeName,
                                  "rollback" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::savepoint( sdiConnectInfo * aConnectInfo,
                       const SChar    * aSavepoint )
{
    IDE_TEST_RAISE( aConnectInfo->mDbc == NULL, ERR_NULL_DBC );

    IDE_TEST( sdl::setSavepoint( aConnectInfo,
                                 aSavepoint,
                                 &(aConnectInfo->mLinkFailure) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_DBC )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_EXECUTE_NULL_DBC,
                                  aConnectInfo->mNodeName,
                                  "savepoint" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void sdi::setExplainPlanAttr( qcSession * aSession,
                              UChar       aValue )
{
    sdiClientInfo    * sClientInfo = NULL;
    sdiConnectInfo   * sConnectInfo = NULL;
    UShort             i = 0;

    IDE_DASSERT( aSession != NULL );

    sClientInfo = aSession->mQPSpecific.mClientInfo;

    if ( sClientInfo != NULL )
    {
        sConnectInfo = sClientInfo->mConnectInfo;

        for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
        {
            if ( sConnectInfo->mPlanAttr != aValue )
            {
                sConnectInfo->mPlanAttr = aValue;

                sConnectInfo->mFlag &= ~SDI_CONNECT_PLANATTR_CHANGE_MASK;
                sConnectInfo->mFlag |= SDI_CONNECT_PLANATTR_CHANGE_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }
}

IDE_RC sdi::setInternalOp( sdiInternalOperation   aValue, 
                           sdiConnectInfo       * aConnectInfo, 
                           idBool               * aOutIsLinkFailure )
{
    if ( aValue != SDI_INTERNAL_OP_NOT )
    {
        IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                    SDL_ALTIBASE_SHARD_INTERNAL_LOCAL_OPERATION,
                                    aValue,
                                    NULL,
                                    aOutIsLinkFailure )
                        != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sdl::setConnAttr( aConnectInfo,
                            SDL_ALTIBASE_SHARD_INTERNAL_LOCAL_OPERATION,
                            0,
                            NULL,
                            aOutIsLinkFailure )
                != IDE_SUCCESS );
    }
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::shardExecDirectNested( qcStatement          * aStatement,
                                   SChar                * aNodeName,
                                   SChar                * aQuery,
                                   UInt                   aQueryLen,
                                   sdiInternalOperation   aInternalLocalOperation,
                                   UInt                 * aExecCount )
{
    sdiStatement   * sSdStmt      = NULL;
    sdiClientInfo  * sClientInfo  = NULL;
    sdiConnectInfo * sConnectInfo = NULL;
    SShort           i = 0;
    idBool           sSuccess = ID_TRUE;
    void           * sCallback    = NULL;

    /* PROJ-2733-DistTxInfo */
    sdiDataNodes     sDataInfo;
    sdiDataNode    * sDataNode = sDataInfo.mNodes;
    smSCN            sSCN;
    idBool           sNeedUpdateSCN = ID_FALSE;
    idBool           sDoingRemoteStmtInitStep = ID_FALSE;

    SM_INIT_SCN( &sSCN );

    IDE_DASSERT( aStatement->session != NULL );

    sClientInfo = aStatement->session->mQPSpecific.mClientInfo;

    *aExecCount = 0;

    sSdStmt = (sdiStatement *)qci::mSessionCallback.mGetShardStmt( QC_MM_STMT( aStatement ) );

    IDE_TEST_CONT( sClientInfo == NULL, NO_NEED_WORK );

    sConnectInfo = sClientInfo->mConnectInfo;
    idlOS::memset( &sDataInfo, 0x00, ID_SIZEOF(sdiDataNodes) );

    /* PROJ-2733-DistTxInfo 수행할 노드를 선택 */
    IDE_TEST( buildDataNodes( sClientInfo, &sDataInfo, aNodeName, NULL ) != IDE_SUCCESS );

    /* 선택된 노드들은 분산정보를 위해 Execute 전에 모두 접속해 둔다. */
    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sDataNode->mState != SDI_NODE_STATE_EXECUTE_SELECTED )  /* PROJ-2733-DistTxInfo */
        {
            continue;
        }

        sNeedUpdateSCN = (sConnectInfo->mDbc == NULL) ? ID_TRUE : ID_FALSE;

        // open shard connection
        IDE_TEST( qdkOpenShardConnection( sConnectInfo ) != IDE_SUCCESS );

        /* PROJ-2733-DistTxInfo Connection 이후 각 노드 SCN을 얻어 온다. */
        if ( sNeedUpdateSCN == ID_TRUE )
        {
            IDE_TEST( sdl::getSCN( sConnectInfo, &sSCN ) != IDE_SUCCESS );

            SM_SET_MAX_SCN( &(sClientInfo->mGCTxInfo.mCoordSCN), &sSCN );
        }

        // PROJ-2727
        IDE_TEST( setShardSessionProperty( aStatement->session,
                                           sClientInfo,
                                           sConnectInfo )
                  != IDE_SUCCESS );
    }

    /* PROJ-2733-DistTxInfo 분산정보를 계산한다. */
    calculateGCTxInfo( QC_PRIVATE_TMPLATE( aStatement ),
                       &sDataInfo,
                       ID_FALSE,
                       SDI_COORD_PLAN_INDEX_NONE );

    /* PROJ-2733-DistTxInfo Node에 분산정보를 전파한다. */
    IDE_TEST( propagateDistTxInfoToNodes( aStatement, sClientInfo, &sDataInfo ) != IDE_SUCCESS );

    sConnectInfo = sClientInfo->mConnectInfo;
    sDataNode = sDataInfo.mNodes;
    sDoingRemoteStmtInitStep = ID_TRUE;
    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sDataNode->mState != SDI_NODE_STATE_EXECUTE_SELECTED )  /* PROJ-2733-DistTxInfo */
        {
            continue;
        }

        /* TASK-7219 Non-shard DML */
        IDE_TEST( sdl::setStmtExecSeq( sConnectInfo, QCG_GET_SESSION_STMT_EXEC_SEQ_FOR_SHARD_TX(aStatement) ) != IDE_SUCCESS );

        IDE_TEST( sdlStatementManager::allocRemoteStatement( sSdStmt,
                                                             sConnectInfo->mNodeId,
                                                             &sDataNode->mRemoteStmt )
                  != IDE_SUCCESS );

        if ( aInternalLocalOperation != SDI_INTERNAL_OP_NOT )
        {
            IDE_TEST(setInternalOp(aInternalLocalOperation,
                                   sConnectInfo,
                                   &(sConnectInfo->mLinkFailure))
                     != IDE_SUCCESS);
        }

        IDE_TEST( sdl::allocStmt( sConnectInfo,
                                  sDataNode->mRemoteStmt,
                                  SDI_SHARD_PARTIAL_EXEC_TYPE_NONE, /* sPartialCoordExecType */
                                  &(sConnectInfo->mLinkFailure) )
                  != IDE_SUCCESS );
                      
        // add shard transaction
        IDE_TEST( qdkAddShardTransaction( aStatement->mStatistics,
                                          QC_SMI_STMT(aStatement)->mTrans->getTransID(),
                                          sClientInfo,
                                          sConnectInfo )
                  != IDE_SUCCESS );

        IDE_TEST( sdl::addExecDirectCallback( &sCallback,
                                              i,
                                              sConnectInfo,
                                              sDataNode->mRemoteStmt,
                                              aQuery,
                                              aQueryLen,
                                              &(sConnectInfo->mLinkFailure) )
                  != IDE_SUCCESS );
    }
    IDE_TEST_CONT( sCallback == NULL, NO_NEED_WORK );
    sdl::doCallback( sCallback );
    // add shard tx 순서의 반대로 del shard tx를 수행해야한다.
    for ( i--, sConnectInfo--, sDataNode--;
          i >= 0;
          i--, sConnectInfo--, sDataNode-- )
    {
        if ( sDataNode->mState == SDI_NODE_STATE_EXECUTE_SELECTED )
        {
            // 수행전 touch count 증가
            sConnectInfo->mTouchCount++;
            if ( sdl::resultCallback( sCallback,
                                      i,
                                      sConnectInfo,
                                      ID_FALSE,
                                      sClientInfo,
                                      SQL_HANDLE_STMT,
                                      sDataNode->mRemoteStmt->mStmt,
                                      (SChar*)"SQLExecDirect",
                                      &(sConnectInfo->mLinkFailure) )
                 == IDE_SUCCESS )
            {
                sDataNode->mState = SDI_NODE_STATE_EXECUTED;
                sDataNode->mExecCount++;
                (*aExecCount)++;
            }
            else
            {
                sDataNode->mState = SDI_NODE_STATE_EXECUTE_CANDIDATED;
                sSuccess = ID_FALSE;
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    sConnectInfo = sClientInfo->mConnectInfo;
    sDataNode = sDataInfo.mNodes;
    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sDataNode->mRemoteStmt != NULL )
        {
            if ( sDataNode->mRemoteStmt->mStmt != NULL )
            {
                if ( sdl::freeStmt( sConnectInfo,
                                    sDataNode->mRemoteStmt,
                                    SQL_DROP,
                                    &(sConnectInfo->mLinkFailure) )
                     != IDE_SUCCESS )
                {
                    // 수행이 실패한 경우
                    sSuccess = ID_FALSE;
                }
            }
            sdlStatementManager::setUnused( &sDataNode->mRemoteStmt );
        }
        if ( ( aInternalLocalOperation != SDI_INTERNAL_OP_NOT ) &&
             ( sConnectInfo->mLinkFailure != ID_TRUE ) &&
             ( sConnectInfo->mDbc != NULL ) )
        {
            IDE_TEST(setInternalOp( SDI_INTERNAL_OP_NOT,
                                    sConnectInfo,
                                    &(sConnectInfo->mLinkFailure))
                     != IDE_SUCCESS);
            ideLog::log(IDE_SD_17,"[SHARD_META] Internal Remote SQL: %s", aQuery);
        }
    }
    sDoingRemoteStmtInitStep = ID_FALSE;

    /* PROJ-2733-DistTxInfo
     * !!! sdl::execDirect 함수를 수행했으면 sSuccess 값에 상관없이
     *     updateMaxNodeSCNToCoordSCN 함수를 수행해야 한다
     */
    IDE_TEST( updateMaxNodeSCNToCoordSCN( aStatement, sClientInfo, &sDataInfo ) != IDE_SUCCESS );

    IDE_TEST( sSuccess == ID_FALSE );

    sdl::removeCallback( sCallback );

    IDE_EXCEPTION_CONT(NO_NEED_WORK);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sDoingRemoteStmtInitStep == ID_TRUE)
    {
        sConnectInfo = sClientInfo->mConnectInfo;
        sDataNode = sDataInfo.mNodes;
        for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
        {
            if ( sDataNode->mRemoteStmt != NULL )
            {
                if ( sDataNode->mRemoteStmt->mStmt != NULL )
                {
                    (void)sdl::freeStmt( sConnectInfo,
                                         sDataNode->mRemoteStmt,
                                         SQL_DROP,
                                         &(sConnectInfo->mLinkFailure) );
                }
                sdlStatementManager::setUnused( &sDataNode->mRemoteStmt );
            }
            if ( ( aInternalLocalOperation != SDI_INTERNAL_OP_NOT ) &&
                 ( sConnectInfo->mLinkFailure != ID_TRUE ) &&
                 ( sConnectInfo->mDbc != NULL ) )
            {
                (void)setInternalOp( SDI_INTERNAL_OP_NOT, sConnectInfo, &(sConnectInfo->mLinkFailure));
            }
        }
    }

    sdl::removeCallback( sCallback );
    return IDE_FAILURE;
}

IDE_RC sdi::shardExecDirect( qcStatement          * aStatement,
                             SChar                * aNodeName,
                             SChar                * aQuery,
                             UInt                   aQueryLen,
                             sdiInternalOperation   aInternalLocalOperation,
                             UInt                 * aExecCount,
                             ULong                * aNumResult,
                             SChar                * aStrResult,
                             UInt                   aMaxBuffer,
                             idBool               * aFetchResult )
{
    sdiStatement   * sSdStmt      = NULL;
    sdlRemoteStmt  * sRemoteStmt  = NULL;
    sdiClientInfo  * sClientInfo  = NULL;
    sdiConnectInfo * sConnectInfo = NULL;
    UShort           i = 0;
    idBool           sSuccess = ID_TRUE;
    UInt             sState   = 0;

    /* PROJ-2733-DistTxInfo */
    sdiDataNodes     sDataInfo; 
    sdiDataNode    * sDataNode = sDataInfo.mNodes;
    smSCN            sSCN;
    idBool           sNeedUpdateSCN = ID_FALSE;

    idBool           sIsBindCol = ID_FALSE;
    SInt             sValueLen = 0;
    SShort           sFetchResult = ID_SSHORT_MAX;

    SM_INIT_SCN( &sSCN );

    IDE_DASSERT( aStatement->session != NULL );

    sClientInfo = aStatement->session->mQPSpecific.mClientInfo;

    *aExecCount = 0;

    sSdStmt = (sdiStatement *)qci::mSessionCallback.mGetShardStmt( QC_MM_STMT( aStatement ) );

    IDE_TEST_CONT( sClientInfo == NULL, NO_NEED_WORK );

    sConnectInfo = sClientInfo->mConnectInfo;

    // check bind col
    if ( aMaxBuffer != 0 )
    {
        sIsBindCol = ID_TRUE;
    }
    
    /* PROJ-2733-DistTxInfo 수행할 노드를 선택 */
    IDE_TEST( buildDataNodes( sClientInfo, &sDataInfo, aNodeName, NULL ) != IDE_SUCCESS );

    /* 선택된 노드들은 분산정보를 위해 Execute 전에 모두 접속해 둔다. */
    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sDataNode->mState != SDI_NODE_STATE_EXECUTE_SELECTED )  /* PROJ-2733-DistTxInfo */
        {
            continue;
        }
 
        IDE_TEST( checkTargetSMN( sClientInfo, sConnectInfo ) != IDE_SUCCESS );

        sNeedUpdateSCN = (sConnectInfo->mDbc == NULL) ? ID_TRUE : ID_FALSE;

        // open shard connection
        IDE_TEST( qdkOpenShardConnection( sConnectInfo ) != IDE_SUCCESS );

        /* PROJ-2733-DistTxInfo Connection 이후 각 노드 SCN을 얻어 온다. */
        if ( sNeedUpdateSCN == ID_TRUE )
        {
            IDE_TEST( sdl::getSCN( sConnectInfo, &sSCN ) != IDE_SUCCESS );

            SM_SET_MAX_SCN( &(sClientInfo->mGCTxInfo.mCoordSCN), &sSCN );
        }

        // PROJ-2727
        IDE_TEST( setShardSessionProperty( aStatement->session,
                                           sClientInfo,
                                           sConnectInfo )
                  != IDE_SUCCESS );
    }

    /* PROJ-2733-DistTxInfo 분산정보를 계산한다. */
    calculateGCTxInfo( QC_PRIVATE_TMPLATE( aStatement ),
                       &sDataInfo,
                       ID_FALSE,
                       SDI_COORD_PLAN_INDEX_NONE );

    /* PROJ-2733-DistTxInfo Node에 분산정보를 전파한다. */
    IDE_TEST( propagateDistTxInfoToNodes( aStatement, sClientInfo, &sDataInfo ) != IDE_SUCCESS );

    sConnectInfo = sClientInfo->mConnectInfo;
    sDataNode = sDataInfo.mNodes;

    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sDataNode->mState != SDI_NODE_STATE_EXECUTE_SELECTED )  /* PROJ-2733-DistTxInfo */
        {
            continue;
        }
        sState = 1;

        /* TASK-7219 Non-shard DML */
        IDE_TEST( sdl::setStmtExecSeq( sConnectInfo, QCG_GET_SESSION_STMT_EXEC_SEQ_FOR_SHARD_TX(aStatement) ) != IDE_SUCCESS );

        IDE_TEST( sdlStatementManager::allocRemoteStatement( sSdStmt,
                                                             sConnectInfo->mNodeId,
                                                             &sRemoteStmt )
                  != IDE_SUCCESS );
        sState = 2;

        if ( aInternalLocalOperation != SDI_INTERNAL_OP_NOT )
        {
            IDE_TEST(setInternalOp(aInternalLocalOperation, 
                                   sConnectInfo, 
                                   &(sConnectInfo->mLinkFailure)) 
                     != IDE_SUCCESS);
        }
        sState = 3;

        IDE_TEST( sdl::allocStmt( sConnectInfo,
                                  sRemoteStmt,
                                  SDI_SHARD_PARTIAL_EXEC_TYPE_NONE, /* sPartialCoordExecType */
                                  &(sConnectInfo->mLinkFailure) )
                  != IDE_SUCCESS );        
        sState = 4;

        // add shard transaction
        IDE_TEST( qdkAddShardTransaction( aStatement->mStatistics,
                                          QC_SMI_STMT(aStatement)->mTrans->getTransID(),
                                          sClientInfo,
                                          sConnectInfo )
                  != IDE_SUCCESS );

        // TASK-7244 PSM partial rollback in Sharding
        if ( ((sConnectInfo->mFlag & SDI_CONNECT_PSM_SVP_SET_MASK) == SDI_CONNECT_PSM_SVP_SET_FALSE) &&
             (qciMisc::isBeginSP(aStatement) == ID_TRUE) )
        {
            IDE_TEST( sdl::setSavepoint( sConnectInfo,
                                         SAVEPOINT_FOR_SHARD_GLOBAL_PROC_PARTIAL_ROLLBACK )
                      != IDE_SUCCESS );
            sConnectInfo->mFlag &= ~SDI_CONNECT_PSM_SVP_SET_MASK;
            sConnectInfo->mFlag |= SDI_CONNECT_PSM_SVP_SET_TRUE;
        }

        if ( sIsBindCol == ID_TRUE )
        {
            if ( aNumResult != NULL )
            {
                IDE_TEST( sdl::bindCol( sConnectInfo,
                                        sRemoteStmt,
                                        1,
                                        MTD_BIGINT_ID,
                                        ID_SIZEOF(mtdBigintType),
                                        (void *)aNumResult,
                                        aMaxBuffer,
                                        &sValueLen,
                                        &(sConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( sdl::bindCol( sConnectInfo,
                                        sRemoteStmt,
                                        1,
                                        MTD_VARCHAR_ID,
                                        ID_SIZEOF(mtdCharType),
                                        (void *)aStrResult,
                                        aMaxBuffer,
                                        &sValueLen,
                                        &(sConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }
        
        if ( sdl::execDirect( sConnectInfo,
                              sRemoteStmt,
                              aQuery,
                              aQueryLen,
                              sClientInfo,
                              &(sConnectInfo->mLinkFailure) )
             == IDE_SUCCESS )
        {
            (*aExecCount)++;
            sDataNode->mState = SDI_NODE_STATE_EXECUTED;  /* PROJ-2733-DistTxInfo */

            if ( sIsBindCol == ID_TRUE )
            {
                IDE_TEST( sdl::fetch( sConnectInfo,
                                      sRemoteStmt,
                                      &sFetchResult,
                                      &(sConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );

                if ( sFetchResult == SQL_NO_DATA_FOUND )
                {
                    *aFetchResult = ID_FALSE;
                }
                else
                {
                    *aFetchResult = ID_TRUE;
                }
            
                IDE_TEST_RAISE(sValueLen == SQL_NULL_DATA, ERR_GET_ROW_COUNT);
            }        
        }
        else
        {
            // 수행이 실패한 경우
            sSuccess = ID_FALSE;
            sDataNode->mState = SDI_NODE_STATE_EXECUTE_CANDIDATED;  /* PROJ-2733-DistTxInfo */
        }

        sState = 3;
        if ( sdl::freeStmt( sConnectInfo,
                            sRemoteStmt,
                            SQL_DROP,
                            &(sConnectInfo->mLinkFailure) )
             != IDE_SUCCESS )
        {
            // 수행이 실패한 경우
            sSuccess = ID_FALSE;
        }

        sState = 2;
        if ( ( aInternalLocalOperation != SDI_INTERNAL_OP_NOT ) && 
             ( sConnectInfo->mLinkFailure != ID_TRUE ) &&
             ( sConnectInfo->mDbc != NULL ) )
        {
            IDE_TEST(setInternalOp( SDI_INTERNAL_OP_NOT, 
                                    sConnectInfo, 
                                    &(sConnectInfo->mLinkFailure)) 
                     != IDE_SUCCESS);
            ideLog::log(IDE_SD_17,"[SHARD_META] Internal Remote SQL: %s", aQuery);
        }
        sState = 1;

        sdlStatementManager::setUnused( &sRemoteStmt );
    }

    /* PROJ-2733-DistTxInfo
     * !!! sdl::execDirect 함수를 수행했으면 sSuccess 값에 상관없이
     *     updateMaxNodeSCNToCoordSCN 함수를 수행해야 한다
     */
    sState = 0;
    IDE_TEST( updateMaxNodeSCNToCoordSCN( aStatement, sClientInfo, &sDataInfo ) != IDE_SUCCESS );

    IDE_TEST( sSuccess == ID_FALSE );

    IDE_EXCEPTION_CONT(NO_NEED_WORK);

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_GET_ROW_COUNT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::shardExecDirect",
                                  "get row count fetch error" ) );
    }
    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 4 :
            if ( sRemoteStmt->mStmt != NULL )
            {
                (void)sdl::freeStmt( sConnectInfo,
                                     sRemoteStmt,
                                     SQL_DROP,
                                     &(sConnectInfo->mLinkFailure) );
            }
            /* fall through */

        case 3 :
            /* fall through */

        case 2 :
            sdlStatementManager::setUnused( &sRemoteStmt );
            /* fall through */

        case 1 :
            (void)updateMaxNodeSCNToCoordSCN( aStatement, sClientInfo, &sDataInfo );
            /* fall through */

        default:
            break;
    }

    sConnectInfo = sClientInfo->mConnectInfo;
    sDataNode = sDataInfo.mNodes;
    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( ( aInternalLocalOperation != SDI_INTERNAL_OP_NOT ) &&
             ( sConnectInfo->mLinkFailure != ID_TRUE ) &&
             ( sConnectInfo->mDbc != NULL ) )
        {
            (void)setInternalOp( SDI_INTERNAL_OP_NOT, sConnectInfo, &(sConnectInfo->mLinkFailure));
        }
    }

    return IDE_FAILURE;
}

IDE_RC sdi::getRemoteRowCount( qcStatement * aStatement,
                               SChar       * aNodeName,
                               SChar       * aUserName,
                               SChar       * aTableName, 
                               SChar       * aPartitionName,
                               sdiInternalOperation aInternalLocalOperation,
                               UInt        * aRowCount )
{
    sdiStatement   * sSdStmt      = NULL;
    sdlRemoteStmt  * sRemoteStmt  = NULL;
    sdiClientInfo  * sClientInfo  = NULL;
    sdiConnectInfo * sConnectInfo = NULL;
    UShort           i = 0;
    idBool           sSuccess = ID_TRUE;

    /* PROJ-2733-DistTxInfo */
    sdiDataNodes     sDataInfo; 
    idBool           sIsRequireUpdateMaxNodeSCN = ID_FALSE;

    sdiDataNode    * sDataNode = sDataInfo.mNodes;
    smSCN            sSCN;
    idBool           sNeedUpdateSCN = ID_FALSE;

    const SChar * sTableCountQuery = "select count(*) from %s.%s";
    const SChar * sPartitionCountQuery = "select count(*) from %s.%s partition (%s)";
    SChar         sQuery[ IDL_MAX(ID_SIZEOF(sTableCountQuery), ID_SIZEOF(sPartitionCountQuery)) + 
                          (QC_MAX_OBJECT_NAME_LEN * 2) ] = {0, };
    mtdBigintType    sRowCount = MTD_BIGINT_NULL;
    SInt             sValueLen = 0;
    SShort           sFetchResult = ID_SSHORT_MAX;

    SM_INIT_SCN( &sSCN );

    IDE_DASSERT( aStatement->session != NULL );

    sClientInfo = aStatement->session->mQPSpecific.mClientInfo;
    IDE_TEST_RAISE( sClientInfo == NULL, ERR_CLIENTINFO );

    IDE_TEST_RAISE( aNodeName == NULL, ERR_NODENAME );
    IDE_TEST_RAISE( idlOS::strlen( aNodeName ) <= 0, ERR_NODENAME );

    IDE_DASSERT(aTableName != NULL);
    IDE_DASSERT(aUserName != NULL);

    sSdStmt = (sdiStatement *)qci::mSessionCallback.mGetShardStmt( QC_MM_STMT( aStatement ) );

    /* PROJ-2733-DistTxInfo 수행할 노드를 선택 */
    IDE_TEST( buildDataNodes( sClientInfo, &sDataInfo, aNodeName, NULL ) != IDE_SUCCESS) ;

    sConnectInfo = sClientInfo->mConnectInfo;

    /* 선택된 노드들은 분산정보를 위해 Execute 전에 모두 접속해 둔다. */
    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sDataNode->mState != SDI_NODE_STATE_EXECUTE_SELECTED )  /* PROJ-2733-DistTxInfo */
        {
            continue;
        }
 
        IDE_TEST( checkTargetSMN( sClientInfo, sConnectInfo ) != IDE_SUCCESS );

        sNeedUpdateSCN = (sConnectInfo->mDbc == NULL) ? ID_TRUE : ID_FALSE;

        // open shard connection
        IDE_TEST( qdkOpenShardConnection( sConnectInfo ) != IDE_SUCCESS );

        /* PROJ-2733-DistTxInfo Connection 이후 각 노드 SCN을 얻어 온다. */
        if ( sNeedUpdateSCN == ID_TRUE )
        {
            IDE_TEST( sdl::getSCN( sConnectInfo, &sSCN ) != IDE_SUCCESS );

            SM_SET_MAX_SCN( &(sClientInfo->mGCTxInfo.mCoordSCN), &sSCN );
        }

        // PROJ-2727
        IDE_TEST( setShardSessionProperty( aStatement->session,
                                           sClientInfo,
                                           sConnectInfo )
                  != IDE_SUCCESS );
    }

    /* PROJ-2733-DistTxInfo 분산정보를 계산한다. */
    calculateGCTxInfo( QC_PRIVATE_TMPLATE( aStatement ),
                       &sDataInfo,
                       ID_FALSE,
                       SDI_COORD_PLAN_INDEX_NONE );

    /* PROJ-2733-DistTxInfo Node에 분산정보를 전파한다. */
    IDE_TEST( propagateDistTxInfoToNodes( aStatement, sClientInfo, &sDataInfo ) != IDE_SUCCESS );

    sConnectInfo = sClientInfo->mConnectInfo;
    sDataNode = sDataInfo.mNodes;

    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sDataNode->mState != SDI_NODE_STATE_EXECUTE_SELECTED )  /* PROJ-2733-DistTxInfo */
        {
            continue;
        }

        /* TASK-7219 Non-shard DML */
        IDE_TEST( sdl::setStmtExecSeq( sConnectInfo, QCG_GET_SESSION_STMT_EXEC_SEQ_FOR_SHARD_TX(aStatement) ) != IDE_SUCCESS );

        IDE_TEST( sdlStatementManager::allocRemoteStatement( sSdStmt,
                                                             sConnectInfo->mNodeId,
                                                             &sRemoteStmt )
                  != IDE_SUCCESS );

        if ( aInternalLocalOperation != SDI_INTERNAL_OP_NOT )
        {
            IDE_TEST(setInternalOp(aInternalLocalOperation, 
                                   sConnectInfo, 
                                   &(sConnectInfo->mLinkFailure)) 
                     != IDE_SUCCESS);
        }
            
        IDE_TEST( sdl::allocStmt( sConnectInfo,
                                  sRemoteStmt,
                                  SDI_SHARD_PARTIAL_EXEC_TYPE_NONE, /* sPartialCoordExecType */
                                  &(sConnectInfo->mLinkFailure) )
                  != IDE_SUCCESS );        

        // add shard transaction
        IDE_TEST( qdkAddShardTransaction( aStatement->mStatistics,
                                          QC_SMI_STMT(aStatement)->mTrans->getTransID(),
                                          sClientInfo,
                                          sConnectInfo )
                  != IDE_SUCCESS );
            
        if ( aPartitionName == NULL )
        {
            idlOS::snprintf( sQuery, ID_SIZEOF(sQuery), sTableCountQuery, aUserName, aTableName);
        }
        else
        {
            idlOS::snprintf( sQuery, ID_SIZEOF(sQuery), sPartitionCountQuery, aUserName, aTableName, aPartitionName);
        }

        IDE_TEST( sdl::bindCol( sConnectInfo,
                                sRemoteStmt,
                                1,
                                MTD_BIGINT_ID,
                                ID_SIZEOF(mtdBigintType),
                                (void *)&sRowCount,
                                ID_SIZEOF(sRowCount),
                                &sValueLen,
                                &(sConnectInfo->mLinkFailure) ) != IDE_SUCCESS );

        sIsRequireUpdateMaxNodeSCN = ID_TRUE;
        if ( sdl::execDirect( sConnectInfo,
                              sRemoteStmt,
                              sQuery,
                              idlOS::strlen(sQuery),
                              sClientInfo,
                              &(sConnectInfo->mLinkFailure) )
             == IDE_SUCCESS )
        {
            sDataNode->mState = SDI_NODE_STATE_EXECUTED;  /* PROJ-2733-DistTxInfo */
        }
        else
        {
            // 수행이 실패한 경우
            sSuccess = ID_FALSE;
            sDataNode->mState = SDI_NODE_STATE_EXECUTE_CANDIDATED;  /* PROJ-2733-DistTxInfo */
        }

        IDE_TEST( sdl::fetch( sConnectInfo,
                              sRemoteStmt,
                              &sFetchResult,
                              &(sConnectInfo->mLinkFailure) )
                    != IDE_SUCCESS );

        IDE_TEST_RAISE(sFetchResult == SQL_NO_DATA_FOUND, ERR_NO_DATA_FOUND);
        IDE_TEST_RAISE(sValueLen == SQL_NULL_DATA, ERR_GET_ROW_COUNT);
        *aRowCount = sRowCount;

        if ( sdl::freeStmt( sConnectInfo,
                            sRemoteStmt,
                            SQL_DROP,
                            &(sConnectInfo->mLinkFailure) )
             != IDE_SUCCESS )
        {
            // 수행이 실패한 경우
            sSuccess = ID_FALSE;
        }

        if ( ( aInternalLocalOperation != SDI_INTERNAL_OP_NOT ) && 
             ( sConnectInfo->mLinkFailure != ID_TRUE ) &&
             ( sConnectInfo->mDbc != NULL ) )
        {
            IDE_TEST(setInternalOp( SDI_INTERNAL_OP_NOT, 
                                    sConnectInfo, 
                                    &(sConnectInfo->mLinkFailure)) 
                     != IDE_SUCCESS);
            ideLog::log(IDE_SD_17,"[SHARD_META] Internal Remote SQL: %s", sQuery);
        }

        sdlStatementManager::setUnused( &sRemoteStmt );
    }

    /* PROJ-2733-DistTxInfo
     * !!! sdl::execDirect 함수를 수행했으면 sSuccess 값에 상관없이
     *     updateMaxNodeSCNToCoordSCN 함수를 수행해야 한다
     */
    sIsRequireUpdateMaxNodeSCN = ID_FALSE;
    IDE_TEST( updateMaxNodeSCNToCoordSCN( aStatement, sClientInfo, &sDataInfo ) != IDE_SUCCESS );

    IDE_TEST( sSuccess == ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CLIENTINFO )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::getRemoteRowCount",
                                  "not exist client info" ) );
    }
    IDE_EXCEPTION( ERR_NODENAME )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::getRemoteRowCount node name wrong",
                                  aNodeName ) );
    }
    IDE_EXCEPTION( ERR_NO_DATA_FOUND )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::getRemoteRowCount",
                                  "get row count fetch no data found" ) );
    }
    IDE_EXCEPTION( ERR_GET_ROW_COUNT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::getRemoteRowCount",
                                  "get row count fetch error" ) );
        ideLog::log(IDE_SD_31,"[DEBUG] Shard get row count: %"ID_INT32_FMT, sFetchResult);
    }
    IDE_EXCEPTION_END;

    if ( sIsRequireUpdateMaxNodeSCN == ID_TRUE )
    {
        /* PROJ-2733-DistTxInfo
         * !!! sdl::execDirect 함수를 수행했으면 sSuccess 값에 상관없이
         *     updateMaxNodeSCNToCoordSCN 함수를 수행해야 한다
         */
        (void) updateMaxNodeSCNToCoordSCN( aStatement, sClientInfo, &sDataInfo );
    }
    if ( sRemoteStmt != NULL )
    {
        if ( sRemoteStmt->mStmt != NULL )
        {
            (void)sdl::freeStmt( sConnectInfo,
                                 sRemoteStmt,
                                 SQL_DROP,
                                 &(sConnectInfo->mLinkFailure) );
        }
        sdlStatementManager::setUnused( &sRemoteStmt );
    }

    if ( ( aInternalLocalOperation != SDI_INTERNAL_OP_NOT ) && 
         ( sConnectInfo->mLinkFailure != ID_TRUE ) &&
         ( sConnectInfo->mDbc != NULL ) )
    {
        (void)setInternalOp( SDI_INTERNAL_OP_NOT, sConnectInfo, &(sConnectInfo->mLinkFailure));
    }
    return IDE_FAILURE;
}

void sdi::initDataInfo( qcShardExecData * aExecData )
{
    aExecData->planCount = 0;
    aExecData->execInfo = NULL;
    aExecData->dataSize = 0;
    aExecData->data = NULL;
    aExecData->partialStmt = ID_FALSE;
    SMI_INIT_SCN( &aExecData->leadingRequestSCN );
    aExecData->leadingPlanIndex = ID_UINT_MAX;
    aExecData->globalPSM = ID_FALSE;

    /* TASK-7219 Non-shard DML */
    aExecData->partialExecType = SDI_SHARD_PARTIAL_EXEC_TYPE_NONE;
}

IDE_RC sdi::allocDataInfo( qcShardExecData * aExecData,
                           iduVarMemList   * aMemory )
{
    sdiDataNodes * sDataInfo;
    UInt           i;

    if ( aExecData->planCount > 0 )
    {
        IDE_TEST( aMemory->alloc( ID_SIZEOF(sdiDataNodes) *
                                  aExecData->planCount,
                                  (void**) &(aExecData->execInfo) )
                  != IDE_SUCCESS );

        sDataInfo = (sdiDataNodes*)aExecData->execInfo;

        for ( i = 0; i < aExecData->planCount; i++, sDataInfo++ )
        {
            sDataInfo->mInitialized = ID_FALSE;
        }
    }
    else
    {
        // Nothing to do.
    }

    if ( aExecData->dataSize > 0 )
    {
        IDE_TEST( aMemory->alloc( aExecData->dataSize,
                                  (void**) &(aExecData->data) )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void sdi::setDataNodePrepared( sdiClientInfo * aClientInfo,
                               sdiDataNodes  * aDataNode )
{
    sdiDataNode    * sDataNode;
    UInt             i;

    ACP_UNUSED( aClientInfo );

    sDataNode = aDataNode->mNodes;

    // executed -> prepared
    for ( i = 0; i < aDataNode->mCount; i++, sDataNode++ )
    {
        if ( sDataNode->mState >= SDI_NODE_STATE_EXECUTED )
        {
            sDataNode->mState = SDI_NODE_STATE_PREPARED;
        }
        else
        {
            // Nothing to do.
        }
    }
}

void sdi::closeDataInfo( qcStatement     * aStatement,
                         qcShardExecData * aExecData )
{
    sdiClientInfo * sClientInfo;
    sdiDataNodes  * sDataInfo;
    UInt            i;

    if ( aExecData->execInfo != NULL )
    {
        sClientInfo = aStatement->session->mQPSpecific.mClientInfo;
        sDataInfo = (sdiDataNodes*)aExecData->execInfo;

        // statement가 clear되는 경우 stmt들을 free한다.
        for ( i = 0; i < aExecData->planCount; i++, sDataInfo++ )
        {
            if ( sDataInfo->mInitialized == ID_TRUE )
            {
                setDataNodePrepared( sClientInfo, sDataInfo );
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }
}

void sdi::clearDataInfo( qcStatement     * aStatement,
                         qcShardExecData * aExecData )
{
    sdiDataNodes   * sDataInfo;
    sdiDataNode    * sDataNode;
    UInt             i;
    UInt             j;

    if ( ( aExecData->execInfo != NULL ) && ( aStatement->session != NULL ) )
    {
        sDataInfo = (sdiDataNodes*)aExecData->execInfo;

        // statement가 clear되는 경우 stmt들을 free한다.
        for ( i = 0; i < aExecData->planCount; i++, sDataInfo++ )
        {
            if ( sDataInfo->mInitialized == ID_TRUE )
            {
                sDataNode    = sDataInfo->mNodes;

                for ( j = 0; j < sDataInfo->mCount; j++, sDataNode++ )
                {
                    sdlStatementManager::setUnused( &sDataNode->mRemoteStmt );
                }

                sDataInfo->mInitialized = ID_FALSE;
            }
            else
            {
                // Nothing to do.
            }
        }

        aExecData->planCount = 0;
        aExecData->execInfo = NULL;
    }
    else
    {
        // Nothing to do.
    }

    if ( aExecData->data != NULL )
    {
        aExecData->dataSize = 0;
        aExecData->data = NULL;
    }
    else
    {
        // Nothing to do.
    }

    aExecData->partialStmt = ID_FALSE;
    SMI_INIT_SCN( &aExecData->leadingRequestSCN );
    aExecData->leadingPlanIndex = ID_UINT_MAX;
    aExecData->globalPSM = ID_FALSE;

    /* TASK-7219 Non-shard DML */
    aExecData->partialExecType = SDI_SHARD_PARTIAL_EXEC_TYPE_NONE;
}

void sdi::clearShardDataInfoForRebuild( qcStatement     * aStatement,
                                        qcShardExecData * aExecData )
{
    sdiDataNodes   * sDataInfo;
    sdiDataNode    * sDataNode;
    UInt             i;
    UInt             j;

    if ( ( aExecData->execInfo != NULL ) && ( aStatement->session != NULL ) )
    {
        sDataInfo = (sdiDataNodes*)aExecData->execInfo;

        for ( i = 0; i < aExecData->planCount; i++, sDataInfo++ )
        {
            if ( sDataInfo->mInitialized == ID_TRUE )
            {
                sDataNode    = sDataInfo->mNodes;

                for ( j = 0; j < sDataInfo->mCount; j++, sDataNode++ )
                {
                    sdlStatementManager::setUnused( &sDataNode->mRemoteStmt );
                }

                sDataInfo->mInitialized = ID_FALSE;
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

}

IDE_RC sdi::initShardDataInfo( qcTemplate     * aTemplate,
                               sdiAnalyzeInfo * aShardAnalysis,
                               sdiClientInfo  * aClientInfo,
                               sdiDataNodes   * aDataInfo,
                               sdiDataNode    * aDataArg )
{
    sdiConnectInfo * sConnectInfo = NULL;
    sdiDataNode    * sDataNode    = NULL;
    UInt             i;
    UInt             j;

    /* PROJ-2655 Composite shard key */
    UShort  sRangeIndexCount = 0;
    UShort  sRangeIndex[SDI_VALUE_MAX_COUNT];

    idBool  sExecDefaultNode = ID_FALSE;
    idBool  sIsAllNodeExec = ID_FALSE;

    SChar          sNodeNameBuf[SDI_NODE_NAME_MAX_SIZE + 1];
    qcShardNodes * sNodeName;
    idBool         sFound;

    sdiStatement * sSdStmt = NULL;

    ULong          sTargetSMN = SDI_NULL_SMN;

    IDE_TEST_RAISE( aShardAnalysis == NULL, ERR_NOT_EXIST_SHARD_ANALYSIS );

    //----------------------------------------
    // data info 초기화
    //----------------------------------------

    aDataInfo->mCount = aClientInfo->mCount;

    sConnectInfo = aClientInfo->mConnectInfo;
    sDataNode = aDataInfo->mNodes;

    sTargetSMN = aClientInfo->mTargetShardMetaNumber;

    sSdStmt = (sdiStatement *)qci::mSessionCallback.mGetShardStmt( QC_MM_STMT( aTemplate->stmt ) );
    IDE_DASSERT( sSdStmt != NULL );

    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        idlOS::memcpy( sDataNode, aDataArg, ID_SIZEOF(sdiDataNode) );

        IDE_DASSERT( sDataNode->mRemoteStmt == NULL );

        IDE_TEST( sdlStatementManager::allocRemoteStatement( sSdStmt,
                                                             sConnectInfo->mNodeId,
                                                             &sDataNode->mRemoteStmt )
                  != IDE_SUCCESS );
    }

    //----------------------------------------
    // data node prepare 후보 선택
    //----------------------------------------

    if ( aShardAnalysis == & gAnalyzeInfoForAllNodes )
    {
        // 전체 data node를 선택
        sConnectInfo = aClientInfo->mConnectInfo;
        sDataNode = aDataInfo->mNodes;

        for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
        {
            if ( sConnectInfo->mNodeInfo.mSMN == sTargetSMN )
            {
                sDataNode->mState = SDI_NODE_STATE_PREPARE_CANDIDATED;
            }
        }
    }
    else
    {
        if ( ( aShardAnalysis->mValuePtrCount > 0 ) &&
             ( findBindParameter( aShardAnalysis ) == ID_FALSE ) &&
             ( getSplitType ( aShardAnalysis->mSplitMethod ) == SDI_SPLIT_TYPE_DIST ) )
        {
            sConnectInfo = aClientInfo->mConnectInfo;
            sDataNode = aDataInfo->mNodes;

            IDE_TEST( getExecNodeRangeIndex( NULL, // aTemplate
                                             NULL, // aShardKeyTuple
                                             NULL, // aShardSubKeyTuple
                                             aShardAnalysis,
                                             sRangeIndex,
                                             &sRangeIndexCount,
                                             &sExecDefaultNode,
                                             &sIsAllNodeExec )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( ( sRangeIndexCount == 0 ) &&
                            ( sExecDefaultNode == ID_FALSE ) &&
                            ( sIsAllNodeExec == ID_FALSE ),
                            ERR_NO_EXEC_NODE_FOUND );

            if ( sIsAllNodeExec == ID_TRUE )
            {
                for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
                {
                    // rangeInfo에 포함되어 있거나, default node는 prepare 대상이다.
                    if ( ( findRangeInfo( &(aShardAnalysis->mRangeInfo),
                                          sConnectInfo->mNodeId ) == ID_TRUE ) ||
                         ( aShardAnalysis->mDefaultNodeId == sConnectInfo->mNodeId ) )
                    {
                        sDataNode->mState = SDI_NODE_STATE_PREPARE_CANDIDATED;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
            else
            {
                // constant value only
                for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
                {
                    for ( j = 0; j < sRangeIndexCount; j++ )
                    {
                        if ( aShardAnalysis->mRangeInfo.mRanges[sRangeIndex[j]].mNodeId ==
                             sConnectInfo->mNodeId )
                        {
                            sDataNode->mState = SDI_NODE_STATE_PREPARE_CANDIDATED;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }

                    if ( ( sExecDefaultNode == ID_TRUE ) &&
                         ( aShardAnalysis->mDefaultNodeId == sConnectInfo->mNodeId ) )
                    {
                        sDataNode->mState = SDI_NODE_STATE_PREPARE_CANDIDATED;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
        }
        else
        {
            if ( aShardAnalysis->mSplitMethod == SDI_SPLIT_NODES )
            {
                for ( sNodeName = aShardAnalysis->mNodeNames;
                      sNodeName != NULL;
                      sNodeName = sNodeName->next )
                {
                    sFound = ID_FALSE;

                    sConnectInfo = aClientInfo->mConnectInfo;
                    sDataNode = aDataInfo->mNodes;

                    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
                    {
                        if ( sConnectInfo->mNodeInfo.mSMN == sTargetSMN )
                        {
                            if ( isMatchedNodeName( sNodeName,
                                                    sConnectInfo->mNodeName ) == ID_TRUE )
                            {
                                sDataNode->mState = SDI_NODE_STATE_PREPARE_CANDIDATED;

                                sFound = ID_TRUE;
                                break;
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        }
                    }

                    IDE_TEST_RAISE( sFound != ID_TRUE, ERR_NODE_NOT_FOUND );
                }
            }
            else
            {
                sConnectInfo = aClientInfo->mConnectInfo;
                sDataNode = aDataInfo->mNodes;

                // clone이라도 range node정보는 있다.
                for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
                {
                    // rangeInfo에 포함되어 있거나, default node는 prepare 대상이다.
                    if ( ( findRangeInfo( &(aShardAnalysis->mRangeInfo),
                                          sConnectInfo->mNodeId ) == ID_TRUE ) ||
                         ( aShardAnalysis->mDefaultNodeId == sConnectInfo->mNodeId ) )
                    {
                        sDataNode->mState = SDI_NODE_STATE_PREPARE_CANDIDATED;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
        }
    }

    aDataInfo->mInitialized = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_SHARD_ANALYSIS )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_NOT_EXIST_SHARD_ANALYSIS ) );
    }
    IDE_EXCEPTION( ERR_NO_EXEC_NODE_FOUND )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_DATA_NODE_NOT_FOUND ) );
    }
    IDE_EXCEPTION( ERR_NODE_NOT_FOUND )
    {
        // 길이검사는 이미 했다.
        IDE_DASSERT( sNodeName->namePos.size <= SDI_NODE_NAME_MAX_SIZE );

        if ( isNameInDoubleQuotation( &(sNodeName->namePos) ) == ID_FALSE )
        {
            idlOS::memcpy( sNodeNameBuf,
                           sNodeName->namePos.stmtText + sNodeName->namePos.offset,
                           sNodeName->namePos.size );
            sNodeNameBuf[sNodeName->namePos.size] = '\0';
        }
        else
        {
            (void)removeDoulbeQuotationFromNodeName( sNodeName,
                                                     sNodeNameBuf,
                                                     SDI_NODE_NAME_MAX_SIZE + 1 );
        }

        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_INVALID_NODE_NAME2,
                                  sNodeNameBuf ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::reuseShardDataInfo( qcTemplate     * aTemplate,
                                sdiClientInfo  * aClientInfo,
                                sdiDataNodes   * aDataInfo,
                                sdiBindParam   * aBindParams,
                                UShort           aBindParamCount,
                                sdiSVPStep       aSVPStep)
{
    sdiDataNode    * sDataNode = NULL;
    UInt             i;

    ACP_UNUSED( aTemplate );
    ACP_UNUSED( aClientInfo );

    //----------------------------------------
    // data info 초기화
    //----------------------------------------

    if ( aDataInfo->mCount > 0 )
    {
        sDataNode = aDataInfo->mNodes;

        for ( i = 0; i < aDataInfo->mCount; i++, sDataNode++ )
        {
            if ( sDataNode->mState >= SDI_NODE_STATE_PREPARED )
            {
                IDE_TEST_RAISE( sDataNode->mRemoteStmt == NULL,
                                ERR_NOT_EXIST_STATEMENT );

                IDE_TEST_RAISE( ( sDataNode->mRemoteStmt->mFreeFlag & SDL_STMT_FREE_FLAG_ALLOCATED_MASK )
                                == SDL_STMT_FREE_FLAG_ALLOCATED_FALSE,
                                ERR_NOT_EXIST_STATEMENT );
            }

            sDataNode->mSVPStep = aSVPStep;
        }

        sDataNode = aDataInfo->mNodes;

        if ( ( aBindParamCount > 0 ) &&
             ( sDataNode->mBindParamCount > 0 ) )
        {
            IDE_DASSERT( sDataNode->mBindParamCount == aBindParamCount );

            // bind parameter가 변경되었나?
            if ( idlOS::memcmp( sDataNode->mBindParams,
                                aBindParams,
                                ID_SIZEOF( sdiBindParam ) * aBindParamCount ) != 0 )
            {
                // bind 정보는 현재 한벌이므로 한번만 복사한다.
                idlOS::memcpy( sDataNode->mBindParams,
                               aBindParams,
                               ID_SIZEOF( sdiBindParam ) * aBindParamCount );

                for ( i = 0; i < aDataInfo->mCount; i++, sDataNode++ )
                {
                    sDataNode->mBindParamChanged = ID_TRUE;
                }
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_STATEMENT )
    {
        /* statement is not allocated. need prepare. */
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::reuseShardDataInfo",
                                  "not exist statement" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::decideShardDataInfo( qcTemplate     * aTemplate,
                                 mtcTuple       * aShardKeyTuple,
                                 sdiAnalyzeInfo * aShardAnalysis,
                                 sdiClientInfo  * aClientInfo,
                                 sdiDataNodes   * aDataInfo,
                                 qcNamePosition * aShardQuery )
{
    sdiConnectInfo * sConnectInfo = NULL;
    UInt             i = 0;

    /* PROJ-2655 Composite shard key */
    UShort  sRangeIndex[SDI_VALUE_MAX_COUNT];
    UShort  sRangeIndexCount = 0;

    idBool  sExecDefaultNode = ID_FALSE;
    idBool  sIsAllNodeExec  = ID_FALSE;

    qcShardNodes * sNodeName;
    idBool         sFound;

    IDE_TEST_RAISE( aShardAnalysis == NULL, ERR_NOT_EXIST_SHARD_ANALYSIS );

    IDE_TEST_RAISE( aShardQuery->size == QC_POS_EMPTY_SIZE,
                    ERR_NULL_SHARD_QUERY );

    //----------------------------------------
    // data node execute 후보 선택
    //----------------------------------------

    if ( aShardAnalysis == & gAnalyzeInfoForAllNodes )
    {
        // 전체 data node를 선택
        IDE_TEST( setPrepareSelected( aClientInfo,
                                      aDataInfo,
                                      ID_TRUE,  // all nodes
                                      0 )
                  != IDE_SUCCESS );
    }
    else
    {
        switch ( aShardAnalysis->mSplitMethod )
        {
            case SDI_SPLIT_CLONE:

                if ( aShardAnalysis->mValuePtrCount == 0 )
                {
                    if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
                    {
                        // split clone ( random execution )
                        i = (UInt)(((UShort)idlOS::rand()) % aShardAnalysis->mRangeInfo.mCount);
                    }
                    else
                    {
                        i = 0;
                    }

                    IDE_TEST( setPrepareSelected( aClientInfo,
                                                  aDataInfo,
                                                  ID_FALSE,
                                                  aShardAnalysis->mRangeInfo.mRanges[i].mNodeId )
                              != IDE_SUCCESS );
                }
                else
                {
                    sConnectInfo = aClientInfo->mConnectInfo;

                    // BUG-44711
                    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++ )
                    {
                        if ( ( findRangeInfo( &(aShardAnalysis->mRangeInfo),
                                              sConnectInfo->mNodeId ) == ID_TRUE ) ||
                             ( aShardAnalysis->mDefaultNodeId == sConnectInfo->mNodeId ) )
                        {
                            IDE_TEST( setPrepareSelected( aClientInfo,
                                                          aDataInfo,
                                                          ID_FALSE,
                                                          sConnectInfo->mNodeId )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                }
                break;

            case SDI_SPLIT_SOLO:

                IDE_DASSERT( aShardAnalysis->mRangeInfo.mCount == 1 );

                IDE_TEST( setPrepareSelected( aClientInfo,
                                              aDataInfo,
                                              ID_FALSE,
                                              aShardAnalysis->mRangeInfo.mRanges[0].mNodeId )
                          != IDE_SUCCESS );
                break;

            case SDI_SPLIT_HASH:
            case SDI_SPLIT_RANGE:
            case SDI_SPLIT_LIST:

                /*
                 * Shard value( bind or constant value )가
                 * Analysis result상의 range info에서 몇 번 째(range index) 위치한 value인지 찾는다.
                 */
                IDE_TEST( getExecNodeRangeIndex( aTemplate,
                                                 aShardKeyTuple,
                                                 aShardKeyTuple,
                                                 aShardAnalysis,
                                                 sRangeIndex,
                                                 &sRangeIndexCount,
                                                 &sExecDefaultNode,
                                                 &sIsAllNodeExec )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE( ( sRangeIndexCount == 0 ) &&
                                ( sExecDefaultNode == ID_FALSE ) &&
                                ( sIsAllNodeExec == ID_FALSE ),
                                ERR_NO_EXEC_NODE_FOUND );

                if ( sIsAllNodeExec == ID_FALSE )
                {
                    for ( i = 0; i < sRangeIndexCount; i++ )
                    {
                        IDE_TEST( setPrepareSelected( aClientInfo,
                                                      aDataInfo,
                                                      ID_FALSE,
                                                      aShardAnalysis->mRangeInfo.mRanges[sRangeIndex[i]].mNodeId )
                                  != IDE_SUCCESS );
                    }

                    if ( sExecDefaultNode == ID_TRUE )
                    {
                        /* BUG-45738 */
                        // Default node외에 수행 대상 노드가 없는데
                        // Default node가 설정 되어있지 않다면 에러
                        IDE_TEST_RAISE( ( sRangeIndexCount == 0 ) &&
                                        ( aShardAnalysis->mDefaultNodeId == SDI_NODE_NULL_ID ),
                                        ERR_NO_EXEC_NODE_FOUND );

                        // Default node가 없더라도, 수행 대상 노드가 하나라도 있으면
                        // 그 노드에서만 수행시킨다. ( for SELECT )
                        if ( aShardAnalysis->mDefaultNodeId != SDI_NODE_NULL_ID )
                        {
                            IDE_TEST( setPrepareSelected( aClientInfo,
                                                          aDataInfo,
                                                          ID_FALSE,
                                                          aShardAnalysis->mDefaultNodeId )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    sConnectInfo = aClientInfo->mConnectInfo;

                    // BUG-44711
                    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++ )
                    {
                        if ( ( findRangeInfo( &(aShardAnalysis->mRangeInfo),
                                              sConnectInfo->mNodeId ) == ID_TRUE ) ||
                             ( aShardAnalysis->mDefaultNodeId == sConnectInfo->mNodeId ) )
                        {
                            IDE_TEST( setPrepareSelected( aClientInfo,
                                                          aDataInfo,
                                                          ID_FALSE,
                                                          sConnectInfo->mNodeId )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                }
                break;

            case SDI_SPLIT_NODES:
                for ( sNodeName = aShardAnalysis->mNodeNames;
                      sNodeName != NULL;
                      sNodeName = sNodeName->next )
                {
                    sFound = ID_FALSE;

                    sConnectInfo = aClientInfo->mConnectInfo;

                    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++ )
                    {
                        if ( isMatchedNodeName( sNodeName,
                                                sConnectInfo->mNodeName ) == ID_TRUE )
                        {
                            IDE_TEST( setPrepareSelected( aClientInfo,
                                                          aDataInfo,
                                                          ID_FALSE,
                                                          sConnectInfo->mNodeId )
                                      != IDE_SUCCESS );

                            sFound = ID_TRUE;
                            break;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }

                    IDE_TEST_RAISE( sFound == ID_FALSE, ERR_NOT_FOUND );
                }
                break;

            default:
                break;
        }
    }

    //----------------------------------------
    // data node execute 후보 준비
    //----------------------------------------

    IDE_TEST( prepare( aTemplate,
                       aClientInfo,
                       aDataInfo,
                       aShardQuery )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_SHARD_ANALYSIS )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_NOT_EXIST_SHARD_ANALYSIS ) );
    }
    IDE_EXCEPTION( ERR_NO_EXEC_NODE_FOUND )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_DATA_NODE_NOT_FOUND ) );
    }
    IDE_EXCEPTION( ERR_NULL_SHARD_QUERY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::decideShardDataInfo",
                                  "null shard query" ) );
    }
    IDE_EXCEPTION( ERR_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::getExecNodeRangeIndex( qcTemplate        * aTemplate,
                                   mtcTuple          * aShardKeyTuple,
                                   mtcTuple          * aShardSubKeyTuple,
                                   sdiAnalyzeInfo    * aShardAnalysis,
                                   UShort            * aRangeIndex,
                                   UShort            * aRangeIndexCount,
                                   idBool            * aExecDefaultNode,
                                   idBool            * aExecAllNodes )
{
    UInt    i;
    UInt    j;

    UShort  sFirstRangeIndexCount = 0;
    sdiRangeIndex sFirstRangeIndex[SDI_VALUE_MAX_COUNT];

    UShort  sSecondRangeIndexCount = 0;
    sdiRangeIndex sSecondRangeIndex[SDI_VALUE_MAX_COUNT];

    UShort sSubValueIndex = ID_USHORT_MAX;

    idBool  sIsFound = ID_FALSE;
    idBool  sIsSame = ID_FALSE;

    if ( ( aShardAnalysis->mValuePtrCount == 0 ) && 
         ( aShardAnalysis->mSubValuePtrCount == 0 ) )
    {
        /*
         * CASE 1 : ( mValueCount == 0 && mSubValueCount == 0 )
         *
         * Shard value가 없다면, 모든 노드가 수행 대상이다.
         *
         */
        *aExecAllNodes = ID_TRUE;
        *aExecDefaultNode = ID_TRUE;
    }
    else
    {
        // shard key value에 해당하는 range info의 index들을 구한다.
        for ( i = 0; i < aShardAnalysis->mValuePtrCount; i++ )
        {
            IDE_TEST( getRangeIndexByValue( aTemplate,
                                            aShardKeyTuple,
                                            aShardAnalysis,
                                            i,
                                            aShardAnalysis->mValuePtrArray[i],
                                            sFirstRangeIndex,
                                            &sFirstRangeIndexCount,
                                            aExecDefaultNode,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }

        // Sub-shard key가 존재하는 경우
        if ( aShardAnalysis->mSubKeyExists == ID_TRUE )
        {
            if ( aShardAnalysis->mValuePtrCount > 1 )
            {
                /*
                 * Sub-shard key가 있는 경우,
                 * 첫 번 째 shard key에 대한 value 가 둘 이상이라면, 수행노드를 정확히 구분 해 낼 수 없다.
                 * 다만, 첫 번 째 shard key에 대한 value가 여러개라도 모두 같은 값이라면,
                 * 수행노드를 판별 할 수 있기 때문에 허용한다.
                 *
                 * e.x.    WHERE ( KEY1 = 100 AND KEY2 = 200 ) OR ( KEY1 = 100 AND KEY2 = 300 )
                 *       = WHERE ( KEY1 = 100 ) AND ( KEY2 = 100 OR KEY2 = 200 )
                 */
                for ( i = 1; i < aShardAnalysis->mValuePtrCount; i++ )
                {
                    IDE_TEST( sdi::checkValuesSame( aTemplate,
                                                    aShardKeyTuple,
                                                    aShardAnalysis->mKeyDataType,
                                                    aShardAnalysis->mValuePtrArray[0],
                                                    aShardAnalysis->mValuePtrArray[i],
                                                    &sIsSame )
                              != IDE_SUCCESS );

                    if ( sIsSame == ID_FALSE )
                    {
                        *aExecAllNodes = ID_TRUE;
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
            else
            {
                // Nothing to do.
            }

            if ( *aExecAllNodes == ID_FALSE )
            {
                for ( i = 0; i < aShardAnalysis->mSubValuePtrCount; i++ )
                {
                    IDE_TEST( getRangeIndexByValue( aTemplate,
                                                    aShardSubKeyTuple,
                                                    aShardAnalysis,
                                                    i,
                                                    aShardAnalysis->mSubValuePtrArray[i],
                                                    sSecondRangeIndex,
                                                    &sSecondRangeIndexCount,
                                                    aExecDefaultNode,
                                                    ID_TRUE )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }

        if ( *aExecAllNodes == ID_FALSE )
        {
            if ( aShardAnalysis->mValuePtrCount > 0 )
            {
                if ( ( aShardAnalysis->mSubKeyExists == ID_TRUE ) && 
                     ( aShardAnalysis->mSubValuePtrCount > 0 ) )
                {
                    /*
                     * CASE 2 : ( mValueCount > 0 && mSubValueCount > 0 )
                     *
                     * value와 sub value의 range index가 같은 노드들이 수행대상이 된다.
                     *
                     */

                    sSubValueIndex = sSecondRangeIndex[0].mValueIndex;

                    for ( i = 0; i < sSecondRangeIndexCount; i++ )
                    {
                        if ( sSubValueIndex != sSecondRangeIndex[i].mValueIndex )
                        {
                            if ( sIsFound == ID_FALSE )
                            {
                                *aExecDefaultNode = ID_TRUE;
                            }
                            else
                            {
                                // Nothing to do.
                            }

                            sSubValueIndex = sSecondRangeIndex[i].mValueIndex;
                            sIsFound = ID_FALSE;
                        }
                        else
                        {
                            // Nothing to do.
                        }

                        for ( j = 0; j < sFirstRangeIndexCount; j++ )
                        {
                            if ( sFirstRangeIndex[j].mRangeIndex == sSecondRangeIndex[i].mRangeIndex )
                            {
                                aRangeIndex[*aRangeIndexCount] = sFirstRangeIndex[j].mRangeIndex;
                                (*aRangeIndexCount)++;

                                sIsFound = ID_TRUE;
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        }
                    }

                    if ( sIsFound == ID_FALSE )
                    {
                        *aExecDefaultNode = ID_TRUE;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    /*
                     * CASE 3 : ( mValueCount > 0 && mSubValueCount == 0 )
                     *
                     * value의 range index에 해당하는 노드들이 수행대상이 된다.
                     *
                     */
                    for ( i = 0; i < sFirstRangeIndexCount; i++ )
                    {
                        aRangeIndex[*aRangeIndexCount] = sFirstRangeIndex[i].mRangeIndex;
                        (*aRangeIndexCount)++;

                        sIsFound = ID_TRUE;
                    }

                    /* BUG-45738 */
                    if ( aShardAnalysis->mSubKeyExists == ID_TRUE )
                    {
                        *aExecDefaultNode = ID_TRUE;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
            else
            {
                /*
                 * CASE 4 : ( mValueCount == 0 && mSubValueCount > 0 )
                 *
                 * sub value의 range index에 해당하는 노드들이 수행대상이 된다.
                 *
                 */
                for ( j = 0; j < sSecondRangeIndexCount; j++ )
                {
                    aRangeIndex[*aRangeIndexCount] = sSecondRangeIndex[j].mRangeIndex;
                    (*aRangeIndexCount)++;

                    *aExecDefaultNode = ID_TRUE;

                    sIsFound = ID_TRUE;
                }
            }

            if ( sIsFound == ID_FALSE )
            {
                *aExecDefaultNode = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::setPrepareSelected( sdiClientInfo    * aClientInfo,
                                sdiDataNodes     * aDataInfo,
                                idBool             aAllNodes,
                                UInt               aNodeId )
{
    sdiConnectInfo * sConnectInfo = NULL;
    sdiDataNode    * sDataNode = NULL;
    UInt             i = 0;
    ULong            sTargetSMN = SDI_NULL_SMN;

    IDE_DASSERT( aClientInfo != NULL );

    sConnectInfo = aClientInfo->mConnectInfo;
    sDataNode    = aDataInfo->mNodes;

    sTargetSMN   = aClientInfo->mTargetShardMetaNumber;

    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( ( aAllNodes == ID_TRUE ) ||
             ( sConnectInfo->mNodeId == aNodeId ) )
        {
            //-------------------------------
            // shard statement 준비
            //-------------------------------

            if ( sDataNode->mState == SDI_NODE_STATE_PREPARED )
            {
                IDE_TEST( sdl::freeStmt( sConnectInfo,
                                         sDataNode->mRemoteStmt,
                                         SQL_CLOSE,
                                         &(sConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );

                sDataNode->mState = SDI_NODE_STATE_EXECUTE_CANDIDATED;
            }
            else if ( sDataNode->mState == SDI_NODE_STATE_EXECUTED )
            {
                sDataNode->mState = SDI_NODE_STATE_EXECUTE_CANDIDATED;
            }
            else
            {
                // Nothing to do.
            }

            //-------------------------------
            // 후보 선정
            //-------------------------------

            if ( sDataNode->mState == SDI_NODE_STATE_PREPARE_CANDIDATED )
            {
                IDE_TEST_RAISE( sConnectInfo->mNodeInfo.mSMN != sTargetSMN,
                                ERR_UNSET_NODE_SELECTED );

                sDataNode->mState = SDI_NODE_STATE_PREPARE_SELECTED;
            }
            else if ( sDataNode->mState == SDI_NODE_STATE_EXECUTE_CANDIDATED )
            {
                sDataNode->mState = SDI_NODE_STATE_EXECUTE_SELECTED;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNSET_NODE_SELECTED )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_INVALID_NODE_NAME2,
                                  sConnectInfo->mNodeInfo.mNodeName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::prepare( qcTemplate       * aTemplate,
                     sdiClientInfo    * aClientInfo,
                     sdiDataNodes     * aDataInfo,
                     qcNamePosition   * aShardQuery )
{
    sdiConnectInfo * sConnectInfo = NULL;
    sdiDataNode    * sDataNode    = NULL;
    void           * sCallback    = NULL;
    void           * sData        = NULL;
    idBool           sSuccess     = ID_TRUE;
    UInt             i = 0;
    UInt             j = 0;
    smSCN            sSCN;
    idBool           sNeedUpdateSCN = ID_FALSE;
    idBool           sIsAlreadyCheck = ID_FALSE;

    /* TASK-7219 Non-shard DML */
    sdiShardPartialExecType sPartialExecType = SDI_SHARD_PARTIAL_EXEC_TYPE_NONE;

    IDE_DASSERT( aClientInfo != NULL );

    sConnectInfo = aClientInfo->mConnectInfo;
    sDataNode    = aDataInfo->mNodes;
    
    SM_INIT_SCN( &sSCN );

    /* TASK-7218 */
    ideErrorCollectionClear();

    /* TASK-7219 Non-shard DML */
    sPartialExecType = aTemplate->shardExecData.partialExecType;

    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        //-------------------------------
        // shard statement 초기화
        //-------------------------------

        if ( sDataNode->mState == SDI_NODE_STATE_PREPARE_SELECTED )
        {
            sNeedUpdateSCN = (sConnectInfo->mDbc == NULL) ? ID_TRUE : ID_FALSE;

            // open shard connection
            IDE_TEST( qdkOpenShardConnection( sConnectInfo )
                      != IDE_SUCCESS );

            /* PROJ-2733-DistTxInfo Connection 이후 각 노드 SCN을 얻어 온다. */
            if ( sNeedUpdateSCN == ID_TRUE )
            {
                IDE_TEST( sdl::getSCN( sConnectInfo, &sSCN ) != IDE_SUCCESS );

                SM_SET_MAX_SCN( &(aClientInfo->mGCTxInfo.mCoordSCN), &sSCN );
            }
        }
        else
        {
            // Nothing to do.
        }

        if (( sDataNode->mState == SDI_NODE_STATE_PREPARE_SELECTED ) ||
            ( sDataNode->mState == SDI_NODE_STATE_EXECUTE_SELECTED ))
        {
            IDE_TEST( checkTargetSMN( aClientInfo, sConnectInfo ) != IDE_SUCCESS );

            if (( sConnectInfo->mFlag & SDI_CONNECT_PLANATTR_CHANGE_MASK )
                == SDI_CONNECT_PLANATTR_CHANGE_TRUE )
            {
                IDE_TEST( sdl::setConnAttr( sConnectInfo,
                                            SDL_ALTIBASE_EXPLAIN_PLAN,
                                            (sConnectInfo->mPlanAttr > 0) ?
                                            SDL_EXPLAIN_PLAN_ON :
                                            SDL_EXPLAIN_PLAN_OFF,
                                            NULL,
                                            &(sConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );

                sConnectInfo->mFlag &= ~SDI_CONNECT_PLANATTR_CHANGE_MASK;
                sConnectInfo->mFlag |= SDI_CONNECT_PLANATTR_CHANGE_FALSE;
            }
            else
            {
                // nothing to do
            }

            // PROJ-2727
            IDE_TEST( setShardSessionProperty( aTemplate->stmt->session,
                                               aClientInfo,
                                               sConnectInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        if ( sDataNode->mState == SDI_NODE_STATE_PREPARE_SELECTED )
        {
            if ( sIsAlreadyCheck == ID_FALSE )
            {
                IDE_TEST( qdkCheckGlobalTransactionStatus( sConnectInfo ) != IDE_SUCCESS );
                sIsAlreadyCheck = ID_TRUE;
            }

            IDE_TEST( sdl::allocStmt( sConnectInfo,
                                      sDataNode->mRemoteStmt,
                                      sPartialExecType,
                                      &(sConnectInfo->mLinkFailure) )
                      != IDE_SUCCESS );

            IDE_TEST( sdl::addPrepareCallback( &sCallback,
                                               i,
                                               sConnectInfo,
                                               sDataNode->mRemoteStmt,
                                               aShardQuery->stmtText +
                                               aShardQuery->offset,
                                               aShardQuery->size,
                                               &(sConnectInfo->mLinkFailure) )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    // BUG-47765
    if (( aTemplate->stmt->session->mQPSpecific.mFlag & QC_SESSION_ATTR_CHANGE_MASK )
         == QC_SESSION_ATTR_CHANGE_TRUE )
    {
        sConnectInfo = aClientInfo->mConnectInfo;
        
        for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++ )
        {
            if (( sConnectInfo->mFlag & SDI_CONNECT_ATTR_CHANGE_MASK )
                 == SDI_CONNECT_ATTR_CHANGE_TRUE )
            {
                // 전 노드에 프로퍼티가 전파 되지 않음
                aTemplate->stmt->session->mQPSpecific.mFlag &= ~QC_SESSION_ATTR_SET_NODE_MASK;
                aTemplate->stmt->session->mQPSpecific.mFlag |= QC_SESSION_ATTR_SET_NODE_FALSE;

                break;
            }
            else
            {
                // nothing to do
            }

            if (( aClientInfo->mCount -1 ) == i )
            {
                // 전 노드에 프로퍼티가 전파 됨
                // PROJ-2727
                unSetSessionPropertyFlag( aTemplate->stmt->session );
            }
        }
    }     
            
    // PROJ-2670 nested execution
    sdl::doCallback( sCallback );

    sConnectInfo = aClientInfo->mConnectInfo;
    sDataNode    = aDataInfo->mNodes;

    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sDataNode->mState == SDI_NODE_STATE_PREPARE_SELECTED )
        {
            if ( sdl::resultCallback( sCallback,
                                      i,
                                      sConnectInfo,
                                      ID_FALSE,
                                      aClientInfo,
                                      SQL_HANDLE_STMT,
                                      sDataNode->mRemoteStmt->mStmt,
                                      (SChar*)"SQLPrepare",
                                      &(sConnectInfo->mLinkFailure) )
                 == IDE_SUCCESS )
            {
                // bind를 해야한다.
                sDataNode->mBindParamChanged = ID_TRUE;
                sDataNode->mExecCount = 0;
                sDataNode->mState = SDI_NODE_STATE_PREPARED;
                sDataNode->mState = SDI_NODE_STATE_EXECUTE_CANDIDATED;
                sDataNode->mState = SDI_NODE_STATE_EXECUTE_SELECTED;
            }
            else
            {
                sSuccess = ID_FALSE;
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST( sSuccess == ID_FALSE );

    sConnectInfo = aClientInfo->mConnectInfo;
    sDataNode    = aDataInfo->mNodes;

    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        //-------------------------------
        // shard statement 재바인드
        //-------------------------------

        if ( ( sDataNode->mState == SDI_NODE_STATE_EXECUTE_SELECTED ) &&
             ( sDataNode->mBindParamChanged == ID_TRUE ) )
        {
            for ( j = 0; j < sDataNode->mBindParamCount; j++ )
            {
                /* PROJ-2728 Sharding LOB */
                if ( sDataNode->mOutBindParams[j].mShadowData != NULL )
                {
                    sData = SDI_GET_SHADOW_DATA( sDataNode, i, j );
                }
                else
                {
                    sData = sDataNode->mBindParams[j].mData;
                }
                IDE_TEST( sdl::bindParam(
                              sConnectInfo,
                              sDataNode->mRemoteStmt,
                              sDataNode->mBindParams[j].mId,
                              sDataNode->mBindParams[j].mInoutType,
                              sDataNode->mBindParams[j].mType,
                              sData,
                              sDataNode->mBindParams[j].mDataSize,
                              sDataNode->mBindParams[j].mPrecision,
                              sDataNode->mBindParams[j].mScale,
                              &(sDataNode->mOutBindParams[j].mIndicator),
                              &(sConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }

            sDataNode->mBindParamChanged = ID_FALSE;
        }
        else
        {
            // Nothing to do.
        }
    }

    sdl::removeCallback( sCallback );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    sConnectInfo = aClientInfo->mConnectInfo;
    sDataNode = aDataInfo->mNodes;

    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sDataNode->mState == SDI_NODE_STATE_PREPARE_SELECTED )
        {
            sDataNode->mState = SDI_NODE_STATE_PREPARE_CANDIDATED;
        }
        else if ( sDataNode->mState == SDI_NODE_STATE_EXECUTE_SELECTED )
        {
            sDataNode->mState = SDI_NODE_STATE_EXECUTE_CANDIDATED;
        }
        else
        {
            // Nothing to do.
        }
    }

    // PROJ-2727
    unSetSessionPropertyFlag( aTemplate->stmt->session );
            
    sdl::removeCallback( sCallback );

    return IDE_FAILURE;
}

IDE_RC sdi::executeDML( qcStatement    * aStatement,
                        sdiClientInfo  * aClientInfo,
                        sdiDataNodes   * aDataInfo,
                        qmxLobInfo     * aLobInfo,
                        vSLong         * aNumRows )
{
    sdiConnectInfo * sConnectInfo = NULL;
    sdiDataNode    * sDataNode    = NULL;
    void           * sCallback    = NULL;
    idBool           sSuccess     = ID_TRUE;
    vSLong           sNumRows = 0;
    SInt             i;

    sConnectInfo = aClientInfo->mConnectInfo;
    sDataNode    = aDataInfo->mNodes;

    /* TASK-7218 */
    ideErrorCollectionClear();

    /* PROJ-2733-DistTxInfo */
    IDE_TEST( propagateDistTxInfoToNodes( aStatement, aClientInfo, aDataInfo ) != IDE_SUCCESS );

    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sDataNode->mState == SDI_NODE_STATE_EXECUTE_SELECTED )
        {
            /* TASK-7219 Non-shard DML */
            IDE_TEST( sdl::setStmtExecSeq( sConnectInfo, QCG_GET_SESSION_STMT_EXEC_SEQ_FOR_SHARD_TX(aStatement) ) != IDE_SUCCESS );

            // add shard transaction
            IDE_TEST( qdkAddShardTransaction(
                          aStatement->mStatistics,
                          QC_SMI_STMT(aStatement)->mTrans->getTransID(),
                          aClientInfo,
                          sConnectInfo )
                      != IDE_SUCCESS );

            // TASK-7244 PSM partial rollback in Sharding
            if ( ((sConnectInfo->mFlag & SDI_CONNECT_PSM_SVP_SET_MASK) == SDI_CONNECT_PSM_SVP_SET_FALSE) &&
                 (qciMisc::isBeginSP(aStatement) == ID_TRUE) )
            {
                IDE_TEST( sdl::setSavepoint( sConnectInfo,
                                             SAVEPOINT_FOR_SHARD_GLOBAL_PROC_PARTIAL_ROLLBACK )
                          != IDE_SUCCESS );
                sConnectInfo->mFlag &= ~SDI_CONNECT_PSM_SVP_SET_MASK;
                sConnectInfo->mFlag |= SDI_CONNECT_PSM_SVP_SET_TRUE;
            }

            IDE_TEST( sdl::addExecuteCallback( &sCallback,
                                               i,
                                               sConnectInfo,
                                               sDataNode,
                                               &(sConnectInfo->mLinkFailure) )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST_CONT( sCallback == NULL, NORMAL_EXIT );

    IDU_FIT_POINT( "sdi::executeDML" );

    // PROJ-2670 nested execution
    sdl::doCallback( sCallback );

    // add shard tx 순서의 반대로 del shard tx를 수행해야한다.
    for ( i--, sConnectInfo--, sDataNode--;
          i >= 0;
          i--, sConnectInfo--, sDataNode-- )
    {
        if ( sDataNode->mState == SDI_NODE_STATE_EXECUTE_SELECTED )
        {
            // 수행전 touch count 증가
            sConnectInfo->mTouchCount++;

            if ( sdl::resultCallback( sCallback,
                                      i,
                                      sConnectInfo,
                                      ID_FALSE,
                                      aClientInfo,
                                      SQL_HANDLE_STMT,
                                      sDataNode->mRemoteStmt->mStmt,
                                      (SChar*)"SQLExecute",
                                      &(sConnectInfo->mLinkFailure) )
                 == IDE_SUCCESS )
            {
                /* BUG-47459 */
                if ( sDataNode->mSVPStep == SDI_SVP_STEP_PROCEDURE_SAVEPOINT )
                {
                    sDataNode->mSVPStep = SDI_SVP_STEP_SET_SAVEPOINT;
                }

                // 수행후
                sDataNode->mState = SDI_NODE_STATE_EXECUTED;
                sDataNode->mExecCount++;

                if ( sdl::rowCount( sConnectInfo,
                                    sDataNode->mRemoteStmt,
                                    &sNumRows,
                                    &(sConnectInfo->mLinkFailure) )
                     == IDE_SUCCESS )
                {
                    // result row count
                    (*aNumRows) += sNumRows;
                    sConnectInfo->mAffectedRowCount = sNumRows;

                    /* PROJ-2728 Sharding LOB */
                    if ( aLobInfo != NULL && sNumRows > 0 )
                    {
                        if ( qmxShard::copyAndOutBindLobInfo( aStatement,
                                                              sConnectInfo,
                                                              i,
                                                              aLobInfo,
                                                              sDataNode )
                             != IDE_SUCCESS )
                        {
                            sSuccess = ID_FALSE;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // 수행이 실패한 경우
                sDataNode->mState = SDI_NODE_STATE_EXECUTE_CANDIDATED;

                sSuccess = ID_FALSE;
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    /* PROJ-2733-DistTxInfo
     * !!! sdl::doCallback 함수를 수행했으면 sSuccess 값에 상관없이
     *     updateMaxNodeSCNToCoordSCN 함수를 수행해야 한다
     */
    IDE_TEST( updateMaxNodeSCNToCoordSCN( aStatement, aClientInfo, aDataInfo ) != IDE_SUCCESS );

    IDE_TEST( sSuccess == ID_FALSE );

    (void) qmxShard::closeLobLocatorForCopy( aStatement->mStatistics,
                                             aLobInfo );

    sdl::removeCallback( sCallback );

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    processExecuteError( aStatement,
                         aClientInfo,
                         aDataInfo->mNodes,
                         ID_FALSE );

    (void) qmxShard::closeLobLocatorForCopy( aStatement->mStatistics,
                                             aLobInfo );

    sdl::removeCallback( sCallback );

    return IDE_FAILURE;
}

IDE_RC sdi::executeInsert( qcStatement    * aStatement,
                           sdiClientInfo  * aClientInfo,
                           sdiDataNodes   * aDataInfo,
                           qmxLobInfo     * aLobInfo,
                           vSLong         * aNumRows )
{
    sdiConnectInfo * sConnectInfo = NULL;
    sdiDataNode    * sDataNode    = NULL;
    void           * sCallback    = NULL;
    idBool           sSuccess     = ID_TRUE;
    SInt             i;

    sConnectInfo = aClientInfo->mConnectInfo;
    sDataNode    = aDataInfo->mNodes;

    /* TASK-7218 */
    ideErrorCollectionClear();

    /* PROJ-2733-DistTxInfo */
    IDE_TEST( propagateDistTxInfoToNodes( aStatement, aClientInfo, aDataInfo ) != IDE_SUCCESS );

    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sDataNode->mState == SDI_NODE_STATE_EXECUTE_SELECTED )
        {
            /* TASK-7219 Non-shard DML */
            IDE_TEST( sdl::setStmtExecSeq( sConnectInfo, QCG_GET_SESSION_STMT_EXEC_SEQ_FOR_SHARD_TX(aStatement) ) != IDE_SUCCESS );

            // add shard transaction
            IDE_TEST( qdkAddShardTransaction(
                          aStatement->mStatistics,
                          QC_SMI_STMT(aStatement)->mTrans->getTransID(),
                          aClientInfo,
                          sConnectInfo )
                      != IDE_SUCCESS );

            // TASK-7244 PSM partial rollback in Sharding
            if ( ((sConnectInfo->mFlag & SDI_CONNECT_PSM_SVP_SET_MASK) == SDI_CONNECT_PSM_SVP_SET_FALSE) &&
                 (qciMisc::isBeginSP(aStatement) == ID_TRUE) )
            {
                IDE_TEST( sdl::setSavepoint( sConnectInfo,
                                             SAVEPOINT_FOR_SHARD_GLOBAL_PROC_PARTIAL_ROLLBACK )
                          != IDE_SUCCESS );
                sConnectInfo->mFlag &= ~SDI_CONNECT_PSM_SVP_SET_MASK;
                sConnectInfo->mFlag |= SDI_CONNECT_PSM_SVP_SET_TRUE;
            }

            if ( sDataNode->mSVPStep == SDI_SVP_STEP_NEED_SAVEPOINT )
            {
                IDE_TEST( sdl::setSavepoint( sConnectInfo,
                            SAVEPOINT_FOR_SHARD_STMT_PARTIAL_ROLLBACK )
                        != IDE_SUCCESS );

                sDataNode->mSVPStep = SDI_SVP_STEP_SET_SAVEPOINT;

            }

            IDE_TEST( sdl::addExecuteCallback( &sCallback,
                        i,
                        sConnectInfo,
                        sDataNode,
                        &(sConnectInfo->mLinkFailure) )
                    != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST_CONT( sCallback == NULL, NORMAL_EXIT );

    IDU_FIT_POINT( "sdi::executeInsert" );

    // PROJ-2670 nested execution
    sdl::doCallback( sCallback );

    // add shard tx 순서의 반대로 del shard tx를 수행해야한다.
    for ( i--, sConnectInfo--, sDataNode--;
          i >= 0;
          i--, sConnectInfo--, sDataNode-- )
    {
        if ( sDataNode->mState == SDI_NODE_STATE_EXECUTE_SELECTED )
        {
            // 수행전 touch count 증가
            sConnectInfo->mTouchCount++;

            if ( sdl::resultCallback( sCallback,
                                      i,
                                      sConnectInfo,
                                      ID_FALSE,
                                      aClientInfo,
                                      SQL_HANDLE_STMT,
                                      sDataNode->mRemoteStmt->mStmt,
                                      (SChar*)"SQLExecute",
                                      &(sConnectInfo->mLinkFailure) )
                 == IDE_SUCCESS )
            {
                // 수행후
                sDataNode->mState = SDI_NODE_STATE_EXECUTED;
                sDataNode->mExecCount++;

                // result row count
                (*aNumRows)++;
                sConnectInfo->mAffectedRowCount = 1;

                /* PROJ-2728 Sharding LOB */
                if ( aLobInfo != NULL )
                {
                    if ( qmxShard::copyAndOutBindLobInfo( aStatement,
                                                          sConnectInfo,
                                                          i,
                                                          aLobInfo,
                                                          sDataNode )
                         != IDE_SUCCESS )
                    {
                        sSuccess = ID_FALSE;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // 수행이 실패한 경우
                sDataNode->mState = SDI_NODE_STATE_EXECUTE_CANDIDATED;

                sSuccess = ID_FALSE;
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    /* PROJ-2733-DistTxInfo
     * !!! sdl::doCallback 함수를 수행했으면 sSuccess 값에 상관없이
     *     updateMaxNodeSCNToCoordSCN 함수를 수행해야 한다
     */
    IDE_TEST( updateMaxNodeSCNToCoordSCN( aStatement, aClientInfo, aDataInfo ) != IDE_SUCCESS );

    IDE_TEST( sSuccess == ID_FALSE );

    (void) qmxShard::closeLobLocatorForCopy( aStatement->mStatistics,
                                             aLobInfo );

    sdl::removeCallback( sCallback );

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    processExecuteError( aStatement,
                         aClientInfo,
                         aDataInfo->mNodes,
                         ID_FALSE );

    (void) qmxShard::closeLobLocatorForCopy( aStatement->mStatistics,
                                             aLobInfo );

    sdl::removeCallback( sCallback );

    return IDE_FAILURE;
}

IDE_RC sdi::executeSelect( qcStatement    * aStatement,
                           sdiClientInfo  * aClientInfo,
                           sdiDataNodes   * aDataInfo )
{
    sdiConnectInfo * sConnectInfo = NULL;
    sdiDataNode    * sDataNode    = NULL;
    void           * sCallback    = NULL;
    idBool           sSuccess     = ID_TRUE;
    SInt             i;

    sConnectInfo = aClientInfo->mConnectInfo;
    sDataNode    = aDataInfo->mNodes;

    /* TASK-7218 */
    ideErrorCollectionClear();

    /* PROJ-2733-DistTxInfo */
    IDE_TEST( propagateDistTxInfoToNodes( aStatement, aClientInfo, aDataInfo ) != IDE_SUCCESS );

    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sDataNode->mState == SDI_NODE_STATE_EXECUTE_SELECTED )
        {
            /* TASK-7219 Non-shard DML */
            IDE_TEST( sdl::setStmtExecSeq( sConnectInfo, QCG_GET_SESSION_STMT_EXEC_SEQ_FOR_SHARD_TX(aStatement) ) != IDE_SUCCESS );

            // add shard transaction
            IDE_TEST( qdkAddShardTransaction(
                          aStatement->mStatistics,
                          QC_SMI_STMT(aStatement)->mTrans->getTransID(),
                          aClientInfo,
                          sConnectInfo )
                      != IDE_SUCCESS );

            IDE_TEST( sdl::addExecuteCallback( &sCallback,
                                               i,
                                               sConnectInfo,
                                               sDataNode,
                                               &(sConnectInfo->mLinkFailure) )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST_CONT( sCallback == NULL, NORMAL_EXIT );

    IDU_FIT_POINT( "sdi::executeSelect" );

    // PROJ-2670 nested execution
    sdl::doCallback( sCallback );

    // add shard tx 순서의 반대로 del shard tx를 수행해야한다.
    for ( i--, sConnectInfo--, sDataNode--;
          i >= 0;
          i--, sConnectInfo--, sDataNode-- )
    {
        if ( sDataNode->mState == SDI_NODE_STATE_EXECUTE_SELECTED )
        {
            if ( sdl::resultCallback( sCallback,
                                      i,
                                      sConnectInfo,
                                      ID_FALSE,
                                      aClientInfo,
                                      SQL_HANDLE_STMT,
                                      sDataNode->mRemoteStmt->mStmt,
                                      (SChar*)"SQLExecute",
                                      &(sConnectInfo->mLinkFailure) )
                 == IDE_SUCCESS )
            {
                // 수행후
                sDataNode->mState = SDI_NODE_STATE_EXECUTED;
                sDataNode->mExecCount++;
            }
            else
            {
                // 수행이 실패한 경우
                sDataNode->mState = SDI_NODE_STATE_EXECUTE_CANDIDATED;

                sSuccess = ID_FALSE;
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    /* PROJ-2733-DistTxInfo
     * !!! sdl::doCallback 함수를 수행했으면 sSuccess 값에 상관없이
     *     updateMaxNodeSCNToCoordSCN 함수를 수행해야 한다
     */
    IDE_TEST( updateMaxNodeSCNToCoordSCN( aStatement, aClientInfo, aDataInfo ) != IDE_SUCCESS );

    IDE_TEST( sSuccess == ID_FALSE );

    sdl::removeCallback( sCallback );

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    processExecuteError( aStatement,
                         aClientInfo,
                         aDataInfo->mNodes,
                         ID_TRUE );

    sdl::removeCallback( sCallback );

    return IDE_FAILURE;
}

IDE_RC sdi::fetch( sdiConnectInfo  * aConnectInfo,
                   sdiDataNode     * aDataNode,
                   idBool          * aExist )
{
    SShort  sResult = ID_SSHORT_MAX;

    IDE_TEST( sdl::fetch( aConnectInfo,
                          aDataNode->mRemoteStmt,
                          &sResult,
                          &(aConnectInfo->mLinkFailure) )
              != IDE_SUCCESS );

    if ( ( sResult == SQL_SUCCESS ) || ( sResult == SQL_SUCCESS_WITH_INFO ) )
    {
        *aExist = ID_TRUE;
    }
    else
    {
        IDE_DASSERT( sResult == SQL_NO_DATA_FOUND );

        *aExist = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::getPlan( sdiConnectInfo  * aConnectInfo,
                     sdiDataNode     * aDataNode )
{
    IDE_TEST( sdl::getPlan( aConnectInfo,
                            aDataNode->mRemoteStmt,
                            &(aDataNode->mPlanText),
                            &(aConnectInfo->mLinkFailure) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void sdi::charXOR( SChar * aText, UInt aLen )
{
    const UChar sEnc[8] = { 172, 134, 132, 122, 114, 109, 117, 134 };
    SChar     * sText;
    UInt        sIndex;
    UInt        i;

    sText = aText;

    for ( i = 0, sIndex = 0; i < aLen; i++ )
    {
        if ( sText[i] != 0 )
        {
            sText[i] ^= sEnc[sIndex];
        }
        else
        {
            // Nothing to do.
        }

        if ( sIndex < 7 )
        {
            sIndex++;
        }
        else
        {
            sIndex = 0;
        }
    }
}

IDE_RC sdi::printMessage( SChar * aMessage,
                          UInt    aLength,
                          void  * aArgument )
{
    sdiConnectInfo * sConnectInfo = (sdiConnectInfo*)aArgument;

    (void) qci::mSessionCallback.mPrintToClient(
        sConnectInfo->mSession->mMmSession,
        (UChar*)aMessage,
        aLength );

    return IDE_SUCCESS;
}

void sdi::setShardMetaTouched( qcSession * aSession )
{
    if ( ( aSession->mQPSpecific.mFlag & QC_SESSION_SHARD_META_TOUCH_MASK ) ==
         QC_SESSION_SHARD_META_TOUCH_FALSE )
    {
        aSession->mQPSpecific.mFlag &= ~QC_SESSION_SHARD_META_TOUCH_MASK;
        aSession->mQPSpecific.mFlag |= QC_SESSION_SHARD_META_TOUCH_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    // 현재 세션에서 plan cache 사용금지
    if ( ( aSession->mQPSpecific.mFlag & QC_SESSION_PLAN_CACHE_MASK ) ==
         QC_SESSION_PLAN_CACHE_ENABLE )
    {
        aSession->mQPSpecific.mFlag &= ~QC_SESSION_PLAN_CACHE_MASK;
        aSession->mQPSpecific.mFlag |= QC_SESSION_PLAN_CACHE_DISABLE;
    }
    else
    {
        // Nothing to do.
    }
}

void sdi::unsetShardMetaTouched( qcSession * aSession )
{
    if ( ( aSession->mQPSpecific.mFlag & QC_SESSION_SHARD_META_TOUCH_MASK ) ==
         QC_SESSION_SHARD_META_TOUCH_TRUE )
    {
        aSession->mQPSpecific.mFlag &= ~QC_SESSION_SHARD_META_TOUCH_MASK;
        aSession->mQPSpecific.mFlag |= QC_SESSION_SHARD_META_TOUCH_FALSE;
    }
    else
    {
        // Nothing to do.
    }

    if ( ( aSession->mQPSpecific.mFlag & QC_SESSION_PLAN_CACHE_MASK ) ==
         QC_SESSION_PLAN_CACHE_DISABLE )
    {
        aSession->mQPSpecific.mFlag &= ~QC_SESSION_PLAN_CACHE_MASK;
        aSession->mQPSpecific.mFlag |= QC_SESSION_PLAN_CACHE_ENABLE;
    }
    else
    {
        // Nothing to do.
    }
}

/* BUG-48586 */
void sdi::setInternalTableSwap( qcSession * aSession )
{
    IDE_DASSERT( isShardEnable() == ID_TRUE );

    if ( ( aSession->mQPSpecific.mFlag & QC_SESSION_INTERNAL_TABLE_SWAP_MASK ) ==
         QC_SESSION_INTERNAL_TABLE_SWAP_FALSE )
    {
        aSession->mQPSpecific.mFlag &= ~QC_SESSION_INTERNAL_TABLE_SWAP_MASK;
        aSession->mQPSpecific.mFlag |= QC_SESSION_INTERNAL_TABLE_SWAP_TRUE;
    }
}

void sdi::unsetInternalTableSwap( qcSession * aSession )
{
    IDE_DASSERT( isShardEnable() == ID_TRUE );

    if ( ( aSession->mQPSpecific.mFlag & QC_SESSION_INTERNAL_TABLE_SWAP_MASK ) ==
         QC_SESSION_INTERNAL_TABLE_SWAP_TRUE )
    {
        aSession->mQPSpecific.mFlag &= ~QC_SESSION_INTERNAL_TABLE_SWAP_MASK;
        aSession->mQPSpecific.mFlag |= QC_SESSION_INTERNAL_TABLE_SWAP_FALSE;
    }
}

IDE_RC sdi::touchShardNode( qcSession * aSession,
                            idvSQL    * aStatistics,
                            smTID       aTransID,
                            UInt        aNodeId )
{
    sdiClientInfo    * sClientInfo = NULL;
    sdiConnectInfo   * sConnectInfo = NULL;
    idBool             sFound = ID_FALSE;
    UShort             i = 0;

    IDE_DASSERT( aSession != NULL );

    sClientInfo = aSession->mQPSpecific.mClientInfo;

    if ( sClientInfo != NULL )
    {
        sConnectInfo = sClientInfo->mConnectInfo;

        for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
        {
            if ( sConnectInfo->mNodeInfo.mNodeId == aNodeId )
            {
                sFound = ID_TRUE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST_RAISE( sFound == ID_FALSE, ERR_NOT_FOUND );

    // open shard connection
    IDE_TEST( qdkOpenShardConnection( sConnectInfo )
              != IDE_SUCCESS );

    // add shard transaction
    IDE_TEST( qdkAddShardTransaction( aStatistics,
                                      aTransID,
                                      sClientInfo,
                                      sConnectInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::openAllShardConnections( qcSession * aSession )
{
    sdiClientInfo    * sClientInfo  = NULL;
    sdiConnectInfo   * sConnectInfo = NULL;
    UShort             i            = 0;

    IDE_DASSERT( aSession != NULL );

    sClientInfo = aSession->mQPSpecific.mClientInfo;

    if ( sClientInfo != NULL )
    {
        sConnectInfo = sClientInfo->mConnectInfo;

        for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
        {
            (void)qdkOpenShardConnection( sConnectInfo );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

void sdi::xidInitialize( sdiConnectInfo * aConnectInfo )
{
    IDE_DASSERT( aConnectInfo != NULL );

    // null XID
    aConnectInfo->mXID.formatID = -1;
    aConnectInfo->mXID.gtrid_length = 0;
    aConnectInfo->mXID.bqual_length = 0;
}

IDE_RC sdi::addPrepareTranCallback( void           ** aCallback,
                                    sdiConnectInfo  * aNode )
{
    IDE_TEST( sdl::addPrepareTranCallback( aCallback,
                                           aNode->mNodeId,
                                           aNode,
                                           &(aNode->mXID),
                                           &(aNode->mReadOnly),
                                           &(aNode->mLinkFailure) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::addEndPendingTranCallback( void           ** aCallback,
                                       sdiConnectInfo  * aNode,
                                       idBool            aIsCommit )
{
    IDE_TEST( sdl::addEndPendingTranCallback( aCallback,
                                              aNode->mNodeId,
                                              aNode,
                                              &(aNode->mXID),
                                              aIsCommit,
                                              &(aNode->mLinkFailure) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::addEndTranCallback( void           ** aCallback,
                                sdiConnectInfo  * aNode,
                                idBool            aIsCommit )
{
    IDE_TEST( sdl::addEndTranCallback( aCallback,
                                       aNode->mNodeId,
                                       aNode,
                                       aIsCommit,
                                       &(aNode->mLinkFailure) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void sdi::doCallback( void * aCallback )
{
    sdl::doCallback( aCallback );
}

IDE_RC sdi::resultCallback( void           * aCallback,
                            sdiConnectInfo * aNode,
                            SChar          * aFuncName,
                            idBool           aReCall )
{
    IDE_TEST( sdl::resultCallback( aCallback,
                                   aNode->mNodeId,
                                   aNode,
                                   aReCall,
                                   aNode->mSession->mQPSpecific.mClientInfo,
                                   SQL_HANDLE_DBC,
                                   aNode->mDbc,
                                   aFuncName,
                                   &(aNode->mLinkFailure) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void sdi::removeCallback( void * aCallback )
{
    sdl::removeCallback( aCallback );
}

IDE_RC sdi::setFailoverSuspend( qcSession              * aSession,
                                sdiFailoverSuspendType   aSuspendOnType,
                                UInt                     aNewErrorCode )
{
    sdiClientInfo  * sClientInfo    = NULL;
    sdiConnectInfo * sConnectInfo   = NULL;
    SInt             i;

    if ( aSuspendOnType != SDI_FAILOVER_SUSPEND_NONE )
    {
        IDE_DASSERT( aNewErrorCode != idERR_IGNORE_NoError );
    }

    sClientInfo = aSession->mQPSpecific.mClientInfo;
    if ( sClientInfo != NULL )
    {
        sConnectInfo = sClientInfo->mConnectInfo;

        for ( i = 0 ; i < sClientInfo->mCount; i++, sConnectInfo++ )
        {
            if ( sConnectInfo->mDbc != NULL )
            {
                IDE_TEST( sdl::setFailoverSuspend( sConnectInfo,
                                                   aSuspendOnType,
                                                   aNewErrorCode )
                          != IDE_SUCCESS );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::setFailoverSuspend( sdiConnectInfo         * aConnectInfo,
                                sdiFailoverSuspendType   aSuspendOnType,
                                UInt                     aNewErrorCode )
{
    if ( aSuspendOnType != SDI_FAILOVER_SUSPEND_NONE )
    {
        IDE_DASSERT( aNewErrorCode != idERR_IGNORE_NoError );
    }

    IDE_TEST( sdl::setFailoverSuspend( aConnectInfo,
                                       aSuspendOnType,
                                       aNewErrorCode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

sdiShardPin sdi::makeShardPin()
{
    return sdmShardPinMgr::getNewShardPin();
}

void sdi::shardPinToString( SChar *aDst, UInt aLen, sdiShardPin aShardPin )
{
    sdmShardPinMgr::shardPinToString( aDst, aLen, aShardPin );
}

IDE_RC sdi::setCommitMode( qcSession * aSession,
                           idBool      aIsAutoCommit,
                           UInt        /* aDBLinkGTXLevel */,
                           idBool      aIsGTx,
                           idBool      aIsGCTx )
{
    sdiClientInfo  * sClientInfo          = NULL;
    sdiConnectInfo * sConnectInfo         = NULL;
    idBool           sOldIsUserAutoCommit = ID_FALSE;
    idBool           sOldIsMetaAutoCommit = ID_FALSE;
    idBool           sNewIsUserAutoCommit = aIsAutoCommit;
    idBool           sNewIsMetaAutoCommit = aIsAutoCommit;
    UShort           i                    = 0;
    UShort           j                    = 0;

    IDE_DASSERT( aSession != NULL );

    sClientInfo = aSession->mQPSpecific.mClientInfo;
    if ( sClientInfo != NULL )
    {
        // Global Transaction인 경우, Meta Connection은 Non-Autocommit이어야 한다.
        if ( ( aIsGTx == ID_TRUE ) && ( sNewIsMetaAutoCommit == ID_TRUE ) )
        {
            sNewIsMetaAutoCommit = ID_FALSE;
        }
        else
        {
            /* Nothing to do */
        }

        sConnectInfo = sClientInfo->mConnectInfo;
        for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
        {
            sOldIsUserAutoCommit = isUserAutoCommit( sConnectInfo );
            sOldIsMetaAutoCommit = isMetaAutoCommit( sConnectInfo );

            if ( sOldIsMetaAutoCommit != sNewIsMetaAutoCommit )
            {
                if ( sConnectInfo->mDbc != NULL )
                {
                    IDE_TEST( sdl::setConnAttr( sConnectInfo,
                                                SDL_SQL_ATTR_AUTOCOMMIT,
                                                ( sNewIsMetaAutoCommit == ID_TRUE
                                                  ? SDL_COMMITMODE_AUTOCOMMIT
                                                  : SDL_COMMITMODE_NONAUTOCOMMIT ),
                                                NULL,
                                                & sConnectInfo->mLinkFailure )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }

            sConnectInfo->mFlag &= ~SDI_CONNECT_USER_AUTOCOMMIT_MODE_MASK;
            sConnectInfo->mFlag |= ( ( sNewIsUserAutoCommit == ID_TRUE ) ?
                                     SDI_CONNECT_USER_AUTOCOMMIT_MODE_ON :
                                     SDI_CONNECT_USER_AUTOCOMMIT_MODE_OFF );

            sConnectInfo->mFlag &= ~SDI_CONNECT_COORD_AUTOCOMMIT_MODE_MASK;
            sConnectInfo->mFlag |= ( ( sNewIsMetaAutoCommit == ID_TRUE ) ?
                                     SDI_CONNECT_COORD_AUTOCOMMIT_MODE_ON :
                                     SDI_CONNECT_COORD_AUTOCOMMIT_MODE_OFF );
        }

        /* PROJ-2733-DistTxInfo Non-Autocommit 모드로 설정한 경우 초기화 해 주자. */
        if ( sNewIsUserAutoCommit == ID_FALSE )
        {
            #if defined(DEBUG)
            ideLog::log( IDE_SD_18, "[GLOBAL_CONSISTENT_TRANSACTION_DISTTXINFO] =[%s] setCommitMode, endTranDistTx()",
                         sClientInfo->mGCTxInfo.mSessionTypeString );
            #endif

            endTranDistTx( sClientInfo, aIsGCTx );
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    sConnectInfo = sClientInfo->mConnectInfo;
    for ( j = 0; ( j <= i ) && ( j < sClientInfo->mCount ); j++, sConnectInfo++ )
    {
        if ( sOldIsMetaAutoCommit != sNewIsMetaAutoCommit )
        {
            if ( sConnectInfo->mDbc != NULL )
            {
                (void) sdl::setConnAttr( sConnectInfo,
                                         SDL_SQL_ATTR_AUTOCOMMIT,
                                         ( sOldIsMetaAutoCommit == ID_TRUE
                                           ? SDL_COMMITMODE_AUTOCOMMIT
                                           : SDL_COMMITMODE_NONAUTOCOMMIT ),
                                         NULL,
                                         & sConnectInfo->mLinkFailure );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }

        sConnectInfo->mFlag &= ~SDI_CONNECT_USER_AUTOCOMMIT_MODE_MASK;
        sConnectInfo->mFlag |= ( ( sOldIsUserAutoCommit == ID_TRUE ) ?
                                 SDI_CONNECT_USER_AUTOCOMMIT_MODE_ON :
                                 SDI_CONNECT_USER_AUTOCOMMIT_MODE_OFF );

        sConnectInfo->mFlag &= ~SDI_CONNECT_COORD_AUTOCOMMIT_MODE_MASK;
        sConnectInfo->mFlag |= ( ( sOldIsMetaAutoCommit == ID_TRUE ) ?
                                 SDI_CONNECT_COORD_AUTOCOMMIT_MODE_ON :
                                 SDI_CONNECT_COORD_AUTOCOMMIT_MODE_OFF );

        sConnectInfo->mLinkFailure = ID_TRUE;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC sdi::setTransactionLevel( qcSession * aSession,
                                 UInt        aOldDBLinkGTXLevel,
                                 UInt        aNewDBLinkGTXLevel )
{
    sdiClientInfo  * sClientInfo        = NULL;
    sdiConnectInfo * sConnectInfo       = NULL;
    UShort           i                  = 0;
    UShort           j                  = 0;

    IDE_DASSERT( aSession != NULL );

    sClientInfo = aSession->mQPSpecific.mClientInfo;
    if ( sClientInfo != NULL )
    {
        sConnectInfo = sClientInfo->mConnectInfo;
        for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
        {
            if ( aOldDBLinkGTXLevel != aNewDBLinkGTXLevel )
            {
                if ( sConnectInfo->mDbc != NULL )
                {
                    IDE_TEST( sdl::setConnAttr( sConnectInfo,
                                                SDL_ALTIBASE_GLOBAL_TRANSACTION_LEVEL,
                                                aNewDBLinkGTXLevel,
                                                NULL,
                                                & sConnectInfo->mLinkFailure )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }
        }

        sClientInfo->mTransactionLevel = aNewDBLinkGTXLevel;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    sConnectInfo = sClientInfo->mConnectInfo;
    for ( j = 0; ( j <= i ) && ( j < sClientInfo->mCount ); j++, sConnectInfo++ )
    {
        if ( aOldDBLinkGTXLevel != aNewDBLinkGTXLevel )
        {
            if ( sConnectInfo->mDbc != NULL )
            {
                (void) sdl::setConnAttr( sConnectInfo,
                                         SDL_ALTIBASE_GLOBAL_TRANSACTION_LEVEL,
                                         aOldDBLinkGTXLevel,
                                         NULL,
                                         & sConnectInfo->mLinkFailure );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }

        sConnectInfo->mLinkFailure = ID_TRUE;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC sdi::setTransactionalDDLMode( qcSession * aSession, idBool aTransactionalDDL )
{
    sdiClientInfo  * sClientInfo        = NULL;
    sdiConnectInfo * sConnectInfo       = NULL;
    UShort           i                  = 0;
    UShort           j                  = 0;

    IDE_DASSERT( aSession != NULL );

    sClientInfo = aSession->mQPSpecific.mClientInfo;
    IDE_TEST_RAISE( sClientInfo == NULL, ERR_CONNECT_NODES );

    sConnectInfo = sClientInfo->mConnectInfo;
    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
    {
        if ( sConnectInfo->mDbc != NULL )
        {
            IDE_TEST( sdl::setConnAttr( sConnectInfo,
                                        SDL_ALTIBASE_TRANSACTIONAL_DDL,
                                        aTransactionalDDL,
                                        NULL,
                                        &(sConnectInfo->mLinkFailure) )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONNECT_NODES )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::setTransactionalDDLMode",
                                  "Need Client Info" ) );

    }
    IDE_EXCEPTION_END;

    if ( sClientInfo != NULL )
    {
        sConnectInfo = sClientInfo->mConnectInfo;
        for ( j = 0; j < i; j++, sConnectInfo++ )
        {
            if ( sConnectInfo->mDbc != NULL )
            {
                (void)sdl::setConnAttr( sConnectInfo,
                                        SDL_ALTIBASE_TRANSACTIONAL_DDL,
                                        !aTransactionalDDL,
                                        NULL,
                                        &(sConnectInfo->mLinkFailure) );
            }
        }
    }

    return IDE_FAILURE;
}

/* BUG-45967 Rebuild Data 완료 대기 */
IDE_RC sdi::waitToRebuildData( idvSQL * aStatistics )
{
    const SChar * sName     = (const SChar *)"SHARD_REBUILD_DATA_STEP";
    SChar         sValue[2] = { '2', '\0' };

    // SHARD_REBUILD_DATA_STEP이 1 이면, 2 로 갱신
    if ( SDU_SHARD_REBUILD_DATA_STEP == 1 )
    {
        // BUG-19498 value range check
        IDE_TEST( idp::validate( sName, sValue,
                                 ID_TRUE )  // isSystem
                  != IDE_SUCCESS );

        IDE_TEST( idp::update( NULL, sName, sValue, 0, NULL ) != IDE_SUCCESS );

        ideLog::log( IDE_SD_31, "[DEBUG] Set Property Internal. SHARD_REBUILD_DATA_STEP=[2]\n" );
    }
    else
    {
        /* Nothing to do */
    }

    // SHARD_REBUILD_DATA_STEP이 0 이 될 때까지 대기
    while ( SDU_SHARD_REBUILD_DATA_STEP != 0 )
    {
        idlOS::sleep( 1 );

        IDE_TEST( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-45899 */
void sdi::setNonShardQueryReason( sdiPrintInfo * aPrintInfo,
                                  UShort         aReason )
{
    sda::setNonShardQueryReason( aPrintInfo, aReason );
}

SChar * sdi::getNonShardQueryReasonArr( UShort aArrIdx )
{
    return gNonShardQueryReason[aArrIdx];
}

idBool sdi::isGCTxPlanPrintable( qcStatement * aStatement )
{
    idBool sResult = ID_FALSE;

    if ( QCG_GET_SESSION_IS_GCTX( aStatement ) == ID_TRUE )
    {
        sResult = ID_TRUE;
    }

    return sResult;
}

idBool sdi::isAnalysisInfoPrintable( qcStatement * aStatement )
{
    idBool sResult = ID_FALSE;

    if ( ( isShardCoordinator( aStatement ) == ID_TRUE ) &&
         ( qci::getStartupPhase() == QCI_STARTUP_SERVICE ) &&
         ( QCG_GET_SESSION_TRCLOG_DETAIL_SHARD( aStatement ) == 1 ) )
    {
        sResult = ID_TRUE;
    }

    return sResult;
}

void sdi::printGCTxPlan( qcTemplate   * aTemplate,
                         iduVarString * aString )
{
    qcShardExecData * sExecData = NULL;
    const UChar       sYesNoStr[2][4] = { "NO", "YES" };
    UInt              sYesNoIdx = 0;

    if ( ( isAnalysisInfoPrintable( aTemplate->stmt ) == ID_TRUE ) &&
         ( isGCTxPlanPrintable( aTemplate->stmt ) == ID_TRUE ) )
    {
        sExecData = &aTemplate->shardExecData;

        iduVarStringAppend( aString, "[ GLOBAL CONSISTENCY INFO ]\n" );

        sYesNoIdx = ( sExecData->partialStmt == ID_TRUE ) ? 1 : 0;
        iduVarStringAppendFormat( aString,
                                  "%.*s: %s\n",
                                  20, "Partial Statement",
                                  sYesNoStr[sYesNoIdx] );

        if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
        {
            iduVarStringAppendFormat( aString,
                                      "%.*s: %"ID_UINT64_FMT"\n",
                                      20, "Leading Request SCN",
                                      sExecData->leadingRequestSCN );
        }
        else
        {
            iduVarStringAppendFormat( aString,
                                      "%.*s: %s\n",
                                      20, "Leading Request SCN",
                                      (sExecData->leadingRequestSCN == 0)? "0":"NNN");
        }

        if ( sExecData->leadingPlanIndex == ID_UINT_MAX )
        {
            iduVarStringAppendFormat( aString,
                                      "%.*s: -\n",
                                      20, "Leading Plan Index" );
        }
        else
        {
            iduVarStringAppendFormat( aString,
                                      "%.*s: %"ID_UINT32_FMT"\n",
                                      20, "Leading Plan Index",
                                      sExecData->leadingPlanIndex );
        }

        sYesNoIdx = ( sExecData->globalPSM == ID_TRUE ) ? 1 : 0;
        iduVarStringAppendFormat( aString,
                                  "%.*s: %s\n",
                                  20, "Global PSM",
                                  sYesNoStr[sYesNoIdx] );
    }
}

void sdi::printAnalysisInfo( qcStatement  * aStatement,
                             iduVarString * aString )
{
    sdiPrintInfo * sPrintInfo = NULL;

    if ( isAnalysisInfoPrintable( aStatement ) == ID_TRUE )
    {
        sPrintInfo = &(aStatement->mShardPrintInfo);

        iduVarStringAppend( aString, "[ SHARD ANALYSIS INFORMATION ]\n" );

        iduVarStringAppendFormat( aString,
                                  "ANALYSIS COST          : %"ID_UINT32_FMT"\n",
                                  sPrintInfo->mAnalyzeCount );

        if ( sPrintInfo->mQueryType == SDI_QUERY_TYPE_SHARD )
        {
            iduVarStringAppend( aString, "QUERY TYPE             : Shard query\n" );
        }
        else
        {
            if ( sPrintInfo->mQueryType == SDI_QUERY_TYPE_NONSHARD )
            {
                if ( sPrintInfo->mAnalyzeCount > 0 )
                {
                    iduVarStringAppend( aString, "QUERY TYPE             : Non-shard query\n" );
                }
                else
                {
                    // 샤드 객체가 없거나 node[meta]로 지정해서 수행되는 경우 분석 skip
                    iduVarStringAppend( aString, "QUERY TYPE             : -\n" );
                }

                iduVarStringAppend( aString, "NON-SHARD QUERY REASON : ");
                if ( sPrintInfo->mNonShardQueryReason < SDI_NON_SHARD_QUERY_REASON_MAX )
                {
                    iduVarStringAppend( aString, gNonShardQueryReason[sPrintInfo->mNonShardQueryReason] );
                }
                else
                {
                    //IDE_DASSERT(0);
                    iduVarStringAppend( aString, gNonShardQueryReason[SDI_UNKNOWN_REASON] );
                }

                if ( sPrintInfo->mTransformable )
                {
                    iduVarStringAppend( aString, "\nQUERY TRANSFORMABLE    : Yes\n" );
                }
                else
                {
                    iduVarStringAppend( aString, "\nQUERY TRANSFORMABLE    : No\n" );
                }
            }
            else
            {
                //IDE_DASSERT(0);
                iduVarStringAppend( aString, gNonShardQueryReason[SDI_UNKNOWN_REASON] );
                iduVarStringAppend( aString, "\n" );
            }
        }

        iduVarStringAppend( aString,
                            "------------------------------------------------------------\n" );
    }
}

sdiQueryType sdi::getQueryType( qciStatement * aStatement )
{
    // s$statement 로 보여주는 shard query type
    // - trclog_detail_shard 과 무관하게 동작
    // - plan cache 가 적용
    // => 따라서 qcStatement.mShardPrintInfo 가 아닌
    //    qcStatement.myPlan->mShardAnalysis 정보를 우선적으로 제공한다.

    IDE_DASSERT( aStatement != NULL );

    qciShardAnalyzeInfo * sAnalyzeInfo = NULL;
    sdiQueryType          sQueryType = SDI_QUERY_TYPE_NONSHARD;

    if ( ( qci::getStartupPhase() == QCI_STARTUP_SERVICE ) &&
         ( isShardCoordinator( &aStatement->statement ) == ID_TRUE ) )
    {
        if ( qci::getShardAnalyzeInfo( aStatement, &sAnalyzeInfo ) == IDE_SUCCESS )
        {
            sQueryType = ( sAnalyzeInfo->mIsShardQuery == ID_TRUE ) ? SDI_QUERY_TYPE_SHARD: SDI_QUERY_TYPE_NONSHARD;
        }
        else
        {
            if ( aStatement->statement.mShardPrintInfo.mQueryType != SDI_QUERY_TYPE_NONE )
            {
                sQueryType = aStatement->statement.mShardPrintInfo.mQueryType;
            }
        }
    }

    return sQueryType;
}

/* PROJ-2638 */
idBool sdi::isShardEnable()
{
    return ( SDU_SHARD_ENABLE == 1 ? ID_TRUE : ID_FALSE );
}

idBool sdi::isShardCoordinator( qcStatement * aStatement )
{
    idBool sIsCoord = ID_FALSE;

    // R2HA 기존 테스트 케이스 pass를 위해 추가 추후 제거
    if( SDU_SHARD_ZOOKEEPER_TEST == 0 )
    {
        sdi::setShardStatus(1);
    }
    
    // shard enable이어야 하고
    // user connection 이어야 한다.
    // SHARD DDL은 SHARD로 수행 된다.
    if ((( SDU_SHARD_ENABLE == 1 ) &&
         ( QCG_GET_SESSION_SHARD_IN_PSM_ENABLE(aStatement) == ID_TRUE ) &&
         ( sdi::getShardStatus() == 1 )) ||
        (( SDU_SHARD_ENABLE == 1 ) &&
         ( sdi::isShardDDL(aStatement) == ID_TRUE )))
    {
        if ( QCG_GET_SESSION_SHARD_SESSION_TYPE( aStatement ) == SDI_SESSION_TYPE_USER )
        {
            sIsCoord = ID_TRUE;
        }
    }

    return sIsCoord;
}

/* PROJ-2745
 * isRebuildCoordinator 함수의 변형.
 * 더이상 Rebuild coordinator 가 생성되지 않고
 * 해당 함수에서 SMN 변경을 감지하므로
 * 함수 이름을 detectShardMetaChange 로 변경한다.
 */
idBool sdi::detectShardMetaChange( qcStatement * aStatement )
{
    idBool sDetect = ID_FALSE;
    if ( SDU_SHARD_ENABLE == 1 )
    {
        if ( aStatement->session->mMmSession != NULL )
        {
            sDetect =
                qci::mSessionCallback.mDetectShardMetaChange(
                    aStatement->session->mMmSession );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return sDetect;
}

idBool sdi::isPartialCoordinator( qcStatement * aStatement )
{
    idBool   sIsDMLCoord = ID_FALSE;

    /*
     * TASK-7219 Non-shard DML
     *
     * DML coordinator 수행의 조건
     *     1. shard enable이어야 한다.
     *     2. user connection 이 아니어야 한다.
     *     3. partial execution type이 COORD 여야 한다.
     */

    if ( (SDU_SHARD_ENABLE == 1) &&
         (QCG_GET_SESSION_SHARD_IN_PSM_ENABLE(aStatement) == ID_TRUE) )
    {
        if ( QCG_GET_SESSION_SHARD_SESSION_TYPE( aStatement ) != SDI_SESSION_TYPE_USER )
        {
            if ( aStatement->mShardPartialExecType == SDI_SHARD_PARTIAL_EXEC_TYPE_COORD )
            {
                sIsDMLCoord = ID_TRUE;
            }
            else
            {
                /* Nothing to do. */
            }
        }
    }

    return sIsDMLCoord;
}

UInt sdi::getShardInternalConnAttrRetryCount()
{
    return SDU_SHARD_INTERNAL_CONN_ATTR_RETRY_COUNT;
}

UInt sdi::getShardInternalConnAttrRetryDelay()
{
    return SDU_SHARD_INTERNAL_CONN_ATTR_RETRY_DELAY;
}

UInt sdi::getShardInternalConnAttrConnTimeout()
{
    return SDU_SHARD_INTERNAL_CONN_ATTR_CONN_TIMEOUT;
}

UInt sdi::getShardInternalConnAttrLoginTimeout()
{
    return SDU_SHARD_INTERNAL_CONN_ATTR_LOGIN_TIMEOUT;
}

void sdi::closeShardSessionByNodeId( qcSession * aSession,
                                     UInt        aNodeId,
                                     UChar       aDestination )
{
    sdiClientInfo  * sClientInfo        = NULL;
    sdiConnectInfo * sConnectInfo       = NULL;
    sdiNode        * sNodeInfo          = NULL;
    UShort           i                  = 0;
    idBool           sFound             = ID_FALSE;

    sClientInfo = aSession->mQPSpecific.mClientInfo;
    IDE_DASSERT( sClientInfo != NULL );

    sConnectInfo = sClientInfo->mConnectInfo;
    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
    {
        sNodeInfo = &sConnectInfo->mNodeInfo;
        if ( sNodeInfo->mNodeId == aNodeId )
        {
            sFound = ID_TRUE;

            if ( sConnectInfo->mFailoverTarget != aDestination )
            {
                qdkCloseShardConnection( sConnectInfo );
            }
            break;
        }
    }

    if ( sFound != ID_TRUE )
    {
        /* impossible case. */
        ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Not found shard fail-over align destination data node.: node-id %"ID_UINT32_FMT"\n",
                               aNodeId );

        /* PROJ-2755 will fix
         * IDE_DASSERT( 0 );
         */
    }
}

void sdi::setTransactionBroken( idBool     aIsUserAutoCommit,
                                void     * aDkiSession,
                                smiTrans * aTrans )
{
    sdl::setTransactionBrokenByTransactionID( aIsUserAutoCommit,
                                              aDkiSession,
                                              aTrans );
}

idBool sdi::isSupportDataType( UInt aModuleID )
{
    return sdm::isSupportDataType( aModuleID );
}

void sdi::shardStmtPartialRollbackUsingSavepoint( qcStatement    * aStatement,
                                                  sdiClientInfo  * aClientInfo,
                                                  sdiDataNodes   * aDataInfo )
{
    sdiConnectInfo * sConnectInfo = aClientInfo->mConnectInfo;
    sdiDataNode    * sDataNode    = aDataInfo->mNodes;
    SInt             i;

    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sDataNode->mSVPStep == SDI_SVP_STEP_SET_SAVEPOINT )
        {
            sDataNode->mSVPStep = SDI_SVP_STEP_ROLLBACK_TO_SAVEPOINT;
            if ( sdi::rollback( sConnectInfo,
                                getShardSavepointName( aStatement->myPlan->parseTree->stmtKind ) )
                 != IDE_SUCCESS )
            {
                sdi::setTransactionBroken( QCG_GET_SESSION_IS_AUTOCOMMIT( aStatement ),
                                           (void*)QCG_GET_DATABASE_LINK_SESSION( aStatement ),
                                           QC_SMI_STMT(aStatement)->mTrans );
            }
        }
    }
}

IDE_RC sdi::waitAndSetSMNForMetaNode( idvSQL       * aStatistics,
                                      smiStatement * aSmiStmt,
                                      UInt           aFlag,
                                      ULong          aNeededSMN,
                                      ULong        * aDataSMN )
{
    /*
     * PROJ-2701 Online data rebuild
     */
    sdiGlobalMetaInfo sMetaNodeInfo;
    ULong                 sDataSMN = SDI_NULL_SMN;
    UInt                  sSleep = 0;

    smiStatement          sSmiStmt;
    idBool                sIsBeginStmt = ID_FALSE;

    if ( aNeededSMN == SDI_NULL_SMN )
    {
        // get dataSMN
        sDataSMN = getSMNForDataNode();
    }
    else
    {
        sDataSMN = aNeededSMN;
    }

    // metaSMNCache가 아직 갱신되지 않았을 수 있다.
    // Shard meta table에서 shard meta number를 직접 읽는다.
    while( getSMNForMetaNode() < sDataSMN )
    {
        IDE_TEST_RAISE( sSleep > SDU_SHARD_META_PROPAGATION_TIMEOUT,
                        ERR_SHARD_META_PROPAGATION_TIMEOUT );

        IDE_TEST( sSmiStmt.begin( aStatistics,
                                  aSmiStmt,
                                  aFlag)
                  != IDE_SUCCESS );
        sIsBeginStmt = ID_TRUE;
                
        IDE_TEST( sdm::getGlobalMetaInfoCore( &sSmiStmt,
                                                  &sMetaNodeInfo )
                  != IDE_SUCCESS );

        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
        sIsBeginStmt = ID_FALSE;

        if ( sMetaNodeInfo.mShardMetaNumber >= sDataSMN )
        {
            setSMNCacheForMetaNode( sMetaNodeInfo.mShardMetaNumber );
        }
        else
        {
            acpSleepUsec(1000);
            sSleep += 1000;
        }
    }

    *aDataSMN = sDataSMN;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_META_PROPAGATION_TIMEOUT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_SHARD_META_PROPAGATION_TIMEOUT ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsBeginStmt == ID_TRUE )
    {
        (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
    }

    return IDE_FAILURE;    
}

IDE_RC sdi::convertRangeValue( SChar       * aValue,
                               UInt          aLength,
                               UInt          aKeyType,
                               sdiValue    * aRangeValue )
{
    IDE_TEST( sdm::convertRangeValue( aValue,
                                      aLength,
                                      aKeyType,
                                      aRangeValue )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::compareKeyData( UInt       aKeyDataType,
                            sdiValue * aValue1,
                            sdiValue * aValue2,
                            SShort   * aResult )
{
    IDE_TEST( sdm::compareKeyData( aKeyDataType,
                                   aValue1,
                                   aValue2,
                                   aResult )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool sdi::isShardDDL( qcStatement * aStatement )
{
    if (( aStatement->session->mQPSpecific.mFlag & QC_SESSION_SHARD_DDL_MASK ) ==
        QC_SESSION_SHARD_DDL_TRUE )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

idBool sdi::isShardDDLForAddClone( qcStatement * aStatement )
{
    if (( aStatement->session->mQPSpecific.mFlag & QC_SESSION_SAHRD_ADD_CLONE_MASK ) ==
        QC_SESSION_SAHRD_ADD_CLONE_TRUE )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

idBool sdi::hasShardCoordPlan( qcStatement * aStatement )
{
    qmnPlan     * sPlan = NULL;
    idBool        sRet = ID_FALSE;

    if ( aStatement != NULL )
    {
        if ( ( isShardCoordinator( aStatement ) == ID_TRUE ) ||
             ( isPartialCoordinator( aStatement ) == ID_TRUE ) )
        {
            sPlan = aStatement->myPlan->plan;

            if ( sPlan != NULL )
            {
                if ( ( sPlan->type == QMN_SDEX ) ||
                     ( sPlan->type == QMN_SDIN ) )
                {
                    sRet = ID_TRUE;
                }
                else
                {
                    /* Nothing to do. */
                }
            }
            else
            {
                /* Nothing to do. */
            }

            if ( sRet == ID_FALSE )
            {
                isShardSelectExists( sPlan,
                                     &sRet );
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            /* Nothing to do. */
        }
    }

    return sRet;
}

idBool sdi::isShardDML( qcStatement * aStatement )
{
    qmnPlan * sPlan = NULL;
    idBool    sRet  = ID_FALSE;

    IDE_DASSERT( aStatement != NULL);
    IDE_DASSERT( aStatement->myPlan != NULL);

    sPlan = aStatement->myPlan->plan;

    if ( sPlan != NULL )
    {
        if ( ( sPlan->type == QMN_SDEX ) ||
             ( sPlan->type == QMN_SDIN ) )
        {
            sRet = ID_TRUE;
        }
        else
        {
            if ( aStatement->myPlan->mShardAnalysis != NULL )
            {
                if ( aStatement->myPlan->mShardAnalysis->mTopQueryFlag[ SDI_TQ_FOR_UPDATE_EXISTS ] == ID_TRUE )
                {
                    sRet = ID_TRUE;
                }
                else
                {
                    /* Nothing to do. */
                }
            }
            else
            {
                /* Nothing to do. */
            }
        }
    }
    else
    {
        /* Nothing to do. */
    }

    return sRet;
}

void sdi::isShardSelectExists( qmnPlan * aPlan,
                               idBool  * aIsSDSEExists  )
{
    qmnChildren * sCurrChild = NULL;

    if ( ( *aIsSDSEExists == ID_FALSE ) &&
         ( aPlan != NULL ) )
    {
        if ( aPlan->type == QMN_SDSE )
        {
            *aIsSDSEExists = ID_TRUE;
        }
        else
        {
            isShardSelectExists( aPlan->left,
                                 aIsSDSEExists );

            isShardSelectExists( aPlan->right,
                                 aIsSDSEExists );

            for ( sCurrChild = aPlan->children;
                  sCurrChild != NULL;
                  sCurrChild = sCurrChild->next )
            {
                isShardSelectExists( sCurrChild->childPlan,
                                     aIsSDSEExists );
            }
        }
    }
    else
    {
        /* Nothing to do. */
    }
}

void sdi::getShardObjInfoForSMN( ULong            aSMN,
                                 sdiObjectInfo  * aShardObjectList,
                                 sdiObjectInfo ** aRet )
{
    sdiObjectInfo * sShardObject = NULL;

    IDE_DASSERT( aShardObjectList != NULL );

    *aRet = NULL;

    for ( sShardObject  = aShardObjectList;
          sShardObject != NULL;
          sShardObject  = sShardObject->mNext )
    {
        if ( sShardObject->mSMN == aSMN )
        {
            *aRet = sShardObject;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }
}

IDE_RC sdi::filterNodeInfo( sdiNodeInfo * aSourceNodeInfo,
                            sdiNodeInfo * aAliveNodeInfo,
                            iduList     * aNodeList )
{
    UShort sSourceNodeIdx = 0;

    iduListNode  * sIterator = NULL; 

    /* nodeinfo init */
    aAliveNodeInfo->mCount = 0;
    
    for ( sSourceNodeIdx = 0;
          sSourceNodeIdx < aSourceNodeInfo->mCount;
          sSourceNodeIdx++ )
    {
        IDU_LIST_ITERATE( aNodeList, sIterator )
        {
            if ( idlOS::strncmp ( (SChar*)sIterator->mObj, 
                                  aSourceNodeInfo->mNodes[sSourceNodeIdx].mNodeName, 
                                  SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                IDE_TEST_RAISE( aAliveNodeInfo->mCount >= SDI_NODE_MAX_COUNT, ERR_NODE_INFO_OVERFLOW );

                idlOS::memcpy( (void*) &aAliveNodeInfo->mNodes[aAliveNodeInfo->mCount],
                               (void*) &aSourceNodeInfo->mNodes[sSourceNodeIdx],
                               ID_SIZEOF(sdiNode) );

                aAliveNodeInfo->mCount++;

                /* Alive Node Found */
                break;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NODE_INFO_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::filterNodeInfo",
                                  "Too many node count for shard meta changes" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::unionNodeInfo( sdiNodeInfo * aSourceNodeInfo,
                           sdiNodeInfo * aTargetNodeInfo )
{
    UShort sSourceNodeIdx = 0;
    UShort sTargetNodeIdx = 0;
    idBool sIsFound = ID_FALSE;
    
    for ( sSourceNodeIdx = 0;
          sSourceNodeIdx < aSourceNodeInfo->mCount;
          sSourceNodeIdx++, sIsFound = ID_FALSE )
    {
        for ( sTargetNodeIdx = 0;
              sTargetNodeIdx < aTargetNodeInfo->mCount;
              sTargetNodeIdx++ )
        {
            if ( aSourceNodeInfo->mNodes[sSourceNodeIdx].mNodeId == aTargetNodeInfo->mNodes[sTargetNodeIdx].mNodeId )
            {
                sIsFound = ID_TRUE;
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }

        if ( sIsFound == ID_FALSE )
        {
            IDE_TEST_RAISE( aTargetNodeInfo->mCount >= SDI_NODE_MAX_COUNT, ERR_NODE_INFO_OVERFLOW );

            idlOS::memcpy( (void*) &aTargetNodeInfo->mNodes[aTargetNodeInfo->mCount],
                           (void*) &aSourceNodeInfo->mNodes[sSourceNodeIdx],
                           ID_SIZEOF(sdiNode) );

            aTargetNodeInfo->mCount++;
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NODE_INFO_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::unionNodeInfo",
                                  "Too many node count for shard meta changes" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::getAddedNodeInfo( sdiClientInfo * aClientInfo,
                              sdiNodeInfo   * aNewNodeInfo,
                              sdiNodeInfo   * aTarget )
{
    UShort sIdx4New;
    UShort sIdx4Cur;
    idBool sIsFound;

    sdiConnectInfo  * sConnectInfo = NULL;

    for ( sIdx4New = 0, sIsFound = ID_FALSE;
          sIdx4New < aNewNodeInfo->mCount;
          ++sIdx4New, sIsFound = ID_FALSE )
    {
        for ( sIdx4Cur = 0, sConnectInfo = aClientInfo->mConnectInfo;
              sIdx4Cur < aClientInfo->mCount;
              ++sIdx4Cur, ++sConnectInfo )
        {
            if (    aNewNodeInfo->mNodes[sIdx4New].mNodeId
                 == sConnectInfo->mNodeInfo.mNodeId )
            {
                sIsFound = ID_TRUE;
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }

        if ( sIsFound == ID_TRUE )
        {
            /* Nothing to do */
        }
        else
        {
            IDE_TEST_RAISE( aTarget->mCount >= SDI_NODE_MAX_COUNT, ERR_NODE_INFO_OVERFLOW );

            idlOS::memcpy( (void*) &aTarget->mNodes[aTarget->mCount],
                           (void*) &aNewNodeInfo->mNodes[sIdx4New],
                           ID_SIZEOF(sdiNode) );

            aTarget->mCount++;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NODE_INFO_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::getAddedNodeInfo",
                                  "Too many node count for shard meta changes" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-47459 */
const SChar * sdi::getShardSavepointName( qciStmtType aStmtKind )
{
    switch( aStmtKind )
    {
        case QCI_STMT_INSERT:
        case QCI_STMT_UPDATE:
        case QCI_STMT_DELETE:
        case QCI_STMT_SELECT_FOR_UPDATE:
            return (SChar*)SAVEPOINT_FOR_SHARD_STMT_PARTIAL_ROLLBACK;
            break;

        case QCI_STMT_EXEC_PROC:
            return (SChar*)SAVEPOINT_FOR_SHARD_CLONE_PROC_PARTIAL_ROLLBACK;
            break;

        default:
            IDE_DASSERT( 0 );
            return NULL;
            break;            
    }
}

/* 나중에 SHARD JOIN이나 ADD 구문에서 호출해야한다. */
IDE_RC sdi::updateLocalMetaInfoForReplication( qcStatement  * aStatement,
                                               UInt           aKSafety,
                                               SChar        * aReplicationMode,
                                               UInt           aParallelCount,
                                               UInt         * aRowCnt )
{
    UInt sRowCnt = 0;
    /* update local meta info */
    IDE_TEST( sdm::updateLocalMetaInfoForReplication( aStatement,
                               (UInt) aKSafety,
                               (SChar*) aReplicationMode,
                               (UInt) aParallelCount,
                               &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_SHARD_META );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_META );
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDF_SHARD_LOCAL_META_ERROR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-2727
IDE_RC sdi::setShardSessionProperty( qcSession      * aSession,
                                     sdiClientInfo  * aClientInfo,
                                     sdiConnectInfo * aConnectInfo )
{
    UInt   sPropertyAttribute = 0;
    SChar  sNlsTerritory[IDP_MAX_VALUE_LEN + 1] = {0,};
    smSCN  sSCN;

    if ((( aConnectInfo->mFlag & SDI_CONNECT_ATTR_CHANGE_MASK )
         == SDI_CONNECT_ATTR_CHANGE_TRUE ))
    {
        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_DATE_FORMAT_MASK )
             == QC_SESSION_ATTR_DATE_FORMAT_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "DEFAULT_DATE_FORMAT",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_DATE_FORMAT,
                                            0,
                                            qci::mSessionCallback.mGetDateFormat( aSession->mMmSession ),
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }
        
        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE_MASK )
             == QC_SESSION_ATTR___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "__OPTIMIZER_DEFAULT_TEMP_TBS_TYPE",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );


            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE,
                                            qci::mSessionCallback.mGetOptimizerDefaultTempTbsType( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR___OPTIMIZER_TRANSITIVITY_OLD_RULE_MASK )
             == QC_SESSION_ATTR___OPTIMIZER_TRANSITIVITY_OLD_RULE_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "__OPTIMIZER_TRANSITIVITY_OLD_RULE",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );


            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE___OPTIMIZER_TRANSITIVITY_OLD_RULE,
                                            qci::mSessionCallback.mGetOptimizerTransitivityOldRule( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }
       
        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR___PRINT_OUT_ENABLE_MASK )
             == QC_SESSION_ATTR___PRINT_OUT_ENABLE_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "__PRINT_OUT_ENABLE",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE___PRINT_OUT_ENABLE,
                                            qci::mSessionCallback.mGetPrintOutEnable( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR___USE_OLD_SORT_MASK )
             == QC_SESSION_ATTR___USE_OLD_SORT_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "__USE_OLD_SORT",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE___USE_OLD_SORT,
                                            qci::mSessionCallback.mGetUseOldSort( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_ARITHMETIC_OPERATION_MODE_MASK )
             == QC_SESSION_ATTR_ARITHMETIC_OPERATION_MODE_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "ARITHMETIC_OPERATION_MODE",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_ARITHMETIC_OPERATION_MODE,
                                            qci::mSessionCallback.mGetArithmeticOpMode( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_AUTO_REMOTE_EXEC_MASK )
             == QC_SESSION_ATTR_AUTO_REMOTE_EXEC_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "AUTO_REMOTE_EXEC",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_AUTO_REMOTE_EXEC,
                                            qci::mSessionCallback.mGetAutoRemoteExec( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_COMMIT_WRITE_WAIT_MODE_MASK )
             == QC_SESSION_ATTR_COMMIT_WRITE_WAIT_MODE_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "COMMIT_WRITE_WAIT_MODE",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                // add session callback
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_COMMIT_WRITE_WAIT_MODE,
                                            qci::mSessionCallback.mGetCommitWriteWaitMode( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT_MASK )
             == QC_SESSION_ATTR_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "DBLINK_REMOTE_STATEMENT_AUTOCOMMIT",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
            // add session callback
            IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                        SDL_ALTIBASE_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT,
                                        qci::mSessionCallback.mGetDblinkRemoteStatementAutoCommit( aSession->mMmSession ),
                                        NULL,
                                        &(aConnectInfo->mLinkFailure) )
                      != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_DDL_TIMEOUT_MASK )
             == QC_SESSION_ATTR_DDL_TIMEOUT_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "DDL_TIMEOUT",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                // add session callback
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_DDL_TIMEOUT,
                                            qci::mSessionCallback.mGetDdlTimeout( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_FETCH_TIMEOUT_MASK )
             == QC_SESSION_ATTR_FETCH_TIMEOUT_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "FETCH_TIMEOUT",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                // add session callback            
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_FETCH_TIMEOUT,
                                            qci::mSessionCallback.mGetFetchTimeout( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_GLOBAL_TRANSACTION_LEVEL_MASK )
             == QC_SESSION_ATTR_GLOBAL_TRANSACTION_LEVEL_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "GLOBAL_TRANSACTION_LEVEL",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_GLOBAL_TRANSACTION_LEVEL,
                                            qci::mSessionCallback.mGetGTXLevel( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );

                /* PROJ-2733-DistTxInfo GCTx인 경우 노드 SCN을 얻어와 COORD SCN으로 갱신한다. */
                if ( qci::mSessionCallback.mIsGCTx( aSession->mMmSession ) == ID_TRUE )
                {
                    IDE_TEST( sdl::getSCN( aConnectInfo, &sSCN ) != IDE_SUCCESS );

                    #if defined(DEBUG)
                    ideLog::log( IDE_SD_18, "[GLOBAL_CONSISTENT_TRANSACTION_DISTTXINFO] = [%s] setShardSessionProperty %s SCN : %"ID_UINT64_FMT,
                                 aClientInfo->mGCTxInfo.mSessionTypeString,
                                 aConnectInfo->mNodeName,
                                 sSCN );
                    #endif

                    SM_SET_MAX_SCN( &(aClientInfo->mGCTxInfo.mCoordSCN), &sSCN );
                }
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_IDLE_TIMEOUT_MASK )
             == QC_SESSION_ATTR_IDLE_TIMEOUT_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "IDLE_TIMEOUT",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                // add session callback 
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_IDLE_TIMEOUT,
                                            qci::mSessionCallback.mGetIdleTimeout( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_LOB_CACHE_THRESHOLD_MASK )
             == QC_SESSION_ATTR_LOB_CACHE_THRESHOLD_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "LOB_CACHE_THRESHOLD",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_LOB_CACHE_THRESHOLD,
                                            qci::mSessionCallback.mGetLobCacheThreshold( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }
        
        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_NLS_TERRITORY_MASk )
             == QC_SESSION_ATTR_NLS_TERRITORY_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "NLS_TERRITORY",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                // add session callback
                qci::mSessionCallback.mGetNlsTerritory( aSession->mMmSession, sNlsTerritory );
            
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_NLS_TERRITORY,
                                            0,
                                            sNlsTerritory,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }
        
        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_MAX_STATEMENTS_PER_SESSION_MASK )
             == QC_SESSION_ATTR_MAX_STATEMENTS_PER_SESSION_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "MAX_STATEMENTS_PER_SESSION",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                // add session callback             
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_MAX_STATEMENTS_PER_SESSION,
                                            qci::mSessionCallback.mGetMaxStatementsPerSession( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS);
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_NLS_CURRENCY_MASK )
             == QC_SESSION_ATTR_NLS_CURRENCY_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "NLS_CURRENCY",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_NLS_CURRENCY,
                                            0,
                                            qci::mSessionCallback.mGetNlsCurrency( aSession->mMmSession ),
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_NLS_ISO_CURRENCY_MASK )
             == QC_SESSION_ATTR_NLS_ISO_CURRENCY_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "NLS_ISO_CURRENCY",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( mtlTerritory::searchNlsISOTerritoryName(
                              qci::mSessionCallback.mGetNlsISOCurrency( aSession->mMmSession ),
                              sNlsTerritory )
                          != IDE_SUCCESS );
                       
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_NLS_ISO_CURRENCY,
                                            0,
                                            sNlsTerritory,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_NLS_NCHAR_CONV_EXCP_MASK )
             == QC_SESSION_ATTR_NLS_NCHAR_CONV_EXCP_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "NLS_NCHAR_CONV_EXCP",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                // add session callback            
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_NLS_NCHAR_CONV_EXCP,
                                            qci::mSessionCallback.mGetNlsNcharConvExcp( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_NLS_NUMERIC_CHARACTERS_MASK )
             == QC_SESSION_ATTR_NLS_NUMERIC_CHARACTERS_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "NLS_NUMERIC_CHARACTERS",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_NLS_NUMERIC_CHARACTERS,
                                            0,
                                            qci::mSessionCallback.mGetNlsNumChar( aSession->mMmSession ),
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_NORMALFORM_MAXIMUM_MASK )
             == QC_SESSION_ATTR_NORMALFORM_MAXIMUM_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "NORMALFORM_MAXIMUM",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_NORMALFORM_MAXIMUM,
                                            qci::mSessionCallback.mGetNormalFormMaximum( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_OPTIMIZER_AUTO_STATS_MASK )
             == QC_SESSION_ATTR_OPTIMIZER_AUTO_STATS_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "OPTIMIZER_AUTO_STATS",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_OPTIMIZER_AUTO_STATS,
                                            qci::mSessionCallback.mGetOptimizerAutoStats( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_OPTIMIZER_DISK_INDEX_COST_ADJ_MASK )
             == QC_SESSION_ATTR_OPTIMIZER_DISK_INDEX_COST_ADJ_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "OPTIMIZER_DISK_INDEX_COST_ADJ",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_OPTIMIZER_DISK_INDEX_COST_ADJ,
                                            qci::mSessionCallback.mGetOptimizerDiskIndexCostAdj( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_OPTIMIZER_MEMORY_INDEX_COST_ADJ_MASK )
             == QC_SESSION_ATTR_OPTIMIZER_MEMORY_INDEX_COST_ADJ_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "OPTIMIZER_MEMORY_INDEX_COST_ADJ",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_OPTIMIZER_MEMORY_INDEX_COST_ADJ,
                                            qci::mSessionCallback.mGetOptimizerMemoryIndexCostAdj( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_OPTIMIZER_MODE_MASK )
             == QC_SESSION_ATTR_OPTIMIZER_MODE_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "OPTIMIZER_MODE",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                if ( qci::mSessionCallback.mGetOptimizerMode( aSession->mMmSession ) == 0)
                {
                    IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                                SDL_ALTIBASE_OPTIMIZER_MODE,
                                                2,
                                                NULL,
                                                &(aConnectInfo->mLinkFailure) )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                                SDL_ALTIBASE_OPTIMIZER_MODE,
                                                qci::mSessionCallback.mGetOptimizerMode( aSession->mMmSession ),
                                                NULL,
                                                &(aConnectInfo->mLinkFailure) )
                              != IDE_SUCCESS );
                }
            }
        }
        

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_OPTIMIZER_PERFORMANCE_VIEW_MASK )
             == QC_SESSION_ATTR_OPTIMIZER_PERFORMANCE_VIEW_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "OPTIMIZER_PERFORMANCE_VIEW",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_OPTIMIZER_PERFORMANCE_VIEW,
                                            qci::mSessionCallback.mGetOptimizerPerformanceView( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_PARALLEL_DML_MODE_MASK )
             == QC_SESSION_ATTR_PARALLEL_DML_MODE_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "PARALLEL_DML_MODE",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_PARALLEL_DML_MODE,
                                            qci::mSessionCallback.mIsParallelDml( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_QUERY_REWRITE_ENABLE_MASK )
             == QC_SESSION_ATTR_QUERY_REWRITE_ENABLE_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "QUERY_REWRITE_ENABLE",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_QUERY_REWRITE_ENABLE,
                                            qci::mSessionCallback.mGetQueryRewriteEnable( aSession->mMmSession ),
                                            NULL,

                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_QUERY_TIMEOUT_MASK )
             == QC_SESSION_ATTR_QUERY_TIMEOUT_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "QUERY_TIMEOUT",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                // add session callback 
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_SQL_ATTR_QUERY_TIMEOUT,
                                            qci::mSessionCallback.mGetQueryTimeout( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_RECYCLEBIN_ENABLE_MASK )
             == QC_SESSION_ATTR_RECYCLEBIN_ENABLE_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "RECYCLEBIN_ENABLE",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_RECYCLEBIN_ENABLE,
                                            qci::mSessionCallback.mGetRecyclebinEnable( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_REPLICATION_DDL_SYNC_MASK )
             == QC_SESSION_ATTR_REPLICATION_DDL_SYNC_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "REPLICATION_DDL_SYNC",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_REPLICATION_DDL_SYNC,
                                            qci::mSessionCallback.mGetReplicationDDLSync( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_REPLICATION_DDL_SYNC_TIMEOUT_MASK )
             == QC_SESSION_ATTR_REPLICATION_DDL_SYNC_TIMEOUT_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "REPLICATION_DDL_SYNC_TIMEOUT",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                // add session callback 
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_REPLICATION_DDL_SYNC_TIMEOUT,
                                            qci::mSessionCallback.mGetReplicationDDLSyncTimeout( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_RESULT_CACHE_ENABLE_MASK )
             ==  QC_SESSION_ATTR_RESULT_CACHE_ENABLE_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "RESULT_CACHE_ENABLE",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
            
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_RESULT_CACHE_ENABLE,
                                            qci::mSessionCallback.mGetResultCacheEnable( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_SELECT_HEADER_DISPLAY_MASK )
             == QC_SESSION_ATTR_SELECT_HEADER_DISPLAY_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "SELECT_HEADER_DISPLAY",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_HEADER_DISPLAY_MODE,
                                            qci::mSessionCallback.mGetSelectHeaderDisplay( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_SERIAL_EXECUTE_MODE_MASK )
             == QC_SESSION_ATTR_SERIAL_EXECUTE_MODE_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "SERIAL_EXECUTE_MODE",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
            
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_SERIAL_EXECUTE_MODE,
                                            qci::mSessionCallback.mGetSerialExecuteMode( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_ST_OBJECT_BUFFER_SIZE_MASK )
             == QC_SESSION_ATTR_ST_OBJECT_BUFFER_SIZE_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "ST_OBJECT_BUFFER_SIZE",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_ST_OBJECT_BUFFER_SIZE,
                                            qci::mSessionCallback.mGetSTObjBufSize( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_TIME_ZONE_MASK )
             == QC_SESSION_ATTR_TIME_ZONE_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "TIME_ZONE",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_TIME_ZONE,
                                            0,
                                            qci::mSessionCallback.mGetSessionTimezoneString( aSession->mMmSession ),
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_TOP_RESULT_CACHE_MODE_MASK )
             == QC_SESSION_ATTR_TOP_RESULT_CACHE_MODE_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "TOP_RESULT_CACHE_MODE",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_TOP_RESULT_CACHE_MODE,
                                            qci::mSessionCallback.mGetTopResultCacheMode( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_TRCLOG_DETAIL_INFORMATION_MASK )
             == QC_SESSION_ATTR_TRCLOG_DETAIL_INFORMATION_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "TRCLOG_DETAIL_INFORMATION",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_TRCLOG_DETAIL_INFORMATION,
                                            qci::mSessionCallback.mGetTrclogDetailInformation( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_TRCLOG_DETAIL_PREDICATE_MASK )
             == QC_SESSION_ATTR_TRCLOG_DETAIL_PREDICATE_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "TRCLOG_DETAIL_PREDICATE",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {   
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_TRCLOG_DETAIL_PREDICATE,
                                            qci::mSessionCallback.mGetTrclogDetailPredicate( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_TRCLOG_DETAIL_SHARD_MASK )
             == QC_SESSION_ATTR_TRCLOG_DETAIL_SHARD_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "TRCLOG_DETAIL_SHARD",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_TRCLOG_DETAIL_SHARD,
                                            qci::mSessionCallback.mGetTrclogDetailShard( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_TRX_UPDATE_MAX_LOGSIZE_MASK )
             == QC_SESSION_ATTR_TRX_UPDATE_MAX_LOGSIZE_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "TRX_UPDATE_MAX_LOGSIZE",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                // add session callback
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_TRX_UPDATE_MAX_LOGSIZE,
                                            qci::mSessionCallback.mGetUpdateMaxLogSize( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_UTRANS_TIMEOUT_MASK )
             == QC_SESSION_ATTR_UTRANS_TIMEOUT_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "UTRANS_TIMEOUT",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                // add session callback
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_UTRANS_TIMEOUT,
                                            qci::mSessionCallback.mGetUTransTimeout( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR___REDUCE_PARTITION_PREPARE_MEMORY_MASK )
             == QC_SESSION_ATTR___REDUCE_PARTITION_PREPARE_MEMORY_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "__REDUCE_PARTITION_PREPARE_MEMORY",
                                     &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE___REDUCE_PARTITION_PREPARE_MEMORY,
                                            qci::mSessionCallback.mGetReducePartPrepareMemory( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_INVOKE_USER_MASK )
             == QC_SESSION_ATTR_INVOKE_USER_TRUE )
        {
            IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                        SDL_ALTIBASE_INVOKE_USER,
                                        0,
                                        qci::mSessionCallback.mGetInvokeUserName( aSession->mMmSession ),
                                        &(aConnectInfo->mLinkFailure) )
                      != IDE_SUCCESS );
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_TRANSACTIONAL_DDL_MASK )
             == QC_SESSION_ATTR_TRANSACTIONAL_DDL_TRUE )
        {
            IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                        SDL_ALTIBASE_TRANSACTIONAL_DDL,
                                        qci::mSessionCallback.mTransactionalDDL( aSession->mMmSession ),
                                        NULL,
                                        &(aConnectInfo->mLinkFailure) )
                      != IDE_SUCCESS );
        }

        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_SHARD_STATEMENT_RETRY_MASK )
             == QC_SESSION_ATTR_SHARD_STATEMENT_RETRY_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "SHARD_STATEMENT_RETRY", &sPropertyAttribute ) != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_SHARD_STATEMENT_RETRY,
                                            qci::mSessionCallback.mGetShardStatementRetry( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }
        
        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_INDOUBT_FETCH_TIMEOUT_MASK )
             == QC_SESSION_ATTR_INDOUBT_FETCH_TIMEOUT_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "INDOUBT_FETCH_TIMEOUT", &sPropertyAttribute ) != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_INDOUBT_FETCH_TIMEOUT,
                                            qci::mSessionCallback.mGetIndoubtFetchTimeout( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }
        
        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_INDOUBT_FETCH_METHOD_MASK )
             == QC_SESSION_ATTR_INDOUBT_FETCH_METHOD_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "INDOUBT_FETCH_METHOD", &sPropertyAttribute ) != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_INDOUBT_FETCH_METHOD,
                                            qci::mSessionCallback.mGetIndoubtFetchMethod( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }
        /* BUG-48132 */ 
        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD_MASK )
             == QC_SESSION_ATTR___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "__OPTIMIZER_PLAN_HASH_OR_SORT_METHOD", &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD,
                                            qci::mSessionCallback.mGetPlanHashOrSortMethod( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }
        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR_DDL_LOCK_TIMEOUT_MASK )
             == QC_SESSION_ATTR_DDL_LOCK_TIMEOUT_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "DDL_LOCK_TIMEOUT", &sPropertyAttribute ) != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE_DDL_LOCK_TIMEOUT,
                                            qci::mSessionCallback.mGetDDLLockTimeout( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }
        /* BUG-48161 */ 
        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR___OPTIMIZER_BUCKET_COUNT_MAX_MASK )
             == QC_SESSION_ATTR___OPTIMIZER_BUCKET_COUNT_MAX_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "__OPTIMIZER_BUCKET_COUNT_MAX", &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE___OPTIMIZER_BUCKET_COUNT_MAX,
                                            qci::mSessionCallback.mGetBucketCountMax( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }
        /* BUG-48348 */
        if ( ( aConnectInfo->mCoordPropertyFlag & QC_SESSION_ATTR___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION_MASK )
             == QC_SESSION_ATTR___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION_TRUE )
        {
            IDE_TEST( idp::getPropertyAttribute( "__OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION", &sPropertyAttribute )
                      != IDE_SUCCESS );

            if ( sPropertyAttribute == IDP_ATTR_SHARD_ALL )
            {
                IDE_TEST( sdl::setConnAttr( aConnectInfo,
                                            SDL_ALTIBASE___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION,
                                            qci::mSessionCallback.mGetEliminateCommonSubexpression( aSession->mMmSession ),
                                            NULL,
                                            &(aConnectInfo->mLinkFailure) )
                          != IDE_SUCCESS );
            }
        }

        // BUG-47765
        aConnectInfo->mFlag &= ~SDI_CONNECT_ATTR_CHANGE_MASK;
        aConnectInfo->mFlag |= SDI_CONNECT_ATTR_CHANGE_FALSE;
    
        aConnectInfo->mCoordPropertyFlag = 0;
    }
    else
    {
        // nothing to do
    }
        
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    // BUG-47765
    aConnectInfo->mFlag &= ~SDI_CONNECT_ATTR_CHANGE_MASK;
    aConnectInfo->mFlag |= SDI_CONNECT_ATTR_CHANGE_FALSE;

    aConnectInfo->mCoordPropertyFlag = 0;
    
    return IDE_FAILURE;
}

// PROJ-2727
void sdi::unSetSessionPropertyFlag( qcSession * aSession )
{
    if (( aSession->mQPSpecific.mFlag & QC_SESSION_ATTR_CHANGE_MASK )
        == QC_SESSION_ATTR_CHANGE_TRUE )
    {
        aSession->mQPSpecific.mFlag &= ~QC_SESSION_ATTR_CHANGE_MASK;
        aSession->mQPSpecific.mFlag |= QC_SESSION_ATTR_CHANGE_FALSE;

        aSession->mQPSpecific.mFlag &= ~QC_SESSION_ATTR_SET_NODE_MASK;
        aSession->mQPSpecific.mFlag |= QC_SESSION_ATTR_SET_NODE_FALSE;
        
        aSession->mQPSpecific.mPropertyFlag = 0;
    }
    else
    {
        // nothing to do
    }
}

IDE_RC sdi::setSessionPropertyFlag( qcSession * aSession,
                                    UShort      aSessionPropID )
{
    IDE_DASSERT( aSession != NULL );

    if (( sdi::isShardEnable() == ID_TRUE ) &&
        (( aSession->mQPSpecific.mFlag & QC_SESSION_ATTR_SHARD_META_MASK )
         != QC_SESSION_ATTR_SHARD_META_TRUE )) 
    {
        aSession->mQPSpecific.mFlag &= ~QC_SESSION_ATTR_SET_NODE_MASK;
        aSession->mQPSpecific.mFlag |= QC_SESSION_ATTR_SET_NODE_TRUE;

        aSession->mQPSpecific.mFlag &= ~QC_SESSION_ATTR_CHANGE_MASK;
        aSession->mQPSpecific.mFlag |= QC_SESSION_ATTR_CHANGE_TRUE;

        switch ( aSessionPropID )
        {
            case CMP_DB_PROPERTY_DATE_FORMAT:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_DATE_FORMAT_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_DATE_FORMAT_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_QUERY_TIMEOUT:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_QUERY_TIMEOUT_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_QUERY_TIMEOUT_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_DDL_TIMEOUT:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_DDL_TIMEOUT_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_DDL_TIMEOUT_TRUE;
                break;

            case CMP_DB_PROPERTY_FETCH_TIMEOUT:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_FETCH_TIMEOUT_MASK;
                aSession->mQPSpecific.mPropertyFlag |=  QC_SESSION_ATTR_FETCH_TIMEOUT_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_UTRANS_TIMEOUT:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_UTRANS_TIMEOUT_MASK;
                aSession->mQPSpecific.mPropertyFlag |=  QC_SESSION_ATTR_UTRANS_TIMEOUT_TRUE;
                break;

            case CMP_DB_PROPERTY_IDLE_TIMEOUT:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_IDLE_TIMEOUT_MASK;
                aSession->mQPSpecific.mPropertyFlag |=  QC_SESSION_ATTR_IDLE_TIMEOUT_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_OPTIMIZER_MODE:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_OPTIMIZER_MODE_MASK;
                aSession->mQPSpecific.mPropertyFlag |=  QC_SESSION_ATTR_OPTIMIZER_MODE_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_HEADER_DISPLAY_MODE:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_SELECT_HEADER_DISPLAY_MASK;
                aSession->mQPSpecific.mPropertyFlag |=  QC_SESSION_ATTR_SELECT_HEADER_DISPLAY_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_NORMALFORM_MAXIMUM:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_NORMALFORM_MAXIMUM_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_NORMALFORM_MAXIMUM_TRUE;
                break;
                    
            case CMP_DB_PROPERTY___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE_TRUE;
                break;

            case CMP_DB_PROPERTY_COMMIT_WRITE_WAIT_MODE:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_COMMIT_WRITE_WAIT_MODE_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_COMMIT_WRITE_WAIT_MODE_TRUE;
                break;

            case CMP_DB_PROPERTY_ST_OBJECT_BUFFER_SIZE:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_ST_OBJECT_BUFFER_SIZE_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_ST_OBJECT_BUFFER_SIZE_TRUE;
                break;
                
            case CMP_DB_PROPERTY_TRX_UPDATE_MAX_LOGSIZE:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_TRX_UPDATE_MAX_LOGSIZE_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_TRX_UPDATE_MAX_LOGSIZE_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_PARALLEL_DML_MODE:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_PARALLEL_DML_MODE_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_PARALLEL_DML_MODE_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_NLS_NCHAR_CONV_EXCP:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_NLS_NCHAR_CONV_EXCP_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_NLS_NCHAR_CONV_EXCP_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_AUTO_REMOTE_EXEC:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_AUTO_REMOTE_EXEC_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_AUTO_REMOTE_EXEC_TRUE;
                break;
     
            case CMP_DB_PROPERTY_MAX_STATEMENTS_PER_SESSION:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_MAX_STATEMENTS_PER_SESSION_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_MAX_STATEMENTS_PER_SESSION_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_TRCLOG_DETAIL_PREDICATE:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_TRCLOG_DETAIL_PREDICATE_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_TRCLOG_DETAIL_PREDICATE_TRUE;
                break;
                                        
            case CMP_DB_PROPERTY_OPTIMIZER_DISK_INDEX_COST_ADJ:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_OPTIMIZER_DISK_INDEX_COST_ADJ_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_OPTIMIZER_DISK_INDEX_COST_ADJ_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_OPTIMIZER_MEMORY_INDEX_COST_ADJ:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_OPTIMIZER_MEMORY_INDEX_COST_ADJ_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_OPTIMIZER_MEMORY_INDEX_COST_ADJ_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_NLS_TERRITORY:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_NLS_TERRITORY_MASk;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_NLS_TERRITORY_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_NLS_ISO_CURRENCY:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_NLS_ISO_CURRENCY_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_NLS_ISO_CURRENCY_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_NLS_CURRENCY:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_NLS_CURRENCY_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_NLS_CURRENCY_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_NLS_NUMERIC_CHARACTERS:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_NLS_NUMERIC_CHARACTERS_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_NLS_NUMERIC_CHARACTERS_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_TIME_ZONE:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_TIME_ZONE_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_TIME_ZONE_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_LOB_CACHE_THRESHOLD:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_LOB_CACHE_THRESHOLD_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_LOB_CACHE_THRESHOLD_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_QUERY_REWRITE_ENABLE:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_QUERY_REWRITE_ENABLE_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_QUERY_REWRITE_ENABLE_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_GLOBAL_TRANSACTION_LEVEL:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_GLOBAL_TRANSACTION_LEVEL_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_GLOBAL_TRANSACTION_LEVEL_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_RECYCLEBIN_ENABLE:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_RECYCLEBIN_ENABLE_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_RECYCLEBIN_ENABLE_TRUE;
                break;
                    
            case CMP_DB_PROPERTY___USE_OLD_SORT:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR___USE_OLD_SORT_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR___USE_OLD_SORT_TRUE;
                break;

            case CMP_DB_PROPERTY_ARITHMETIC_OPERATION_MODE:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_ARITHMETIC_OPERATION_MODE_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_ARITHMETIC_OPERATION_MODE_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_RESULT_CACHE_ENABLE:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_RESULT_CACHE_ENABLE_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_RESULT_CACHE_ENABLE_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_TOP_RESULT_CACHE_MODE:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_TOP_RESULT_CACHE_MODE_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_TOP_RESULT_CACHE_MODE_TRUE;
                break;

            case CMP_DB_PROPERTY_OPTIMIZER_AUTO_STATS:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_OPTIMIZER_AUTO_STATS_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_OPTIMIZER_AUTO_STATS_TRUE;
                break;
                    
            case CMP_DB_PROPERTY___OPTIMIZER_TRANSITIVITY_OLD_RULE:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR___OPTIMIZER_TRANSITIVITY_OLD_RULE_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR___OPTIMIZER_TRANSITIVITY_OLD_RULE_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_OPTIMIZER_PERFORMANCE_VIEW:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_OPTIMIZER_PERFORMANCE_VIEW_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_OPTIMIZER_PERFORMANCE_VIEW_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_REPLICATION_DDL_SYNC:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_REPLICATION_DDL_SYNC_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_REPLICATION_DDL_SYNC_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_REPLICATION_DDL_SYNC_TIMEOUT:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_REPLICATION_DDL_SYNC_TIMEOUT_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_REPLICATION_DDL_SYNC_TIMEOUT_TRUE;
                break;
                    
            case CMP_DB_PROPERTY___PRINT_OUT_ENABLE:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR___PRINT_OUT_ENABLE_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR___PRINT_OUT_ENABLE_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_TRCLOG_DETAIL_SHARD:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_TRCLOG_DETAIL_SHARD_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_TRCLOG_DETAIL_SHARD_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_SERIAL_EXECUTE_MODE:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_SERIAL_EXECUTE_MODE_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_SERIAL_EXECUTE_MODE_TRUE;
                break;
                    
            case CMP_DB_PROPERTY_TRCLOG_DETAIL_INFORMATION:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_TRCLOG_DETAIL_INFORMATION_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_TRCLOG_DETAIL_INFORMATION_TRUE;
                break;
                    
            case CMP_DB_PROPERTY___REDUCE_PARTITION_PREPARE_MEMORY:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR___REDUCE_PARTITION_PREPARE_MEMORY_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR___REDUCE_PARTITION_PREPARE_MEMORY_TRUE;
                break;
            
            case CMP_DB_PROPERTY_INVOKE_USER:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_INVOKE_USER_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_INVOKE_USER_TRUE;
                break;

            case CMP_DB_PROPERTY_TRANSACTIONAL_DDL:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_TRANSACTIONAL_DDL_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_TRANSACTIONAL_DDL_TRUE;
                break;

            case CMP_DB_PROPERTY_SHARD_STATEMENT_RETRY:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_SHARD_STATEMENT_RETRY_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_SHARD_STATEMENT_RETRY_TRUE;
                break;

            case CMP_DB_PROPERTY_INDOUBT_FETCH_TIMEOUT:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_INDOUBT_FETCH_TIMEOUT_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_INDOUBT_FETCH_TIMEOUT_TRUE;
                break;

            case CMP_DB_PROPERTY_INDOUBT_FETCH_METHOD:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_INDOUBT_FETCH_METHOD_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_INDOUBT_FETCH_METHOD_TRUE;
                break;
            /* BUG-48132 */
            case CMP_DB_PROPERTY___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD_TRUE;
                break;
            /* BUG-48161 */
            case CMP_DB_PROPERTY___OPTIMIZER_BUCKET_COUNT_MAX:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR___OPTIMIZER_BUCKET_COUNT_MAX_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR___OPTIMIZER_BUCKET_COUNT_MAX_TRUE;
                break;
            case CMP_DB_PROPERTY_DDL_LOCK_TIMEOUT:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR_DDL_LOCK_TIMEOUT_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR_DDL_LOCK_TIMEOUT_TRUE;
                break;
            /* BUG-48348 */
            case CMP_DB_PROPERTY___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION:
                aSession->mQPSpecific.mPropertyFlag &= ~QC_SESSION_ATTR___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION_MASK;
                aSession->mQPSpecific.mPropertyFlag |= QC_SESSION_ATTR___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION_TRUE;
                break;
            /* 전송하지 않을 프로퍼티는 플래그를 세팅해주지 않는다. */
            case CMP_DB_PROPERTY_GLOBAL_DDL:
                break;
            default:
                IDE_RAISE( ERR_NOT_EXIST_PROPERTY );
                break;
        }
    }
    else
    {
        // Nothing to do.
    }
    aSession->mQPSpecific.mFlag &= ~QC_SESSION_ATTR_SHARD_META_MASK;
    aSession->mQPSpecific.mFlag |= QC_SESSION_ATTR_SHARD_META_FALSE;
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_PROPERTY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::setSessionPropertyFlag",
                                  "not exist property" ) );
    }
    IDE_EXCEPTION_END;

    unSetSessionPropertyFlag( aSession );
    
    return IDE_FAILURE;
}

// BUG-47765
IDE_RC sdi::copyPropertyFlagToCoordPropertyFlag( qcSession     * aSession,
                                                 sdiClientInfo * aClientInfo )
{
    sdiConnectInfo  * sConnectInfo = NULL;
    UShort            i            = 0;

    if ( aClientInfo != NULL )
    {
        sConnectInfo = aClientInfo->mConnectInfo;
    
        if (( aSession->mQPSpecific.mFlag & QC_SESSION_ATTR_CHANGE_MASK )
            == QC_SESSION_ATTR_CHANGE_TRUE )
        {
            sConnectInfo = aClientInfo->mConnectInfo;
            
            for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++ )
            {
                if (( aSession->mQPSpecific.mFlag & QC_SESSION_ATTR_SET_NODE_MASK )
                    == QC_SESSION_ATTR_SET_NODE_TRUE )
                {
                    // 1. ALTER SESSION SET 변경
                    // 2. SQLSetConnectAttr() 변경
                    sConnectInfo->mFlag &= ~SDI_CONNECT_ATTR_CHANGE_MASK;
                    sConnectInfo->mFlag |= SDI_CONNECT_ATTR_CHANGE_TRUE;

                    sConnectInfo->mCoordPropertyFlag = aSession->mQPSpecific.mPropertyFlag;
                }
                else
                {
                    // 1. ALTER SESSION SET -> 2. SELECT NODE2 -> 3. SELECT NODE1, NODE2, NODE3
                    // 2번에서 NODE2에만 프로퍼티가 변경 되어(전 노드 전파가 되지 않음)
                    // 3번 수행 경우  nothing to do 
                    // nothing to do
                }
            }
        }
        else
        {
            sConnectInfo->mCoordPropertyFlag = 0;
        }
    }
    else
    {
        // nothing to do
    }
    
    return IDE_SUCCESS;
}

/* PROJ-2728 Sharding LOB */
UInt sdi::getRemoteStmtId( sdiDataNode * aDataNode )
{
    return aDataNode->mRemoteStmt->mRemoteStmtId;
}

IDE_RC sdi::getSplitMethodCharByType(sdiSplitMethod aSplitMethod, SChar* aOutChar)
{
    SChar sSplitMethod = '\0';

    switch (aSplitMethod)
    {
        case SDI_SPLIT_HASH:
            sSplitMethod = 'H';
            break;
        case SDI_SPLIT_RANGE:
            sSplitMethod = 'R';
            break;
        case SDI_SPLIT_LIST:
            sSplitMethod = 'L';
            break;
        case SDI_SPLIT_CLONE:
            sSplitMethod = 'C';
            break;
        case SDI_SPLIT_SOLO:
            sSplitMethod = 'S';
            break;
        default:
            IDE_RAISE(ERR_INVALID_SHARD_SPLIT_METHOD_NAME);
    }

    *aOutChar = sSplitMethod;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_SHARD_SPLIT_METHOD_NAME );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_SHARD_SPLIT_METHOD_NAME ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::getSplitMethodCharByStr(SChar* aSplitMethodStr, SChar* aOutChar)
{
    SChar sSplitMethod = '\0';

    if ( idlOS::strMatch( aSplitMethodStr, idlOS::strlen(aSplitMethodStr),
                          "H", 1 ) == 0 )
    {
        sSplitMethod = 'H';
    }
    else if ( idlOS::strMatch( aSplitMethodStr, idlOS::strlen(aSplitMethodStr),
                               "R", 1 ) == 0 )
    {
        sSplitMethod = 'R';
    }
    else if ( idlOS::strMatch( aSplitMethodStr, idlOS::strlen(aSplitMethodStr),
                               "C", 1 ) == 0 )
    {
        sSplitMethod = 'C';
    }
    else if ( idlOS::strMatch( aSplitMethodStr, idlOS::strlen(aSplitMethodStr),
                               "L", 1 ) == 0 )
    {
        sSplitMethod = 'L';
    }
    else if ( idlOS::strMatch( aSplitMethodStr, idlOS::strlen(aSplitMethodStr),
                               "S", 1 ) == 0 )
    {
        sSplitMethod = 'S';
    }
    else
    {
        IDE_RAISE( ERR_INVALID_SHARD_SPLIT_METHOD_NAME );
    }

    *aOutChar = sSplitMethod;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_SHARD_SPLIT_METHOD_NAME );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_SHARD_SPLIT_METHOD_NAME ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* QDSD에서 gShardDBInfo에 접근하기 위한 getting 함수 */
sdiDatabaseInfo sdi::getShardDBInfo()
{
    return gShardDBInfo;
}

/* QDSD에서 sdm::getLocalMetaInfo에 접근하기 위한 warper 함수 */
IDE_RC sdi::getLocalMetaInfo( sdiLocalMetaInfo * aLocalMetaInfo )
{
    IDE_TEST( sdm::getLocalMetaInfo( aLocalMetaInfo ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::getLocalMetaInfoAndCheckKSafety( sdiLocalMetaInfo * aLocalMetaInfo, idBool * aOutIsKSafetyNULL )
{
    IDE_TEST( sdm::getLocalMetaInfoAndCheckKSafety( aLocalMetaInfo, aOutIsKSafetyNULL ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* 
 * PROJ-2733-DistTxInfo 아래 순서로 수행된다.
 *  1. sdi::calculateGCTxInfo
 *     + sdi::calcDistTxInfo()
 *     + sdi::decideRequestSCN()
 *  3. sdi::propagateDistTxInfoToNodes()
 *  4. sdi::executeInsert() || sdi::executeSelect() || sdi::executeDML()
 *  5. sdi::updateMaxNodeSCNToCoordSCN()
 */

/**
 *  propagateDistTxInfoToNodes
 *  @aStatement:
 *  @aClientInfo:
 *  @aDataInfo:
 *
 *  구문 수행 전에 분산정보를 각 노드에 전파한다.
 */
IDE_RC sdi::propagateDistTxInfoToNodes( qcStatement    * aStatement,
                                        sdiClientInfo  * aClientInfo,
                                        sdiDataNodes   * aDataInfo )
{
    sdiConnectInfo * sConnectInfo = aClientInfo->mConnectInfo;
    sdiDataNode    * sDataNode    = aDataInfo->mNodes;

    SInt   i = 0;

    ACP_UNUSED( aStatement );

    #if defined(DEBUG)
    ideLog::log( IDE_SD_18, "[GLOBAL_CONSISTENT_TRANSACTION_DISTTXINFO] = [%s] propagateDistTxInfoToNodes, RequestSCN : %"ID_UINT64_FMT
                            ", TxFirstStmtSCN : %"ID_UINT64_FMT
                            ", TxFirstStmtTime : %"ID_INT64_FMT
                            ", DistLevel : %"ID_UINT32_FMT,
                 aClientInfo->mGCTxInfo.mSessionTypeString,
                 aClientInfo->mGCTxInfo.mRequestSCN,
                 aClientInfo->mGCTxInfo.mTxFirstStmtSCN,
                 aClientInfo->mGCTxInfo.mTxFirstStmtTime,
                 aClientInfo->mGCTxInfo.mDistLevel );
    #endif

    IDE_DASSERT( aClientInfo->mCount == aDataInfo->mCount );

    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sDataNode->mState == SDI_NODE_STATE_EXECUTE_SELECTED )
        {
            IDE_TEST( sdl::setSCN( sConnectInfo, &aClientInfo->mGCTxInfo.mRequestSCN ) != IDE_SUCCESS );
            IDE_TEST( sdl::setTxFirstStmtSCN( sConnectInfo, &aClientInfo->mGCTxInfo.mTxFirstStmtSCN ) != IDE_SUCCESS );
            IDE_TEST( sdl::setTxFirstStmtTime( sConnectInfo, aClientInfo->mGCTxInfo.mTxFirstStmtTime ) != IDE_SUCCESS );
            IDE_TEST( sdl::setDistLevel( sConnectInfo, aClientInfo->mGCTxInfo.mDistLevel ) != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 *  updateMaxNodeSCNToCoordSCN
 *  @aStatement:
 *  @aClientInfo:
 *  @aDataInfo:
 *
 *  구문 수행 후 각 노드의 SCN 값을 수집해 Max 값을 COORD SCN에 업데이트한다.
 */
IDE_RC sdi::updateMaxNodeSCNToCoordSCN( qcStatement    * aStatement,
                                        sdiClientInfo  * aClientInfo,
                                        sdiDataNodes   * aDataInfo )
{
    sdiConnectInfo * sConnectInfo = aClientInfo->mConnectInfo;
    sdiDataNode    * sDataNode    = aDataInfo->mNodes;

    smSCN            sSCN;
    smSCN            sMaxSCN;
    smSCN            sLastSystemSCN;
    SInt             i = 0;

    if ( QCG_GET_SESSION_IS_GCTX( aStatement ) == ID_TRUE )
    {
        SM_INIT_SCN( &sSCN );
        SM_INIT_SCN( &sMaxSCN );

        #if defined(DEBUG)
        ideLog::log( IDE_SD_18, "[GLOBAL_CONSISTENT_TRANSACTION_DISTTXINFO] = [%s] +updateMaxNodeSCNToCoordSCN",
                     aClientInfo->mGCTxInfo.mSessionTypeString );
        #endif

        IDE_DASSERT( aClientInfo->mCount == aDataInfo->mCount );

        for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
        {
            if ( ( sDataNode->mState == SDI_NODE_STATE_EXECUTED ) ||
                 ( sDataNode->mState == SDI_NODE_STATE_EXECUTE_CANDIDATED ) )
            {
                IDE_TEST( sdl::getSCN( sConnectInfo, &sSCN ) != IDE_SUCCESS );

                #if defined(DEBUG)
                ideLog::log( IDE_SD_18, "[GLOBAL_CONSISTENT_TRANSACTION_DISTTXINFO] = [%s] %s SCN : %"ID_UINT64_FMT,
                             aClientInfo->mGCTxInfo.mSessionTypeString,
                             sConnectInfo->mNodeName,
                             sSCN );
                #endif

                SM_SET_MAX_SCN( &sMaxSCN, &sSCN );
            }
        }

        if ( SM_SCN_IS_NOT_INIT( sMaxSCN ) )
        {
            IDE_DASSERT( QCG_GET_SESSION_IS_GCTX( aStatement ) == ID_TRUE );

            SM_SET_MAX_SCN( &(aClientInfo->mGCTxInfo.mCoordSCN), &sMaxSCN );

            IDU_FIT_POINT( "sdi::updateMaxNodeSCNToCoordSCN::statementEndSync" );

            /* PROJ-2733-DistTxInfo Statement SCN Sync (statement end) */
            IDE_TEST( syncSystemSCN4GCTx( &(aClientInfo->mGCTxInfo.mCoordSCN),
                                          &sLastSystemSCN )
                      != IDE_SUCCESS );

            /* PROJ-2733-DistTxInfo */
            SM_SET_MAX_SCN( &(aClientInfo->mGCTxInfo.mCoordSCN), &sLastSystemSCN );

            #if defined(DEBUG)
            ideLog::log( IDE_SD_18, "[GLOBAL_CONSISTENT_TRANSACTION_DISTTXINFO] = [%s] SCN to CoordSCN : %"ID_UINT64_FMT,
                         aClientInfo->mGCTxInfo.mSessionTypeString,
                         aClientInfo->mGCTxInfo.mCoordSCN );
            #endif
        }

        #if defined(DEBUG)
        ideLog::log( IDE_SD_18, "[GLOBAL_CONSISTENT_TRANSACTION_DISTTXINFO] = [%s] -updateMaxNodeSCNToCoordSCN",
                     aClientInfo->mGCTxInfo.mSessionTypeString );
        #endif
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2733-DistTxInfo */
IDE_RC sdi::getSCN( sdiConnectInfo * aConnectInfo, smSCN * aSCN )
{
    return sdl::getSCN( aConnectInfo, aSCN );
}

IDE_RC sdi::setSCN( sdiConnectInfo * aConnectInfo, smSCN * aSCN )
{
    return sdl::setSCN( aConnectInfo, aSCN );
}

IDE_RC sdi::setTxFirstStmtSCN( sdiConnectInfo * aConnectInfo, smSCN * aTxFirstStmtSCN )
{
    return sdl::setTxFirstStmtSCN( aConnectInfo, aTxFirstStmtSCN );
}

IDE_RC sdi::setTxFirstStmtTime( sdiConnectInfo * aConnectInfo, SLong aTxFirstStmtTime )
{
    return sdl::setTxFirstStmtTime( aConnectInfo, aTxFirstStmtTime );
}

IDE_RC sdi::setDistLevel( sdiConnectInfo * aConnectInfo, sdiDistLevel aDistLevel )
{
    return sdl::setDistLevel( aConnectInfo, aDistLevel );
}

/* PROJ-2733-DistTxInfo Distribution Transaction Information */

/**
 *  calcDistTxInfo
 *  @aTemplate:
 *  @aDataInfo:
 *
 *  분산정보(TxFirstStmtSCN, TxFirstStmtTime, DistLevel)를 계산해서
 *  ClientInfo->mGCTxInfo에 업데이트 한다.
 */
void sdi::calcDistTxInfo( qcTemplate   * aTemplate,
                          sdiDataNodes * aDataInfo,
                          idBool         aCalledByPSM )
{
    sdiClientInfo  * sClientInfo  = aTemplate->stmt->session->mQPSpecific.mClientInfo;
    sdiConnectInfo * sConnectInfo = sClientInfo->mConnectInfo;
    qcStatement    * sStatement   = aTemplate->stmt;
    sdiDataNode    * sDataNode    = aDataInfo->mNodes;

    idBool       sIsGCTx     = QCG_GET_SESSION_IS_GCTX( aTemplate->stmt );
    smSCN        sRequestSCN;
    smSCN        sTxFirstStmtSCN;
    SLong        sTxFirstStmtTime = 0;
    smSCN        sLastSystemSCN;
    sdiDistLevel sDistLevel = SDI_DIST_LEVEL_INIT;
    SInt         i = 0;
    UInt         sNodeCount = 0;
    SShort       sBeforeExecutedDataNodeIndex = -1;

    PDL_Time_Value sCurTime;

    sCurTime.initialize();
    SM_INIT_SCN( &sRequestSCN );
    SM_INIT_SCN( &sTxFirstStmtSCN );

    ACP_UNUSED( aCalledByPSM );

    qci::mSessionCallback.mGetStatementRequestSCN( QC_MM_STMT( sStatement ), &sRequestSCN );
    qci::mSessionCallback.mGetStatementTxFirstStmtSCN( QC_MM_STMT( sStatement ), &sTxFirstStmtSCN );
    sTxFirstStmtTime = qci::mSessionCallback.mGetStatementTxFirstStmtTime( QC_MM_STMT( sStatement ) );
    sDistLevel = qci::mSessionCallback.mGetStatementDistLevel( QC_MM_STMT( sStatement ) );

#if defined(DEBUG)
    /* BUG-48109 Non-GCTx, RequestSCN : 0, TxFirstStmtSCN : 1 */
    if ( ( sIsGCTx != ID_TRUE ) && ( sConnectInfo->mIsShardClient == SDI_SHARD_CLIENT_TRUE ) )
    {
        IDE_DASSERT( ( SM_SCN_IS_INIT( sRequestSCN ) ) &&
                     ( SM_SCN_IS_NON_GCTX_TX_FIRST_STMT_SCN( sTxFirstStmtSCN ) ) );
    }
#endif

    #if defined(DEBUG)
    ideLog::log( IDE_SD_18, "[GLOBAL_CONSISTENT_TRANSACTION_DISTTXINFO] = [%s] calcDistTxInfo From Client GCTx : %s"
                            ", SCN : %"ID_UINT64_FMT
                            ", TxFirstStmtSCN : %"ID_UINT64_FMT
                            ", TxFirstStmtTime : %"ID_INT64_FMT
                            ", DistLevel : %"ID_UINT32_FMT,
                 sClientInfo->mGCTxInfo.mSessionTypeString,
                 (sIsGCTx == ID_TRUE) ? "True" : "False",
                 sRequestSCN,
                 sTxFirstStmtSCN,
                 sTxFirstStmtTime,
                 sDistLevel );
    #endif

    /* PROJ-2733-DistTxInfo
     *  (server-side execute)
     *  성능에 문제되지 않는다면
     *  요구자 SCN 또는 Coordinator SCN 이 현재 LastSystemSCN 보다 작으면 업데이트 하자
     */
    if ( sIsGCTx == ID_TRUE )
    {
        getSystemSCN4GCTx( &sLastSystemSCN );
        SM_SET_MAX_SCN( &sClientInfo->mGCTxInfo.mCoordSCN, &sLastSystemSCN );
    }

    if ( sConnectInfo->mCoordinatorType == SDI_COORDINATOR_SHARD )
    {
        /* User 세션이 ShardCLI에서 접속한 경우 Client에서 보낸 값을 사용해야 한다. */
        if ( sConnectInfo->mIsShardClient == SDI_SHARD_CLIENT_TRUE )
        {
            SM_SET_SCN( &sClientInfo->mGCTxInfo.mTxFirstStmtSCN, &sTxFirstStmtSCN );
            sClientInfo->mGCTxInfo.mTxFirstStmtTime = sTxFirstStmtTime;
            sClientInfo->mGCTxInfo.mDistLevel       = sDistLevel;
        }
        else
        {
#if defined(DEBUG)
            if (sIsGCTx == ID_TRUE)
            {
                IDE_DASSERT(  ( ( SM_SCN_IS_INIT( sClientInfo->mGCTxInfo.mTxFirstStmtSCN ) ) &&
                                ( sClientInfo->mGCTxInfo.mTxFirstStmtTime == 0 ) )
                              ||
                              ( ( SM_SCN_IS_NOT_INIT( sClientInfo->mGCTxInfo.mTxFirstStmtSCN ) ) &&
                                ( SM_SCN_IS_NOT_NON_GCTX_TX_FIRST_STMT_SCN( sClientInfo->mGCTxInfo.mTxFirstStmtSCN ) ) &&
                                ( sClientInfo->mGCTxInfo.mTxFirstStmtTime != 0 ) )  );
            }
            else
            {
                IDE_DASSERT(  ( ( SM_SCN_IS_INIT( sClientInfo->mGCTxInfo.mTxFirstStmtSCN ) ) &&
                                ( sClientInfo->mGCTxInfo.mTxFirstStmtTime == 0 ) )
                              ||
                              ( ( SM_SCN_IS_NON_GCTX_TX_FIRST_STMT_SCN( sClientInfo->mGCTxInfo.mTxFirstStmtSCN ) ) &&
                                ( sClientInfo->mGCTxInfo.mTxFirstStmtTime != 0 ) )  );
            }
#endif

            /*
             * GCTx (or GTx) 인 경우 Coordinator session은 항상 Nonautocommit 모드
             * commit이나 commitmode 변경시 endTranDistTx()에서 mTxFirst*를 0으로 설정한다.
             */
            if ( SM_SCN_IS_INIT( sClientInfo->mGCTxInfo.mTxFirstStmtSCN ) )
            {
                if ( sIsGCTx == ID_TRUE )
                {
                    SM_SET_SCN( &sClientInfo->mGCTxInfo.mTxFirstStmtSCN, &sClientInfo->mGCTxInfo.mCoordSCN );
                }
                else
                {
                    /* BUG-48109 Non-GCTx */
                    SM_SET_NON_GCTX_TX_FIRST_STMT_SCN( &sClientInfo->mGCTxInfo.mTxFirstStmtSCN );
                }
            }
            if ( sClientInfo->mGCTxInfo.mTxFirstStmtTime == 0 )
            {
                sCurTime = idlOS::gettimeofday();
                sClientInfo->mGCTxInfo.mTxFirstStmtTime = sCurTime.msec();
            }
        }
    }
    else  /* SDI_COORDINATOR_RESHARD */
    {
        SM_SET_SCN( &sClientInfo->mGCTxInfo.mTxFirstStmtSCN, &sTxFirstStmtSCN );
        sClientInfo->mGCTxInfo.mTxFirstStmtTime = sTxFirstStmtTime;

        /* Resharding된 경우 PARALLEL은 유지하고 그 외는 MULTI로 변경 */
        if ( sDistLevel == SDI_DIST_LEVEL_PARALLEL)
        {
            sClientInfo->mGCTxInfo.mDistLevel = SDI_DIST_LEVEL_PARALLEL;
        }
        else
        {
            sClientInfo->mGCTxInfo.mDistLevel = SDI_DIST_LEVEL_MULTI;
        }
    }

    for ( i = 0; i < aDataInfo->mCount; i++, sDataNode++ )
    {
        if ( sDataNode->mState == SDI_NODE_STATE_EXECUTE_SELECTED )
        {
            if (sBeforeExecutedDataNodeIndex == -1)
            {
                sBeforeExecutedDataNodeIndex = i;
            }
            sNodeCount += 1;
        }
    }

    if ( ( sNodeCount >= 2 ) ||
         ( aTemplate->shardExecData.partialExecType == SDI_SHARD_PARTIAL_EXEC_TYPE_COORD ) )
    {
        /* BUG-48831 Partial coordinating SQL의 경우 수행 노드가 하나일 경우에도 dist level을 parallel로 가져간다. */
        sClientInfo->mGCTxInfo.mDistLevel = SDI_DIST_LEVEL_PARALLEL;
    }
    else
    {
        /* 노드가 1개인 경우 */
        switch ( sClientInfo->mGCTxInfo.mDistLevel )
        {
            case SDI_DIST_LEVEL_INIT:
                sClientInfo->mGCTxInfo.mBeforeExecutedDataNodeIndex = sBeforeExecutedDataNodeIndex;
                sClientInfo->mGCTxInfo.mDistLevel = SDI_DIST_LEVEL_SINGLE;
                break;

            case SDI_DIST_LEVEL_SINGLE:
                IDE_DASSERT( sClientInfo->mGCTxInfo.mBeforeExecutedDataNodeIndex != -1 );
                if ( sClientInfo->mGCTxInfo.mBeforeExecutedDataNodeIndex != sBeforeExecutedDataNodeIndex )
                {
                    sClientInfo->mGCTxInfo.mDistLevel = SDI_DIST_LEVEL_MULTI;
                }
                break;

            case SDI_DIST_LEVEL_MULTI:
                break;

            case SDI_DIST_LEVEL_PARALLEL:
                /* RESHARD일때는 수행노드가 1개라도 COORD에서 보낸 PARALLEL을 유지해야 한다. */
                if ( sConnectInfo->mCoordinatorType == SDI_COORDINATOR_SHARD )
                {
                    sClientInfo->mGCTxInfo.mDistLevel = SDI_DIST_LEVEL_MULTI;
                }
                break;

            default:
                IDE_DASSERT(0);
                sClientInfo->mGCTxInfo.mDistLevel = SDI_DIST_LEVEL_MULTI;  /* Release */
                break;
        }
    }
}

/**
 *  decideRequestSCN
 *  @aTemplate:
 *
 *  분산정보(RequestSCN)를 계산해서 ClientInfo->mGCTxInfo에 업데이트 한다.
 */
void sdi::decideRequestSCN( qcTemplate * aTemplate,
                            idBool       aCalledByPSM,
                            UInt         aPlanIndex )
{
    qcStatement     * sStatement     = aTemplate->stmt;
    qcShardExecData * sExecData      = &aTemplate->shardExecData;
    sdiClientInfo   * sClientInfo    = aTemplate->stmt->session->mQPSpecific.mClientInfo;
    sdiConnectInfo  * sConnectInfo   = sClientInfo->mConnectInfo;
    idBool            sIsGCTx        = QCG_GET_SESSION_IS_GCTX( aTemplate->stmt );

    smSCN             sClientRequestSCN;
    smSCN             sRequestSCN;

    SM_INIT_SCN( &sRequestSCN );
    SM_INIT_SCN( &sClientRequestSCN );

    IDE_TEST_CONT( sIsGCTx != ID_TRUE, SKIP_DECIDE_REQUEST_SCN );

    /* SCN from client */
    qci::mSessionCallback.mGetStatementRequestSCN( QC_MM_STMT( sStatement ), &sClientRequestSCN );

    SM_SET_MAX_SCN( &sClientInfo->mGCTxInfo.mCoordSCN, &sClientRequestSCN );

    if ( aCalledByPSM == ID_FALSE )
    {
        /* NOT-PSM */
        if ( sConnectInfo->mIsShardClient == SDI_SHARD_CLIENT_TRUE )
        {
            /* NOT-PSM + SDI_SHARD_CLIENT_TRUE */
            if ( sConnectInfo->mCoordinatorType == SDI_COORDINATOR_SHARD )
            {
                /* NOT-PSM + SDI_SHARD_CLIENT_TRUE + SDI_COORDINATOR_SHARD */
                IDE_DASSERT( SM_SCN_IS_INIT( sClientRequestSCN ) == 0 );

                if ( sExecData->partialStmt == ID_TRUE )
                {
                    /* SC01 */
                    IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_SHARD-CLIENT_COORD-SHARD_PARTIAL-STMT" );

                    SM_SET_SCN( &sRequestSCN, &sClientInfo->mGCTxInfo.mCoordSCN );
                }
                else
                {
                    switch ( sClientInfo->mGCTxInfo.mDistLevel )
                    {
                        case SDI_DIST_LEVEL_SINGLE:
                        case SDI_DIST_LEVEL_MULTI:
                            /* SC02 */
                            /* Nothing to do */
                            IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_SHARD-CLIENT_COORD-SHARD_NOT-PARTIAL-STMT_DIST-MULTI" );
                            break;

                        case SDI_DIST_LEVEL_PARALLEL:
                            /* SC03 */
                            IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_SHARD-CLIENT_COORD-SHARD_NOT-PARTIAL-STMT_DIST-PARALLEL" );
                            SM_SET_SCN( &sRequestSCN, &sClientInfo->mGCTxInfo.mCoordSCN );
                            break;

                        default:
                            IDE_DASSERT( 0 );
                            break;
                    }
                }
            }
            else
            {
                /* NOT-PSM + SDI_SHARD_CLIENT_TRUE + SDI_COORDINATOR_RESHARD */
                if ( SM_SCN_IS_INIT( sClientRequestSCN ) )
                {
                    /* Recv Request SCN == 0 */
                    if ( sExecData->partialStmt == ID_TRUE )
                    {
                        /* SR04 */
                        IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_SHARD-CLIENT_COORD-RESHARD_NO-REQ-SCN_PARTIAL-STMT" );

                        SM_SET_SCN( &sRequestSCN, &sClientInfo->mGCTxInfo.mCoordSCN );
                    }
                    else
                    {
                        switch ( sClientInfo->mGCTxInfo.mDistLevel )
                        {
                            case SDI_DIST_LEVEL_SINGLE:
                            case SDI_DIST_LEVEL_MULTI:
                                /* SR05 */
                                IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_SHARD-CLIENT_COORD-RESHARD_NO-REQ-SCN_NOT-PARTIAL-STMT_DIST-MULTI" );
                                /* Nothing to do */
                                break;

                            case SDI_DIST_LEVEL_PARALLEL:
                                /* SR06 */
                                IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_SHARD-CLIENT_COORD-RESHARD_NO-REQ-SCN_NOT-PARTIAL-STMT_DIST-PARALLEL" );
                                SM_SET_SCN( &sRequestSCN, &sClientInfo->mGCTxInfo.mCoordSCN );
                                break;

                            default:
                                IDE_DASSERT( 0 );
                                break;
                        }
                    }
                }
                else
                {
                    /* Recv Request SCN != 0 */
#ifdef ALTIBASE_FIT_CHECK
                    if ( sExecData->partialStmt == ID_TRUE )
                    {
                        /* SR01 */
                        IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_SHARD-CLIENT_COORD-RESHARD_REQ-SCN_PARTIAL-STMT" );
                    }
                    else
                    {
                        switch ( sClientInfo->mGCTxInfo.mDistLevel )
                        {
                            case SDI_DIST_LEVEL_SINGLE:
                            case SDI_DIST_LEVEL_MULTI:
                                /* SR02 */
                                IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_SHARD-CLIENT_COORD-RESHARD_REQ-SCN_NOT-PARTIAL-STMT_DIST-MULTI" );
                                break;

                            case SDI_DIST_LEVEL_PARALLEL:
                                /* SR03 */
                                IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_SHARD-CLIENT_COORD-RESHARD_REQ-SCN_NOT-PARTIAL-STMT_DIST-PARALLEL" );
                                break;

                            default:
                                IDE_DASSERT( 0 );
                                break;
                        }
                    }
#endif

                    SM_SET_SCN( &sRequestSCN, &sClientRequestSCN );
                }
            }
        }
        else
        {
            /* NOT-PSM + SDI_SHARD_CLIENT_FALSE */
            if ( sConnectInfo->mCoordinatorType == SDI_COORDINATOR_SHARD )
            {
                /* NOT-PSM + SDI_SHARD_CLIENT_FALSE + SDI_COORDINATOR_SHARD */
                IDE_DASSERT( SM_SCN_IS_INIT( sClientRequestSCN ) );

                if ( sExecData->partialStmt == ID_TRUE )
                {
                    /* OC01 */
                    IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_ODBC-CLIENT_COORD-SHARD_PARTIAL-STMT" );

                    SM_SET_SCN( &sRequestSCN, &sClientInfo->mGCTxInfo.mCoordSCN );
                }
                else
                {
                    switch ( sClientInfo->mGCTxInfo.mDistLevel )
                    {
                        case SDI_DIST_LEVEL_SINGLE:
                        case SDI_DIST_LEVEL_MULTI:
                            /* OC02 */
                            IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_ODBC-CLIENT_COORD-SHARD_NOT-PARTIAL-STMT_DIST-MULTI" );
                            /* Nothing to do */
                            break;

                        case SDI_DIST_LEVEL_PARALLEL:
                            /* OC03 */
                            IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_ODBC-CLIENT_COORD-SHARD_NOT-PARTIAL-STMT_DIST-PARALLEL" );

                            SM_SET_SCN( &sRequestSCN, &sClientInfo->mGCTxInfo.mCoordSCN );
                            break;

                        default:
                            IDE_DASSERT( 0 );
                            break;
                    }
                }
            }
            else
            {
                /* NOT-PSM + SDI_SHARD_CLIENT_FALSE + SDI_COORDINATOR_RESHARD */
                if ( SM_SCN_IS_INIT( sClientRequestSCN ) )
                {
                    /* Recv Request SCN == 0 */
                    if ( sExecData->partialStmt == ID_TRUE )
                    {
                        /* OR04 */
                        IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_ODBC-CLIENT_COORD-RESHARD_NO-REQ-SCN_PARTIAL-STMT" );

                        SM_SET_SCN( &sRequestSCN, &sClientInfo->mGCTxInfo.mCoordSCN );
                    }
                    else
                    {
                        switch ( sClientInfo->mGCTxInfo.mDistLevel )
                        {
                            case SDI_DIST_LEVEL_SINGLE:
                            case SDI_DIST_LEVEL_MULTI:
                                /* OR05 */
                                IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_ODBC-CLIENT_COORD-RESHARD_NO-REQ-SCN_NOT-PARTIAL-STMT_DIST-MULTI" );
                                /* Nothing to do */
                                break;

                            case SDI_DIST_LEVEL_PARALLEL:
                                /* OR06 */
                                IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_ODBC-CLIENT_COORD-RESHARD_NO-REQ-SCN_NOT-PARTIAL-STMT_DIST-PARALLEL" );

                                SM_SET_SCN( &sRequestSCN, &sClientInfo->mGCTxInfo.mCoordSCN );
                                break;

                            default:
                                IDE_DASSERT( 0 );
                                break;
                        }
                    }
                }
                else
                {
                    /* Recv Request SCN != 0 */
#ifdef ALTIBASE_FIT_CHECK
                    if ( sExecData->partialStmt == ID_TRUE )
                    {
                        /* OR01 */
                        IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_ODBC-CLIENT_COORD-RESHARD_REQ-SCN_PARTIAL-STMT" );
                    }
                    else
                    {
                        switch ( sClientInfo->mGCTxInfo.mDistLevel )
                        {
                            case SDI_DIST_LEVEL_SINGLE:
                            case SDI_DIST_LEVEL_MULTI:
                                /* OR02 */
                                IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_ODBC-CLIENT_COORD-RESHARD_REQ-SCN_NOT-PARTIAL-STMT_DIST-MULTI" );
                                break;

                            case SDI_DIST_LEVEL_PARALLEL:
                                /* OR03 */
                                IDU_FIT_POINT( "sdi::decideRequestSCN_NOT-PSM_ODBC-CLIENT_COORD-RESHARD_REQ-SCN_NOT-PARTIAL-STMT_DIST-PARALLEL" );
                                break;

                            default:
                                IDE_DASSERT( 0 );
                                break;
                        }
                    }
#endif

                    SM_SET_SCN( &sRequestSCN, &sClientRequestSCN );
                }
            }
        }
    }
    else
    {
        /* aCalledByPSM == ID_TRUE */
        if ( sExecData->partialStmt == ID_TRUE )
        {
            IDU_FIT_POINT( "sdi::decideRequestSCN_PSM_PARTIAL-STMT" );

            SM_SET_SCN( &sRequestSCN, &sClientInfo->mGCTxInfo.mCoordSCN );
        }
        else
        {
            IDU_FIT_POINT( "sdi::decideRequestSCN_PSM_NOT-PARTIAL-STMT" );

            switch ( sClientInfo->mGCTxInfo.mDistLevel )
            {
                case SDI_DIST_LEVEL_SINGLE:
                case SDI_DIST_LEVEL_MULTI:
                    /* Nothing to do */
                    break;

                case SDI_DIST_LEVEL_PARALLEL:
                    SM_SET_SCN( &sRequestSCN, &sClientInfo->mGCTxInfo.mCoordSCN );
                    break;

                default:
                    IDE_DASSERT( 0 );
                    break;
            }
        }
    }

    if ( sExecData->partialStmt == ID_TRUE )
    {
        if ( SM_SCN_IS_INIT( sExecData->leadingRequestSCN ) )
        {
            /* first time execution of partial statement */
            IDE_DASSERT( sExecData->leadingPlanIndex == ID_UINT_MAX );

            sExecData->leadingPlanIndex = aPlanIndex;

            SM_SET_SCN( &sExecData->leadingRequestSCN, &sRequestSCN );
        }
        else if ( sExecData->leadingPlanIndex == aPlanIndex )
        {
            /* shard statement retry */
            SM_SET_SCN( &sExecData->leadingRequestSCN, &sRequestSCN );
        }
        else
        {
            /* Not first statement. use leadingRequestSCN.
             * Nothing to do. */
        }
    }
    else
    {
        SM_SET_SCN( &sExecData->leadingRequestSCN, &sRequestSCN );
    }

    IDE_EXCEPTION_CONT( SKIP_DECIDE_REQUEST_SCN );

    /* BUG-48109 Non-GCTx */
    if ( sIsGCTx == ID_TRUE )
    {
        SM_SET_SCN( &sClientInfo->mGCTxInfo.mRequestSCN, &sExecData->leadingRequestSCN );
    }
    else
    {
        /* CoordSCN은 유지하자. */
        SM_INIT_SCN( &sClientInfo->mGCTxInfo.mRequestSCN );

#if defined(DEBUG)
        if ( sConnectInfo->mIsShardClient == SDI_SHARD_CLIENT_TRUE )
        {
            IDE_DASSERT( ( SM_SCN_IS_INIT( sClientInfo->mGCTxInfo.mRequestSCN ) ) &&
                         ( SM_SCN_IS_NON_GCTX_TX_FIRST_STMT_SCN( sClientInfo->mGCTxInfo.mTxFirstStmtSCN ) ) );
        }
#endif
    }

    #if defined(DEBUG)
    ideLog::log( IDE_SD_18, "[GLOBAL_CONSISTENT_TRANSACTION_DISTTXINFO] = [%s] decideRequestSCN "
                            "PSM: %"ID_UINT32_FMT
                            ", SHARDCLI: %"ID_UINT32_FMT
                            ", COORD: %"ID_UINT32_FMT
                            ", PARTIAL: %"ID_UINT32_FMT
                            ", GCTxInfo, Coord SCN : %"ID_UINT64_FMT
                            ", RequestSCN : %"ID_UINT64_FMT
                            ", TxFirstStmtSCN : %"ID_UINT64_FMT
                            ", TxFirstStmtTime : %"ID_INT64_FMT
                            ", DistLevel : %"ID_UINT32_FMT,
                 sClientInfo->mGCTxInfo.mSessionTypeString,
                 aCalledByPSM,
                 sConnectInfo->mIsShardClient,
                 sConnectInfo->mCoordinatorType,
                 sExecData->partialStmt,
                 sClientInfo->mGCTxInfo.mCoordSCN,
                 sClientInfo->mGCTxInfo.mRequestSCN,
                 sClientInfo->mGCTxInfo.mTxFirstStmtSCN,
                 sClientInfo->mGCTxInfo.mTxFirstStmtTime,
                 sClientInfo->mGCTxInfo.mDistLevel );
    #endif

#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION_END;
#endif
}

void sdi::calculateGCTxInfo( qcTemplate   * aTemplate,
                             sdiDataNodes * aDataInfo,
                             idBool         aCalledByPSM,
                             UInt           aPlanIndex )
{
    /* BUG-48109 Non-GCTx */
    calcDistTxInfo( aTemplate, aDataInfo, aCalledByPSM );
    decideRequestSCN( aTemplate, aCalledByPSM, aPlanIndex );
}

/**
 *  buildDataNodes
 *  @aDataInfo:
 *  @aClientInfo:
 *  @aNodeName:
 *
 *  aNodeName에 해당하는 Node들의 State를 설정한다.
 *  aNodeName이 NULL이나 빈스트링이면 모든 Node를 의미한다.
 *  aSelectedNodeCnt에 선택된 노드의 수를 반환한다.
 */
IDE_RC sdi::buildDataNodes( sdiClientInfo * aClientInfo,
                            sdiDataNodes  * aDataInfo,
                            SChar         * aNodeName,
                            UInt          * aSelectedNodeCnt )
{
    sdiConnectInfo * sConnectInfo = aClientInfo->mConnectInfo;
    sdiDataNode    * sDataNode    = aDataInfo->mNodes;

    idBool sIsAllNodeExec = ID_TRUE;
    UInt i = 0;
    UInt sSelectedNodeCnt = 0;

    if ( aNodeName != NULL )
    {
        if ( aNodeName[0] != '\0' )
        {
            sIsAllNodeExec = ID_FALSE;
        }
    }

    for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sIsAllNodeExec == ID_TRUE )
        {
            sDataNode->mState = SDI_NODE_STATE_EXECUTE_SELECTED;
            sSelectedNodeCnt += 1;
        }
        else if ( idlOS::strMatch( aNodeName,
                                   idlOS::strlen( aNodeName ),
                                   sConnectInfo->mNodeName,
                                   idlOS::strlen( sConnectInfo->mNodeName ) ) == 0 )
        {
            sDataNode->mState = SDI_NODE_STATE_EXECUTE_SELECTED;
            sSelectedNodeCnt += 1;
        }
        else
        {
            sDataNode->mState = SDI_NODE_STATE_NONE;
        }
    }

    aDataInfo->mCount = aClientInfo->mCount;
    aDataInfo->mInitialized = ID_TRUE;

    if (aSelectedNodeCnt != NULL)
    {
        *aSelectedNodeCnt = sSelectedNodeCnt;
    }

    /* BUG-48666 선택된 노드가 없으면 NODE 이름이 잘못된 경우이다. */
    IDE_TEST_RAISE( sSelectedNodeCnt == 0, ERR_NODE_NOT_EXIST );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NODE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void sdi::checkErrorIsShardRetry( sdiStmtShardRetryType * aRetryType )
{
    UInt                    sErrCnt_StmtViewOld = 0; 
    UInt                    sErrCnt_RebuilRetry = 0; 
    UInt                    sErrCnt_SMNProp     = 0; 
    UInt                    sErrCnt_Others      = 0; 

    iduListNode             * sIterator         = NULL;
    iduList                 * sErrorList        = &(gIdeErrorMgr.mErrors.mErrorList);
    ideErrorCollectionStack * sError            = NULL;

    *aRetryType = SDI_STMT_SHARD_RETRY_NONE;

    /* GTx, GCTx 인 경우
     * Coordinator session 은 autocommit off 로 동작하기 때문에
     * user session 이 autocommit on 인 경우도 shard statement retry 기능 동작이 가능하다. */

    if ( ideErrorCollectionSize() != 0 )
    {
        IDU_LIST_ITERATE( sErrorList, sIterator )
        {
            sError = (ideErrorCollectionStack *)sIterator->mObj;

            if ( E_ERROR_CODE( sError->mErrorCode ) ==
                 E_ERROR_CODE( sdERR_ABORT_SHARD_META_OUT_OF_DATE ) )
            {
                ++sErrCnt_RebuilRetry;
            }
            else if ( E_ERROR_CODE( sError->mErrorCode ) ==
                      E_ERROR_CODE( smERR_ABORT_StatementTooOld ) )
            {
                ++sErrCnt_StmtViewOld;
            }
            else if ( E_ERROR_CODE( sError->mErrorCode ) ==
                      E_ERROR_CODE( sdERR_ABORT_SHARD_MULTIPLE_ERRORS ) )
            {
                /*  Nothig to do */
            }
            else
            {
                ++sErrCnt_Others;
                break;
            }
        }
    }
    else
    {
        if ( ideGetErrorCode() == sdERR_ABORT_FAILED_TO_PROPAGATE_SHARD_META_NUMBER )
        {
            ++sErrCnt_SMNProp;
        }
        else if ( ideGetErrorCode() == sdERR_ABORT_SHARD_META_OUT_OF_DATE )
        {
            ++sErrCnt_RebuilRetry;
        }
        else if ( ideGetErrorCode() == smERR_ABORT_StatementTooOld )
        {
            ++sErrCnt_StmtViewOld;
        }
        else if ( ideGetErrorCode() == sdERR_ABORT_SHARD_MULTIPLE_ERRORS )
        {
            /* Nothing to do */
        }
        else
        {
            ++sErrCnt_Others;
        }
    }

    if ( sErrCnt_SMNProp != 0 )
    {
        /* SMN Propagation event 를 우선한다.
         */
        *aRetryType = SDI_STMT_SHARD_SMN_PROPAGATION;
    }
    else if ( sErrCnt_Others == 0 )
    {
        if ( sErrCnt_RebuilRetry != 0 )
        {
            /* Rebuild event 를 우선한다.
             * Statement Too Old 에러가 발생했을때 Rebuild Retry 를 수행해도 된다.
             */
            *aRetryType = SDI_STMT_SHARD_REBUILD_RETRY;
        }
        else if ( sErrCnt_StmtViewOld != 0 )
        {
            /* Statement Too Old 에러 인 경우 */
            *aRetryType = SDI_STMT_SHARD_VIEW_OLD_RETRY;
        }
        else
        {
            IDE_DASSERT( 0 );
        }
    }
}

idBool sdi::isNeedRequestSCN( qciStatement * aStatement )
{
    idBool      sIsSupportGCTx = ID_FALSE;
    qciStmtType sStmtType      = QCI_STMT_MASK_MAX;

    IDE_TEST_CONT( isShardCoordinator( &aStatement->statement ) == ID_TRUE,
                   EndOfFunction );

    IDE_TEST_CONT( qci::getStmtType( aStatement, &sStmtType ) != IDE_SUCCESS,
                   EndOfFunction );

    IDE_TEST_CONT( qciMisc::isStmtDML( sStmtType ) == ID_FALSE,
                   EndOfFunction );

    sIsSupportGCTx = ID_TRUE;

    IDE_EXCEPTION_CONT( EndOfFunction );

    return sIsSupportGCTx;
}


IDE_RC sdi::makeShardMeta4NewSMN( qcStatement * aStatement )
{
    IDE_TEST( sdm::makeShardMeta4NewSMN( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::getGlobalMetaInfoCore( smiStatement          * aSmiStmt,
                                   sdiGlobalMetaInfo * aMetaNodeInfo )
{
    IDE_TEST( sdm::getGlobalMetaInfoCore( aSmiStmt,
                                          aMetaNodeInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::getAllReplicaSetInfoSortedByPName( smiStatement       * aSmiStmt,
                                               sdiReplicaSetInfo  * aReplicaSetsInfo,
                                               ULong                aSMN )
{
    IDE_TEST( sdm::getAllReplicaSetInfoSortedByPName( aSmiStmt,
                                                      aReplicaSetsInfo,
                                                      aSMN )
              != IDE_SUCCESS );

    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::getTableInfoAllObject( qcStatement        * aStatement,
                                   sdiTableInfoList  ** aTableInfoList,
                                   ULong                aSMN )
{
    IDE_TEST( sdm::getTableInfoAllObject( aStatement,
                                          aTableInfoList,
                                          aSMN )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::getRangeInfo( qcStatement  * aStatement,
                          smiStatement * aSmiStmt,
                          ULong          aSMN,
                          sdiTableInfo * aTableInfo,
                          sdiRangeInfo * aRangeInfo,
                          idBool         aNeedMerge )
{                                                                                            
    IDE_TEST( sdm::getRangeInfo ( aStatement,
                                  aSmiStmt,
                                  aSMN,
                                  aTableInfo,
                                  aRangeInfo,
                                  aNeedMerge ) 
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::getNodeByName( smiStatement * aSmiStmt,
                           SChar        * aNodeName,
                           ULong          aSMN,
                           sdiNode      * aNode )
{
    IDE_TEST( sdm::getNodeByName( aSmiStmt,
                                  aNodeName,
                                  aSMN,
                                  aNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::addReplTable( qcStatement * aStatement,
                          SChar       * aNodeName,
                          SChar       * aReplName,
                          SChar       * aUserName,
                          SChar       * aTableName,
                          SChar       * aPartitionName,
                          sdiReplDirectionalRole aRole,
                          idBool        aIsNewTrans )
{
    if ( aRole == SDI_REPL_SENDER )
    {   
        IDE_TEST( sdm::addReplTable( aStatement,
                                     aNodeName,
                                     aReplName,
                                     aUserName,
                                     aTableName,
                                     aPartitionName,
                                     SDM_REPL_SENDER,
                                     aIsNewTrans )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sdm::addReplTable( aStatement,
                                     aNodeName,
                                     aReplName,
                                     aUserName,
                                     aTableName,
                                     aPartitionName,
                                     SDM_REPL_RECEIVER,
                                     aIsNewTrans )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::findSendReplInfoFromReplicaSet( sdiReplicaSetInfo   * aReplicaSetInfo,
                                            SChar               * aNodeName,
                                            sdiReplicaSetInfo   * aOutReplicaSetInfo )
{
    IDE_TEST( sdm::findSendReplInfoFromReplicaSet( aReplicaSetInfo,
                                                   aNodeName,
                                                   aOutReplicaSetInfo )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Description : Validate aReShardAttr and extract from node and return it.
 *               if aReShardAttr is clone then aFromNode return Null.
 * IN: aStatement, aReShardAttr, aRangeInfo
 */
IDE_RC sdi::validateOneReshardTable( qcStatement * aStatement,
                                     SChar * aUserName,
                                     SChar * aTableName, 
                                     SChar * aPartitionName, 
                                     sdiObjectInfo * aTableObjInfo,
                                     SChar * aOutFromNodeName,
                                     SChar * aOutDefaultNodeName )
{
    sdiTableInfo      *sTableInfo = &(aTableObjInfo->mTableInfo);
    sdiRangeInfo      *sRangeInfo = &(aTableObjInfo->mRangeInfo);

    SInt              i = 0;
    sdiNode           sNode; 
    sdiShardObjectType sSplitType = SDI_NON_SHARD_OBJECT;

    IDE_TEST_RAISE( idlOS::strncmp(sTableInfo->mUserName, 
                                   aUserName,
                                   QC_MAX_OBJECT_NAME_LEN ) != 0,
                    ERR_USER_NAME );
    IDE_TEST_RAISE( idlOS::strncmp(sTableInfo->mObjectName,
                                   aTableName,
                                   QC_MAX_OBJECT_NAME_LEN) != 0,
                    ERR_TABLE_NAME );

    sSplitType = sdi::getShardObjectType( sTableInfo );
    SDI_INIT_NODE(&sNode);
    switch ( sSplitType )
    {
        case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
            IDE_TEST_RAISE( aPartitionName[0] == '\0' ||
                            SDI_IS_NULL_NAME(aPartitionName) == ID_TRUE,
                            ERR_PARTITION_NAME );

            for ( i = 0; i < sRangeInfo->mCount; i++ )
            {
                IDE_TEST_RAISE( idlOS::strncmp( sRangeInfo->mRanges[i].mPartitionName, 
                                                SDM_NA_STR, 
                                                QC_MAX_OBJECT_NAME_LEN) == 0, ERR_NOT_PARTITIONED_TABLE);

                if( idlOS::strncmp( sRangeInfo->mRanges[i].mPartitionName, aPartitionName, QC_MAX_OBJECT_NAME_LEN) == 0 )
                {
                    IDE_TEST_RAISE(sNode.mNodeId != SDI_NODE_NULL_ID, ERR_TOO_MANY_FROM_NODE);
                    IDE_TEST( sdm::getNodeByID( QC_SMI_STMT( aStatement ),
                                                sRangeInfo->mRanges[i].mNodeId,
                                                aTableObjInfo->mSMN,
                                                &sNode )
                              != IDE_SUCCESS );
                }
            }
            if ( sNode.mNodeId == SDI_NODE_NULL_ID )
            {
                if( idlOS::strncmp( sTableInfo->mDefaultPartitionName, aPartitionName, QC_MAX_OBJECT_NAME_LEN) == 0 )
                {
                    IDE_TEST( sdm::getNodeByID( QC_SMI_STMT( aStatement ),
                                                sTableInfo->mDefaultNodeId,
                                                aTableObjInfo->mSMN,
                                                &sNode )
                            != IDE_SUCCESS );
                }
            }
            break;
        case SDI_CLONE_DIST_OBJECT:
        case SDI_SOLO_DIST_OBJECT:
            IDE_TEST_RAISE( aPartitionName[0] != '\0' &&
                            SDI_IS_NULL_NAME(aPartitionName) != ID_TRUE,
                            ERR_PARTITION_NAME_FOR_SOLO );
            IDE_TEST( sdm::getNodeByID( QC_SMI_STMT( aStatement ),
                                        sRangeInfo->mRanges[0].mNodeId,
                                        aTableObjInfo->mSMN,
                                        &sNode )
                    != IDE_SUCCESS );

            break;
        case SDI_NON_SHARD_OBJECT:
        case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
        default:
            IDE_RAISE(ERR_SPLIT_TYPE);
            break;
    }

    IDE_TEST_RAISE( sNode.mNodeId == SDI_NODE_NULL_ID, ERR_NOT_FOUND_NODE);
    idlOS::strncpy(aOutFromNodeName, sNode.mNodeName, SDI_NODE_NAME_MAX_SIZE);
    aOutFromNodeName[SDI_NODE_NAME_MAX_SIZE] = '\0';

    SDI_INIT_NODE(&sNode);
    if ( sTableInfo->mDefaultNodeId != SDI_NODE_NULL_ID )
    {
        IDE_TEST( sdm::getNodeByID( QC_SMI_STMT( aStatement ),
                                    sTableInfo->mDefaultNodeId,
                                    aTableObjInfo->mSMN,
                                    &sNode )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sNode.mNodeId == SDI_NODE_NULL_ID, ERR_NOT_FOUND_NODE);
        idlOS::strncpy( aOutDefaultNodeName, sNode.mNodeName, SDI_NODE_NAME_MAX_SIZE );
        aOutDefaultNodeName[SDI_NODE_NAME_MAX_SIZE] = '\0';
    }
    else
    {
        SDI_SET_NULL_NAME(aOutDefaultNodeName);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION( ERR_PARTITION_NAME )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_DATA_NODE_NOT_FOUND_BY_PART,
                                  aPartitionName ) );
    }
    IDE_EXCEPTION( ERR_NOT_FOUND_NODE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_DATA_NODE_NOT_FOUND_BY_PART,
                                  aPartitionName ) );
    }
    IDE_EXCEPTION( ERR_PARTITION_NAME_FOR_SOLO )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_PART_NAME_ERROR,
                                  aTableName,
                                  aPartitionName ) );
    }
    IDE_EXCEPTION( ERR_TABLE_NAME )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::validateOneReshardTable",
                                  "Invalid table name" ) );
    }
    IDE_EXCEPTION( ERR_USER_NAME )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::validateOneReshardTable",
                                  "Invalid user name" ) );
    }
    IDE_EXCEPTION( ERR_NOT_PARTITIONED_TABLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::validateOneReshardTable",
                                  "not partitioned table" ) );
    }
    IDE_EXCEPTION( ERR_SPLIT_TYPE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::validateOneReshardTable",
                                  "Invalid split type " ) );
    }
    IDE_EXCEPTION( ERR_TOO_MANY_FROM_NODE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::validateOneReshardTable",
                                  "ERR_TOO_MANY_FROM_NODE " ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::validateOneReshardProc( qcStatement * aStatement,
                                    SChar * aUserName,
                                    SChar * aProcName,
                                    SChar * aKeyValue,
                                    sdiObjectInfo * aProcObjInfo,
                                    SChar * aOutFromNodeName,
                                    SChar * aOutDefaultNodeName )
{
    sdiTableInfo     *sProcInfo = &(aProcObjInfo->mTableInfo);
    sdiRangeInfo     *sRangeInfo = &(aProcObjInfo->mRangeInfo);
    sdiValue          sValueStr;
    SInt              i = 0;
    sdiNode           sNode;
    SChar            *sKeyValue = aKeyValue;
    sdiShardObjectType sSplitType = SDI_NON_SHARD_OBJECT;

    IDE_TEST_RAISE( idlOS::strncmp(sProcInfo->mUserName,
                                   aUserName,
                                   QC_MAX_OBJECT_NAME_LEN ) != 0,
                    ERR_USER_NAME );
    IDE_TEST_RAISE( idlOS::strncmp(sProcInfo->mObjectName,
                                   aProcName,
                                   QC_MAX_OBJECT_NAME_LEN) != 0,
                    ERR_OBJECT_NAME );

    SDI_INIT_NODE(&sNode);
    sSplitType = sdi::getShardObjectType( sProcInfo );
    switch ( sSplitType )
    {
        case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
            if ( aKeyValue != NULL )
            {
                for ( i = 0; i < sRangeInfo->mCount; i++ )
                {
                    IDE_TEST( sdi::getValueStr( sProcInfo->mKeyDataType,
                                                &sRangeInfo->mRanges[i].mValue,
                                                &sValueStr )
                                      != IDE_SUCCESS );
                    if( idlOS::strncmp( (SChar*)sValueStr.mCharMax.value, aKeyValue, SDI_RANGE_VARCHAR_MAX_SIZE ) == 0 )
                    {
                        IDE_TEST_RAISE(sNode.mNodeId != SDI_NODE_NULL_ID, ERR_TOO_MANY_FROM_NODE);
                        IDE_TEST( sdm::getNodeByID( QC_SMI_STMT( aStatement ),
                                                    sRangeInfo->mRanges[i].mNodeId,
                                                    aProcObjInfo->mSMN,
                                                    &sNode )
                                  != IDE_SUCCESS );
                    }
                }
            }
            else
            {
                IDE_TEST( sdm::getNodeByID( QC_SMI_STMT( aStatement ),
                                            sProcInfo->mDefaultNodeId,
                                            aProcObjInfo->mSMN,
                                            &sNode )
                        != IDE_SUCCESS );
                sKeyValue = (SChar*)"DEFAULT";
            }
            break;
        case SDI_CLONE_DIST_OBJECT:
            IDE_TEST_RAISE(aKeyValue != NULL, ERR_SHARD_KEY_CLONE);
            IDE_TEST( sdm::getNodeByID( QC_SMI_STMT( aStatement ),
                                        sRangeInfo->mRanges[0].mNodeId,
                                        aProcObjInfo->mSMN,
                                        &sNode )
                    != IDE_SUCCESS );
            break;
        case SDI_SOLO_DIST_OBJECT:
            IDE_TEST_RAISE(aKeyValue != NULL, ERR_SHARD_KEY_SOLO);
            IDE_TEST( sdm::getNodeByID( QC_SMI_STMT( aStatement ),
                                        sRangeInfo->mRanges[0].mNodeId,
                                        aProcObjInfo->mSMN,
                                        &sNode )
                    != IDE_SUCCESS );
            break;
        case SDI_NON_SHARD_OBJECT:
        case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
        default:
            IDE_RAISE(ERR_SPLIT_TYPE);
            break;
    }

    IDE_TEST_RAISE( sNode.mNodeId == SDI_NODE_NULL_ID, ERR_NOT_FOUND_NODE);
    idlOS::strncpy(aOutFromNodeName, sNode.mNodeName, SDI_NODE_NAME_MAX_SIZE);
    aOutFromNodeName[SDI_NODE_NAME_MAX_SIZE] = '\0';

    SDI_INIT_NODE(&sNode);
    if ( sProcInfo->mDefaultNodeId != SDI_NODE_NULL_ID )
    {
        IDE_TEST( sdm::getNodeByID( QC_SMI_STMT( aStatement ),
                                    sProcInfo->mDefaultNodeId,
                                    aProcObjInfo->mSMN,
                                    &sNode )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sNode.mNodeId == SDI_NODE_NULL_ID, ERR_NOT_FOUND_NODE);
        idlOS::strncpy( aOutDefaultNodeName, sNode.mNodeName, SDI_NODE_NAME_MAX_SIZE );
        aOutDefaultNodeName[SDI_NODE_NAME_MAX_SIZE] = '\0';
    }
    else
    {
        SDI_SET_NULL_NAME(aOutDefaultNodeName);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND_NODE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_DATA_NODE_NOT_FOUND_BY_VALUE,
                                  sKeyValue ) );
    }
    IDE_EXCEPTION( ERR_OBJECT_NAME )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::validateOneReshardProc",
                                  "Invalid table name" ) );
    }
    IDE_EXCEPTION( ERR_USER_NAME )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::validateOneReshardProc",
                                  "Invalid user name" ) );
    }
    IDE_EXCEPTION( ERR_SPLIT_TYPE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::validateOneReshardProc",
                                  "Invalid split type " ) );
    }
    IDE_EXCEPTION( ERR_TOO_MANY_FROM_NODE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::validateOneReshardProc",
                                  "ERR_TOO_MANY_FROM_NODE " ) );
    }
    IDE_EXCEPTION( ERR_SHARD_KEY_CLONE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_SHARD_KEY_ERROR,
                                  aProcName) );
    }
    IDE_EXCEPTION( ERR_SHARD_KEY_SOLO )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_SHARD_KEY_ERROR,
                                  aProcName) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::getReplicaSet( smiTrans          * aTrans,
                           SChar             * aPrimaryNodeName,
                           idBool              aIsShardMetaChanged,
                           ULong               aSMN,
                           sdiReplicaSetInfo * aReplicaSetsInfo )
{
    smiTrans       sTrans;
    smiStatement   sSmiStmt;
    smiStatement * sDummySmiStmt = NULL;
    UShort         sTransStage = 0;
    idBool         sIsStmtBegun = ID_FALSE;

    ULong          sDataSMNForMetaInfo = ID_ULONG(0);
    idvSQL       * sStatistics = NULL;
    smiStatement * sSmiStmtForMetaInfo = NULL;

    if ( aTrans == NULL )
    {
        IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
        sTransStage = 1;

        IDE_TEST( sTrans.begin( &sDummySmiStmt, NULL )
                  != IDE_SUCCESS );
        sTransStage = 2;

        sStatistics = NULL;
        sSmiStmtForMetaInfo = sDummySmiStmt;
    }
    else
    {
        IDE_TEST_RAISE( aTrans->isBegin() == ID_FALSE, ERR_INVALID_TRANS );

        sStatistics = aTrans->getStatistics();
        sSmiStmtForMetaInfo = aTrans->getStatement();
    }

    // PROJ-2701 Online data rebuild
    if ( aIsShardMetaChanged == ID_TRUE )
    {
        /* Shard Meta를 변경한 Transaction이므로 aSMN에 해당하는 Internal Node Info를 얻을 수 있다. */
        sDataSMNForMetaInfo = aSMN;
    }
    else if ( getSMNForMetaNode() < aSMN )
    {
        IDE_TEST( waitAndSetSMNForMetaNode( sStatistics,
                                            sSmiStmtForMetaInfo,
                                            ( SMI_STATEMENT_UNTOUCHABLE | SMI_STATEMENT_MEMORY_CURSOR ),
                                            aSMN,
                                            &sDataSMNForMetaInfo )
                  != IDE_SUCCESS );

        IDE_DASSERT( aSMN == sDataSMNForMetaInfo );
    }
    else
    {
        sDataSMNForMetaInfo = aSMN;
    }

    IDE_TEST( sSmiStmt.begin( sStatistics,
                              sSmiStmtForMetaInfo,
                              (SMI_STATEMENT_UNTOUCHABLE |
                               SMI_STATEMENT_MEMORY_CURSOR) )
              != IDE_SUCCESS );
    sIsStmtBegun = ID_TRUE;


    IDE_TEST(sdm::getReplicaSetsByPName( &sSmiStmt,
                                         aPrimaryNodeName,
                                         sDataSMNForMetaInfo,
                                         aReplicaSetsInfo ) != IDE_SUCCESS );

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsStmtBegun = ID_FALSE;

    if ( aTrans == NULL )
    {
        IDE_TEST( sTrans.commit() != IDE_SUCCESS );
        sTransStage = 1;

        sTransStage = 0;
        IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_TRANS )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::getReplicaSet",
                                  "Invalid transaction" ) );
    }
    IDE_EXCEPTION_END;    

    if ( sIsStmtBegun == ID_TRUE )
    {
        (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
    }
    else
    {
        /* Nothing to do */
    }

    switch ( sTransStage )
    {
        case 2:
            ( void )sTrans.rollback();
            /* fall through */
        case 1:
            ( void )sTrans.destroy( NULL );
            /* fall through */
        default:
            break;
    }

    ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. errorcode 0x%05"ID_XINT32_FMT" %s\n",
                           E_ERROR_CODE(ideGetErrorCode()),
                           ideGetErrorMsg(ideGetErrorCode()));

    return IDE_FAILURE;
}

IDE_RC sdi::checkBackupReplicationRunning( UInt            aKSafety,
                                           sdiReplicaSet * aReplicaSet,
                                           UInt            aNodeCount )
{
    IDE_TEST_RAISE( aReplicaSet->mReplicaSetId == SDI_REPLICASET_NULL_ID, ERR_REPLICA_SET );

    switch ( aKSafety )
    {
        case 2:
            if ( aNodeCount != 2 )
            {
                IDE_TEST_RAISE( idlOS::strncmp( aReplicaSet->mSecondBackupNodeName,
                                                SDM_NA_STR,
                                                SDI_NODE_NAME_MAX_SIZE) == 0,
                                ERR_SECOND_BACKUP_REPLICATION );
            }            
            /* fall through */
        case 1:
            IDE_TEST_RAISE( idlOS::strncmp( aReplicaSet->mFirstBackupNodeName,
                                            SDM_NA_STR,
                                            SDI_NODE_NAME_MAX_SIZE) == 0,
                            ERR_FIRST_BACKUP_REPLICATION );
            break;
        case 0:
            /* do nothing */
            break;
        default:
            IDE_RAISE(ERR_INVALID_KSAFETY);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_KSAFETY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::checkBackupReplicationRunning",
                                  "Invalid ksafety" ) );
    }
    IDE_EXCEPTION( ERR_REPLICA_SET )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::checkBackupReplicationRunning",
                                  "Invalid replica set ID" ) );
    }
    IDE_EXCEPTION( ERR_FIRST_BACKUP_REPLICATION )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_NOT_RUNNING_REPLICATION,
                                  aReplicaSet->mFirstReplName) );
    }
    IDE_EXCEPTION( ERR_SECOND_BACKUP_REPLICATION )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_NOT_RUNNING_REPLICATION,
                                  aReplicaSet->mSecondReplName) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::shardExecTempDDLWithNewTrans(qcStatement * aStatement,
                                         SChar       * aNodeName,
                                         SChar       * aQuery,
                                         UInt          aQueryLen )
{
    SChar         sNodeNameStr[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
    SChar       * sSqlStr = NULL;
    void        * sMmSession = NULL;
    idBool        sIsAllocMmSession = ID_FALSE;
    const SChar * sRemoteSQLFmt;
    const SChar * sRemoteLockTimeoutSQLStr;
    SInt          sLen = 0;
    sdiLocalMetaInfo    sLocalMetaInfo;
    qcSession         * sQcSession;
    
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
            
    IDE_TEST( qci::mSessionCallback.mAllocInternalSession( &sMmSession, 
                                                           aStatement->session->mMmSession )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sMmSession == NULL, ERR_INVALID_CONDITION );
    sIsAllocMmSession = ID_TRUE;
    
    qci::mSessionCallback.mSetShardMetaNumber( sMmSession,
                                               sdi::getSMNForDataNode());    
    qci::mSessionCallback.mSetNewShardPIN( sMmSession );
    IDE_TEST(qci::mSessionCallback.mSetShardInternalLocalOperation( sMmSession,
                                                                    SDI_INTERNAL_OP_NORMAL )
             != IDE_SUCCESS);
    
    if ( isShardDDL( aStatement ) == ID_TRUE )
    {
        sQcSession = qci::mSessionCallback.mGetQcSession( sMmSession );
        
        sQcSession->mQPSpecific.mFlag &= ~QC_SESSION_SHARD_DDL_MASK;
        sQcSession->mQPSpecific.mFlag |= QC_SESSION_SHARD_DDL_TRUE;
    }

    //IDE_TEST( qcg::setSessionIsInternalLocalOperation( aStatement->session->mMmSession, 1) != IDE_SUCCESS);
    if ( aNodeName != NULL )
    {
        idlOS::strncpy(sNodeNameStr, aNodeName, ID_SIZEOF(sNodeNameStr));
    }
    else
    {
        idlOS::strncpy(sNodeNameStr, "NULL", ID_SIZEOF(sNodeNameStr));
    }

    // BUG-48616
    if ( isSameNode( sLocalMetaInfo.mNodeName, aNodeName ) == ID_TRUE )
    {
        sRemoteSQLFmt = "%s";
        sRemoteLockTimeoutSQLStr = "alter session set ddl_lock_timeout = %"ID_INT32_FMT"";

        sLen = IDL_MAX( (aQueryLen + idlOS::strlen(sRemoteSQLFmt) + 1 ), 
                        (idlOS::strlen(sRemoteLockTimeoutSQLStr) + 1 + 5 ) ) + 1;
    }
    else
    {
        sRemoteSQLFmt = "exec DBMS_SHARD.EXECUTE_IMMEDIATE('%s', '"QCM_SQL_STRING_SKIP_FMT"')";
        sRemoteLockTimeoutSQLStr = "exec DBMS_SHARD.EXECUTE_IMMEDIATE('alter session set ddl_lock_timeout = %"ID_INT32_FMT"', '"QCM_SQL_STRING_SKIP_FMT"')";

        sLen = IDL_MAX( (aQueryLen + idlOS::strlen(sRemoteSQLFmt) + 1 + ID_SIZEOF(sNodeNameStr)), 
                        (idlOS::strlen(sRemoteLockTimeoutSQLStr) + 1 + 5 + ID_SIZEOF(sNodeNameStr)) ) + 1;
    }
    
    IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_SDI,
                                sLen,
                                (void **)&sSqlStr,
                                IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS, InsufficientMemory );
    
    idlOS::snprintf(sSqlStr,
                    sLen,
                    sRemoteLockTimeoutSQLStr,
                    QCG_GET_SESSION_SHARD_DDL_LOCK_TIMEOUT(aStatement),
                    sNodeNameStr);
    
    IDE_TEST( qciMisc::executeTempSQL( sMmSession, sSqlStr, ID_TRUE ) != IDE_SUCCESS );
    ideLog::log(IDE_SD_17,"[SHARD_META] execute tempSQL success: %s", sSqlStr);

    idlOS::snprintf( sSqlStr,
                     sLen,
                     sRemoteSQLFmt,
                     aQuery,
                     sNodeNameStr );

    IDE_TEST( qciMisc::executeTempSQL( sMmSession, sSqlStr, ID_TRUE ) != IDE_SUCCESS );
    ideLog::log(IDE_SD_17,"[SHARD_META] execute tempSQL success: %s", sSqlStr);

    sIsAllocMmSession = ID_FALSE;
    qci::mSessionCallback.mFreeInternalSession( sMmSession,
                                                ID_TRUE /* aIsSuccess */ );

    (void)iduMemMgr::free( sSqlStr );
    sSqlStr = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_CONDITION)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sdi::shardExecTempDDLWithNewTrans",
                                "invliad condition"));
    }
    IDE_EXCEPTION( InsufficientMemory );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    } 
    IDE_EXCEPTION_END;

    ideLog::log(IDE_SD_17,"[SHARD_META] execute tempSQL failure: %s", sSqlStr);

    if ( sSqlStr != NULL )
    {
        (void)iduMemMgr::free( sSqlStr );
    }

    if ( sIsAllocMmSession == ID_TRUE )
    {
        qci::mSessionCallback.mFreeInternalSession( sMmSession,
                                                    ID_FALSE /* aIsSuccess */ );
    }
    return IDE_FAILURE;
}

IDE_RC sdi::shardExecTempDMLOrDCLWithNewTrans( qcStatement * aStatement,
                                               SChar       * aQuery )
{
    void         * sMmSession = NULL;
    idBool         sIsAllocMmSession = ID_FALSE;
    idBool         sIsSmiStmtEnd = ID_FALSE;
    smiStatement * spRootStmt    = QC_SMI_STMT(aStatement)->getTrans()->getStatement();
    UInt           sSmiStmtFlag  = QC_SMI_STMT(aStatement)->mFlag;
    qcSession    * sQcSession;
    
    IDE_TEST( qci::mSessionCallback.mAllocInternalSession( &sMmSession, 
                                                           aStatement->session->mMmSession )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sMmSession == NULL, ERR_INVALID_CONDITION );
    sIsAllocMmSession = ID_TRUE;

    qci::mSessionCallback.mSetShardMetaNumber( sMmSession,
                                               sdi::getSMNForDataNode());
    qci::mSessionCallback.mSetNewShardPIN( sMmSession );
    IDE_TEST(qci::mSessionCallback.mSetShardInternalLocalOperation( sMmSession, SDI_INTERNAL_OP_NORMAL )
             != IDE_SUCCESS);

    if ( isShardDDL( aStatement ) == ID_TRUE )
    {
        sQcSession = qci::mSessionCallback.mGetQcSession( sMmSession );
        
        sQcSession->mQPSpecific.mFlag &= ~QC_SESSION_SHARD_DDL_MASK;
        sQcSession->mQPSpecific.mFlag |= QC_SESSION_SHARD_DDL_TRUE;
    }
    
    IDE_TEST( qciMisc::executeTempSQL( sMmSession,
                                       aQuery,
                                       ID_TRUE )
              != IDE_SUCCESS );
    
    ideLog::log(IDE_SD_17,"[SHARD_META] execute tempSQL: %s", aQuery);
    
    sIsAllocMmSession = ID_FALSE;

    qci::mSessionCallback.mFreeInternalSession( sMmSession,
                                                ID_TRUE /* aIsSuccess */ );
    
    // commit된 다음 해당 정보를 보기 위해
    // statement end, begin 수행
    IDE_TEST( QC_SMI_STMT(aStatement)->end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS );
    sIsSmiStmtEnd = ID_TRUE;

    IDE_TEST( QC_SMI_STMT(aStatement)->begin( aStatement->mStatistics,
                                              spRootStmt,
                                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sIsSmiStmtEnd = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_CONDITION)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sdi::shardExecTempDMLWithNewTrans",
                                "invliad condition"));
    }
    IDE_EXCEPTION_END;

    if ( sIsAllocMmSession == ID_TRUE )
    {
        qci::mSessionCallback.mFreeInternalSession( sMmSession,
                                                    ID_FALSE /* aIsSuccess */ );
    }

    if( sIsSmiStmtEnd == ID_TRUE )
    {
        IDE_ASSERT( QC_SMI_STMT(aStatement)->begin( aStatement->mStatistics,
                                                    spRootStmt,
                                                    SMI_STATEMENT_NORMAL|SMI_STATEMENT_ALL_CURSOR ) 
                    == IDE_SUCCESS);
    }
    ideLog::log(IDE_SD_1,"[SHARD_META_ERROR] shardExecTempDMLOrDCLWithNewTrans. Failed to execute tempSQL: %s", aQuery);
    return IDE_FAILURE;
}

/*
 * OUT: aTableInfo, aTableSCN, aTableHandle, aShardObjInfo
 */
IDE_RC sdi::getTableInfoAllForDDL( qcStatement    * aStatement,
                                   UInt             aUserID,
                                   UChar          * aTableName,
                                   SInt             aTableNameSize,
                                   idBool           aIsRangeMerge,
                                   qcmTableInfo  ** aTableInfo,
                                   smSCN          * aTableSCN,
                                   void          ** aTableHandle,
                                   sdiObjectInfo ** aShardObjInfo )
{
    qcmTableInfo   * sTableInfo = NULL;
    smSCN            sTableSCN = SM_SCN_INIT;
    void           * sTableHandle = NULL;
    sdiObjectInfo  * sShardObjInfo = NULL;

    smiTrans          sSmiTrans;
    smiStatement    * sDummySmiStmt = NULL;
    smiStatement    * sSmiStmtOrg   = NULL;
    smiStatement      sSmiStmt;
    SInt              sState        = 0;
    UInt              sSmiStmtFlag  = 0;

    IDE_TEST( sSmiTrans.initialize() != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sSmiTrans.begin( &sDummySmiStmt,
                               aStatement->mStatistics )
              != IDE_SUCCESS );
    sState = 2;

    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_UNTOUCHABLE;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics, sDummySmiStmt, sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 3;

    /* swap smistatement */
    qcg::getSmiStmt( aStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( aStatement, &sSmiStmt );
    sState = 4;

    IDE_TEST( qcm::getTableInfo(aStatement,
                                aUserID,
                                aTableName,
                                aTableNameSize,
                                &(sTableInfo),
                                &(sTableSCN),
                                &(sTableHandle)) != IDE_SUCCESS );

    IDE_TEST( qcm::lockTableForDDLValidation(aStatement,
                                             sTableHandle, 
                                             sTableSCN)
              != IDE_SUCCESS );

    IDE_TEST( sdi::getTableInfo( aStatement,
                                 sTableInfo,
                                 &(sShardObjInfo),
                                 aIsRangeMerge)
              != IDE_SUCCESS );
    IDE_TEST_RAISE(sShardObjInfo == NULL, ERR_GET_SHARD_OBJ);

    /* restore */
    sState = 3;
    qcg::setSmiStmt( aStatement, sSmiStmtOrg );

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS )!= IDE_SUCCESS );
    sState = 2;

    sState = 1;
    IDE_TEST( sSmiTrans.commit() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sSmiTrans.destroy( aStatement->mStatistics ) != IDE_SUCCESS );

    *aTableInfo = sTableInfo;
    *aTableSCN = sTableSCN;
    *aTableHandle = sTableHandle;
    *aShardObjInfo = sShardObjInfo;

    ACP_UNUSED(aTableInfo);
    ACP_UNUSED(aTableSCN);
    ACP_UNUSED(aTableHandle);
    ACP_UNUSED(aShardObjInfo);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_GET_SHARD_OBJ )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_SHARD_OBJECT_NOT_EXIST, aTableName ) );
    }
    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 4:
            qcg::setSmiStmt( aStatement, sSmiStmtOrg );
        case 3:
            (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            (void)sSmiTrans.rollback();
        case 1:
            (void)sSmiTrans.destroy( aStatement->mStatistics );
        case 0:
        default:
            break;
    }
    return IDE_FAILURE;
}

IDE_RC sdi::getRangeValueStrFromPartition(SChar * aPartitionName, sdiObjectInfo * aShardObjectInfo, SChar * aOutValueBuf)
{
    SInt i = 0;
    sdiValue  sValueStr;
    idBool    sIsFound = ID_FALSE;

    for ( i = 0;i < aShardObjectInfo->mRangeInfo.mCount; i++ )
    {
        if ( idlOS::strncmp(aShardObjectInfo->mRangeInfo.mRanges[i].mPartitionName, aPartitionName, QC_MAX_OBJECT_NAME_LEN) == 0 )
        {
            IDE_TEST( getValueStr(aShardObjectInfo->mTableInfo.mKeyDataType, 
                                  &aShardObjectInfo->mRangeInfo.mRanges[i].mValue, 
                                  &sValueStr) != IDE_SUCCESS );
            sIsFound = ID_TRUE;
            break;
        }
    }
    IDE_TEST_RAISE(sIsFound != ID_TRUE, ERR_NOT_FOUND);
    idlOS::strncpy(aOutValueBuf, (SChar*)sValueStr.mCharMax.value, sValueStr.mCharMax.length);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::getRangeValueStrFromPartition",
                                  "Invalid partition " ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::getValueStr( UInt       aKeyDataType,
                         sdiValue * aValue,
                         sdiValue * aValueStr )
{
    switch(aKeyDataType)
    {
        case MTD_CHAR_ID:
        case MTD_VARCHAR_ID:
            idlOS::memcpy( (void*)aValueStr,
                        (void*)aValue,
                        ID_SIZEOF(sdiValue) );
            break;
        case MTD_SMALLINT_ID:
            aValueStr->mCharMax.length = idlOS::snprintf( (SChar*)aValueStr->mCharMax.value,
                ID_SIZEOF(aValueStr),
                "%"ID_INT32_FMT,
                (SShort)aValue->mSmallintMax );
            break;
        case MTD_INTEGER_ID:
            aValueStr->mCharMax.length = idlOS::snprintf( (SChar*)aValueStr->mCharMax.value,
                ID_SIZEOF(aValueStr),
                "%"ID_INT32_FMT,
                (SInt)aValue->mIntegerMax );
            break;
        case MTD_BIGINT_ID:
            aValueStr->mCharMax.length = idlOS::snprintf( (SChar*)aValueStr->mCharMax.value,
                ID_SIZEOF(aValueStr),
                "%"ID_INT64_FMT,
                (SLong)aValue->mBigintMax );
            break;
        default :
                // 발생하지 않는다.
            IDE_DASSERT(0);
            IDE_RAISE(ERR_SHARD_REBUILD_ERROR);
            break;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_SHARD_REBUILD_ERROR )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SHARD_REBUILD_ERROR ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


IDE_RC sdi::validateSMNForShardDDL( qcStatement * aStatement )
{
    ULong             sDataSMN = SDI_NULL_SMN;
    sdiGlobalMetaInfo sGlobalMetaInfo = { ID_ULONG(0) };
    
    IDE_TEST( waitAndSetSMNForMetaNode( aStatement->mStatistics,
                                        QC_SMI_STMT( aStatement ),
                                        ( SMI_STATEMENT_NORMAL |
                                        SMI_STATEMENT_SELF_TRUE |
                                        SMI_STATEMENT_ALL_CURSOR ),
                                        SDI_NULL_SMN,
                                        &sDataSMN )
            != IDE_SUCCESS );
    
    // Meta SMN of another transaction
    IDE_TEST ( sdm::getGlobalMetaInfo ( &sGlobalMetaInfo ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sDataSMN != sGlobalMetaInfo.mShardMetaNumber,
                    ERR_SMN_IS_INVALID);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SMN_IS_INVALID )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::validateSMNForShardDDL",
                                  "ERR_SMN_IS_INVALID " ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::compareDataAndSessionSMN( qcStatement * aStatement )
{
    ULong sSessionSMN = ID_ULONG(0);
    ULong sDataSMN    = ID_ULONG(0);

    sDataSMN = getSMNForDataNode();
    
    if ( sdi::isShardCoordinator( aStatement ) == ID_TRUE )
    {
        // shardCoordinator using sessionSMN 
        sSessionSMN = QCG_GET_SESSION_SHARD_META_NUMBER( aStatement );

        IDE_TEST_RAISE( sDataSMN < sSessionSMN, ERR_INVALID_SMN );        
        IDE_TEST_RAISE( sDataSMN > sSessionSMN, REBUILD_RETRY );        
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_SMN  )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_SHARD_NOT_SAME_DATA_SESSION_SMN ));
    }
    IDE_EXCEPTION( REBUILD_RETRY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SHARD_META_OUT_OF_DATE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::checkReferenceObj( qcStatement * aStatement,
                               SChar       * aNodeName )
{
    IDE_TEST( sdm::checkReferenceObj( aStatement,
                                      aNodeName,
                                      getSMNForDataNode() )
              != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* if one node name is $$N/A then they are not equal node */
idBool sdi::isSameNode(SChar * aNodeName1, SChar * aNodeName2)
{
    idBool sResult = ID_FALSE;
    
    if ( (aNodeName1 != NULL) && (aNodeName2 != NULL) )
    {
        if ( ( SDI_IS_NULL_NAME(aNodeName1) != ID_TRUE ) && 
            ( SDI_IS_NULL_NAME(aNodeName2) != ID_TRUE ) &&
            idlOS::strncmp(aNodeName1, aNodeName2, SDI_NODE_NAME_MAX_SIZE) == 0 )
        {
            sResult = ID_TRUE;
        }
        else
        {
            sResult = ID_FALSE;
        }
    }
    else
    {
        sResult = ID_FALSE;
    }
    return sResult;
}



IDE_RC sdi::shardExecTempSQLWithoutSession( SChar       * aSqlStr,
                                            SChar       * aExecNodeName,
                                            UInt          aDDLLockTimeout,
                                            qciStmtType   aSQLStmtType )
{
    void              * sMmSession = NULL;
    idBool              sIsAllocMmSession = ID_FALSE;

    qciUserInfo         sUserInfo;

    QCD_HSTMT           sHstmt;

    qcStatement       * sStatement;

    sdiClientInfo     * sClientInfo  = NULL;

    UInt                sExecCount = 0;
    qciStmtType         sStmtType;

    smiStatement        sSmiStmt;
    smiStatement      * sDummySmiStmt;
    UInt                sSmiStmtFlag;
    UInt                sStage = 0;

    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];

    iduList           * sNodeList;
    idBool              sIsAllocList = ID_FALSE;
    idBool              sIsStmtAlloc = ID_FALSE;
    idBool              sIsTransBegin  = ID_FALSE;
    SChar             * sDummyPrepareSQL = (SChar*)"select 1 from dual";

    smiTrans          * sTrans = NULL;
    vSLong              sRowCnt = 0;
    sdiLocalMetaInfo    sLocalMetaInfo;
    
    idlOS::memcpy( &sUserInfo,
                   &sdiZookeeper::mUserInfo,
                   ID_SIZEOF( qciUserInfo ) );

    IDE_TEST( qci::mSessionCallback.mAllocInternalSessionWithUserInfo( &sMmSession,
                                                                       (void*)&sUserInfo )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sMmSession == NULL, ERR_INVALID_CONDITION );
    sIsAllocMmSession = ID_TRUE;

    qci::mSessionCallback.mSetNewShardPIN( sMmSession );

    qci::mSessionCallback.mSetShardMetaNumber( sMmSession,
                                               sdi::getSMNForDataNode());

    ideLog::log(IDE_SD_17,"shardExecTempDMLOrDCLWithOutSession: %s", aSqlStr);

    sTrans     = qci::mSessionCallback.mGetTransWithBegin( sMmSession );
    IDE_TEST_RAISE( sTrans == NULL, ERR_INVALID_CONDITION );
    sIsTransBegin = ID_TRUE;

    IDE_TEST( qcd::allocStmtNoParent( sMmSession,
                                      ID_TRUE,  // dedicated mode
                                      & sHstmt )
              != IDE_SUCCESS );
    sIsStmtAlloc = ID_TRUE;

    IDE_TEST( qcd::getQcStmt( sHstmt,
                              &sStatement )
              != IDE_SUCCESS );

    sStatement->session->mQPSpecific.mFlag &= ~QC_SESSION_TEMP_SQL_MASK;
    sStatement->session->mQPSpecific.mFlag |= QC_SESSION_TEMP_SQL_TRUE;

    IDE_TEST( qcd::prepare( sHstmt,
                            NULL,
                            NULL,
                            & sStmtType,
                            sDummyPrepareSQL,
                            ID_SIZEOF(sDummyPrepareSQL),
                            ID_TRUE )  // direct-execute mode
              != IDE_SUCCESS );

    if ( QCG_GET_SESSION_TRANSACTIONAL_DDL( sStatement) != ID_TRUE )
    {
        sStatement->session->mBakSessionProperty.mTransactionalDDL = 0;

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "ALTER SESSION SET TRANSACTIONAL_DDL = 1 " );

        IDE_TEST( qciMisc::runDCLforInternal( sStatement,
                                              sSqlStr,
                                              sStatement->session->mMmSession )
                  != IDE_SUCCESS );
    }
    if ( QCG_GET_SESSION_GTX_LEVEL( sStatement ) < 2 )
    {
        sStatement->session->mBakSessionProperty.mGlobalTransactionLevel = QCG_GET_SESSION_GTX_LEVEL( sStatement );

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "ALTER SESSION SET  GLOBAL_TRANSACTION_LEVEL = 2 " );

        IDE_TEST( qciMisc::runDCLforInternal( sStatement,
                                              sSqlStr,
                                              sStatement->session->mMmSession )
                  != IDE_SUCCESS );
    }

    if ( aDDLLockTimeout != 0 )
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "ALTER SESSION SET DDL_LOCK_TIMEOUT = %"ID_INT32_FMT" ", aDDLLockTimeout);

        IDE_TEST( qciMisc::runDCLforInternal( sStatement,
                                              sSqlStr,
                                              sStatement->session->mMmSession )
                  != IDE_SUCCESS );
    }

    sDummySmiStmt = sTrans->getStatement();

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    IDE_TEST( sSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sStage = 1;

    QC_SMI_STMT(sStatement) = &sSmiStmt;

    IDE_TEST( sdiZookeeper::getAliveNodeNameList( &sNodeList ) != IDE_SUCCESS );
    sIsAllocList = ID_TRUE;

    IDE_TEST( sdi::checkShardLinkerWithNodeList ( sStatement,
                                                  QCG_GET_SESSION_SHARD_META_NUMBER( sStatement ),
                                                  sTrans, 
                                                  sNodeList ) != IDE_SUCCESS );
    
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
    
    sClientInfo = sStatement->session->mQPSpecific.mClientInfo;

    // 전체 Node 대상으로 Meta Update 실행
    if ( sClientInfo != NULL )
    {   
        if (( idlOS::strMatch( sLocalMetaInfo.mNodeName,
                               idlOS::strlen( sLocalMetaInfo.mNodeName ),
                               aExecNodeName,
                               idlOS::strlen( aExecNodeName )) == 0 ) &&
            ( aSQLStmtType != QCI_STMT_MASK_MAX ))
        {
            // BUG-48616
            switch ( qciMisc::getStmtType( aSQLStmtType ))
            {
                case QCI_STMT_MASK_DDL:
                    IDE_TEST( qci::mSessionCallback.mSetShardInternalLocalOperation(
                                  sStatement->session->mMmSession,
                                  SDI_INTERNAL_OP_NORMAL ) 
                              != IDE_SUCCESS );

                    IDE_TEST( qciMisc::runDDLforInternalWithMmSession(
                                  sStatement->mStatistics,
                                  sStatement->session->mMmSession,
                                  QC_SMI_STMT( sStatement ),
                                  QC_EMPTY_USER_ID,
                                  QCI_SESSION_INTERNAL_DDL_TRUE,
                                  aSqlStr )
                              != IDE_SUCCESS );
                    break;
                case QCI_STMT_MASK_SP:
                case QCI_STMT_MASK_DML:
                    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( sStatement ),
                                                 aSqlStr,
                                                 &sRowCnt ) != IDE_SUCCESS);
                    break;
                case QCI_STMT_MASK_DCL:
                    IDE_TEST( qciMisc::runDCLforInternal( sStatement,
                                                          aSqlStr,
                                                          sStatement->session->mMmSession )
                              != IDE_SUCCESS );
                    break;
                default:
                    IDE_DASSERT(0);
                    break;
            }
        }
        else
        {

            IDE_TEST( sdi::shardExecDirectNested( sStatement,
                                                  aExecNodeName,
                                                  (SChar*)aSqlStr,
                                                  (UInt) idlOS::strlen( aSqlStr ),
                                                  SDI_INTERNAL_OP_NORMAL,
                                                  &sExecCount)
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sExecCount != 1, ERR_REMOTE_EXECUTION );    
        }
    }

    sStage = 0;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sIsStmtAlloc = ID_FALSE;
    IDE_TEST( qcd::freeStmt( sHstmt,
                             ID_TRUE )  // free & drop
              != IDE_SUCCESS );

    IDE_TEST_RAISE( qci::mSessionCallback.mCommit( sMmSession, ID_FALSE )
                    != IDE_SUCCESS, COMMIT_FAIL );
    sIsTransBegin = ID_FALSE;

    sIsAllocMmSession = ID_FALSE;
    qci::mSessionCallback.mFreeInternalSession( sMmSession,
                                                ID_TRUE /* aIsSuccess */ );

    sIsAllocList = ID_FALSE;
    sdiZookeeper::freeList( sNodeList, SDI_ZKS_LIST_NODENAME );

    return IDE_SUCCESS;

    IDE_EXCEPTION(COMMIT_FAIL)
    {
        ideLog::log( IDE_QP_0, "[Temporary SQL: COMMIT FAILURE] ERR-%05X : %s",
                     E_ERROR_CODE(ideGetErrorCode()),
                     ideGetErrorMsg(ideGetErrorCode()));
    }
    IDE_EXCEPTION( ERR_REMOTE_EXECUTION );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_EXECUTE_REMOTE_SQL_FAILED, aSqlStr ) );
    }
    IDE_EXCEPTION(ERR_INVALID_CONDITION)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sdi::shardExecTempSQLWithoutSession",
                                "invliad condition"));
    }
    IDE_EXCEPTION_END;

    if ( sStage > 0 )
    {
        switch ( sStage )
        {
            case 1:
                ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
            default:
                break;
        }
    }

    if ( sIsStmtAlloc != ID_FALSE )
    {
        (void)qcd::freeStmt( sHstmt, ID_TRUE );
    }

    if ( sIsTransBegin == ID_TRUE )
    {
        (void)qci::mSessionCallback.mRollback( sMmSession, NULL, ID_FALSE );
    }

    if ( sIsAllocMmSession == ID_TRUE )
    {
        qci::mSessionCallback.mFreeInternalSession( sMmSession,
                                                    ID_FALSE /* aIsSuccess */ );
    }

    if ( sIsAllocList == ID_TRUE )
    {
        sdiZookeeper::freeList( sNodeList, SDI_ZKS_LIST_NODENAME );
    }

    return IDE_FAILURE;
}

/* TASK-7219 Shard Transformer Refactoring */
IDE_RC sdi::allocAndInitQuerySetList( qcStatement * aStatement )
{
    IDE_TEST( sdo::allocAndInitQuerySetList( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::makeAndSetQuerySetList( qcStatement * aStatement,
                                    qmsQuerySet * aQuerySet )
{
    IDE_TEST( sdo::makeAndSetQuerySetList( aStatement,
                                           aQuerySet )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::setQuerySetListState( qcStatement  * aStatement,
                                  qmsParseTree * aParseTree,
                                  idBool       * aIsChanged )
{
    IDE_TEST( sdo::setQuerySetListState( aStatement,
                                         aParseTree,
                                         aIsChanged )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::unsetQuerySetListState( qcStatement * aStatement,
                                    idBool        aIsChanged )
{
    IDE_TEST( sdo::unsetQuerySetListState( aStatement,
                                           aIsChanged )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::setStatementFlagForShard( qcStatement * aStatement,
                                      UInt          aFlag )
{
    IDE_TEST( sdo::setStatementFlagForShard( aStatement,
                                             aFlag )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::isTransformNeeded( qcStatement * aStatement,
                               idBool      * aIsTransformNeeded )
{
    IDE_TEST( sdo::isTransformNeeded( aStatement,
                                      aIsTransformNeeded )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::isRebuildTransformNeeded( qcStatement * aStatement,
                                      idBool      * aIsTransformNeeded )
{
    qcShardStmtType sStmtType = QC_STMT_SHARD_NONE;

    sStmtType = aStatement->myPlan->parseTree->stmtShard;

    switch ( sStmtType )
    {
        case QC_STMT_SHARD_NONE:
        case QC_STMT_SHARD_ANALYZE:
            IDE_TEST( sdo::isTransformNeeded( aStatement,
                                              aIsTransformNeeded )
                      != IDE_SUCCESS );
            break;

        case QC_STMT_SHARD_DATA:
            IDE_RAISE( REBUILD_RETRY );
            break;

        case QC_STMT_SHARD_META:
            *aIsTransformNeeded = ID_FALSE;
            break;

        default:
            IDE_RAISE( ERR_INVALID_STMT_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( REBUILD_RETRY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SHARD_META_OUT_OF_DATE ) );
    }
    IDE_EXCEPTION( ERR_INVALID_STMT_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "sdi::isRebuildTransformNeeded",
                                  "stmt type is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::setPrintInfoFromTransformAble( qcStatement * aStatement )
{
    IDE_TEST( sdo::setPrintInfoFromTransformAble( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::raiseInvalidShardQueryError( qcStatement * aStatement,
                                         qcParseTree * aParseTree )
{
    IDE_TEST( sdo::raiseInvalidShardQueryError( aStatement,
                                                aParseTree )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::preAnalyzeQuerySet( qcStatement * aStatement,
                                qmsQuerySet * aQuerySet,
                                ULong         aSMN )
{
    IDE_TEST( sdo::preAnalyzeQuerySet( aStatement,
                                       aQuerySet,
                                       aSMN )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::preAnalyzeSubquery( qcStatement * aStatement,
                                qtcNode     * aNode,
                                ULong         aSMN )
{
    IDE_TEST( sdo::preAnalyzeSubquery( aStatement,
                                       aNode,
                                       aSMN )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::makeAndSetAnalysis( qcStatement * aSrcStatement,
                                qcStatement * aDstStatement,
                                qmsQuerySet * aDstQuerySet )
{
    IDE_TEST( sdo::makeAndSetAnalysis( aSrcStatement,
                                       aDstStatement,
                                       aDstQuerySet )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::allocAndCopyTableInfoList( qcStatement       * aStatement,
                                       sdiTableInfoList  * aSrcTableInfoList,
                                       sdiTableInfoList ** aDstTableInfoList )
{
    IDE_TEST( sdo::allocAndCopyTableInfoList( aStatement,
                                              aSrcTableInfoList,
                                              aDstTableInfoList )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::makeAndSetAnalyzeInfoFromStatement( qcStatement * aStatement )
{
    IDE_TEST( sdo::makeAndSetAnalyzeInfoFromStatement( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::makeAndSetAnalyzeInfoFromParseTree( qcStatement     * aStatement,
                                                qcParseTree     * aParseTree,
                                                sdiAnalyzeInfo ** aAnalyzeInfo )
{
    IDE_TEST( sdo::makeAndSetAnalyzeInfoFromParseTree( aStatement,
                                                       aParseTree,
                                                       aAnalyzeInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::makeAndSetAnalyzeInfoFromQuerySet( qcStatement     * aStatement,
                                               qmsQuerySet     * aQuerySet,
                                               sdiAnalyzeInfo ** aAnalyzeInfo )
{
    IDE_TEST( sdo::makeAndSetAnalyzeInfoFromQuerySet( aStatement,
                                                      aQuerySet,
                                                      aAnalyzeInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::makeAndSetAnalyzeInfoFromObjectInfo( qcStatement     * aStatement,
                                                 sdiObjectInfo   * aShardObjInfo,
                                                 sdiAnalyzeInfo ** aAnalyzeInfo )
{
    IDE_TEST( sdo::makeAndSetAnalyzeInfoFromObjectInfo( aStatement,
                                                        aShardObjInfo,
                                                        aAnalyzeInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::isShardParseTree( qcParseTree * aParseTree,
                              idBool      * aIsShardParseTree )
{
    IDE_TEST( sdo::isShardParseTree( aParseTree,
                                     aIsShardParseTree )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::isShardQuerySet( qmsQuerySet * aQuerySet,
                             idBool      * aIsShardQuerySet,
                             idBool      * aIsTransformAble )
{
    IDE_TEST( sdo::isShardQuerySet( aQuerySet,
                                    aIsShardQuerySet,
                                    aIsTransformAble )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::isShardObject( qcParseTree * aParseTree,
                           idBool      * aIsShardObject )
{
    IDE_TEST( sdo::isShardObject( aParseTree,
                                  aIsShardObject )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::isSupported( qmsQuerySet * aQuerySet,
                         idBool      * aIsSupported )
{
    IDE_TEST( sdo::isSupported( aQuerySet,
                                aIsSupported )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::setShardPlanStmtVariable( qcStatement    * aStatement,
                                      qcShardStmtType  aStmtType,
                                      qcNamePosition * aStmtPos )
{
    IDE_TEST( sdo::setShardPlanStmtVariable( aStatement,
                                             aStmtType,
                                             aStmtPos )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::setShardPlanCommVariable( qcStatement      * aStatement,
                                      sdiAnalyzeInfo   * aAnalyzeInfo,
                                      UShort             aParamCount,
                                      UShort             aParamOffset,
                                      qcShardParamInfo * aParamInfo )
{
    IDE_TEST( sdo::setShardPlanCommVariable( aStatement,
                                             aAnalyzeInfo,
                                             aParamCount,
                                             aParamOffset,
                                             aParamInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::setShardStmtType( qcStatement * aStatement,
                              qcStatement * aViewStatement )
{
    IDE_TEST( sdo::setShardStmtType( aStatement,
                                     aViewStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::propagateShardMetaNumber( qcSession * aSession )
{
    sdiClientInfo  * sClientInfo          = NULL;
    sdiConnectInfo * sConnectInfo         = NULL;
    ULong            sTargetSMN           = SDI_NULL_SMN;
    UShort           i                    = 0;
    idBool           sFail                = ID_FALSE;

    IDE_DASSERT( aSession != NULL );

    sClientInfo = aSession->mQPSpecific.mClientInfo;
    if ( sClientInfo != NULL )
    {
        sTargetSMN = sClientInfo->mTargetShardMetaNumber;

        sConnectInfo = sClientInfo->mConnectInfo;
        for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
        {
            if ( ( sConnectInfo->mNodeInfo.mSMN == sTargetSMN ) &&
                 ( sConnectInfo->mShardMetaNumber != sTargetSMN ) )
            {
                if ( sConnectInfo->mDbc != NULL )
                {
                    if ( sdl::setTargetShardMetaNumber( sClientInfo,
                                                        sConnectInfo )
                         == IDE_SUCCESS )
                    {
                        if ( sdl::setConnAttr( sConnectInfo,
                                               SDL_ALTIBASE_SHARD_META_NUMBER,
                                               sTargetSMN,
                                               NULL,
                                               & sConnectInfo->mLinkFailure )
                             == IDE_SUCCESS )
                        {
                            sConnectInfo->mShardMetaNumber = sTargetSMN;
                        }
                        else
                        {
                            sFail = ID_TRUE;
                        }
                    }
                    else
                    {
                        sFail = ID_TRUE;
                    }
                }
                else
                {
                    /* DBC == NULL, Success case */
                    sConnectInfo->mShardMetaNumber = sTargetSMN;
                }
            }
            else
            {
                /* sConnectInfo->mShardMetaNumber == sTargetSMN
                 * Nothing to do */
            }
        } /* FOR sConnectInfo++ */
    }
    else
    {
        /* sClientInfo == NULL
         * Nothing to do */
    }

    IDE_TEST( sFail == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    sConnectInfo = sClientInfo->mConnectInfo;
    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
    {
        if ( sConnectInfo->mShardMetaNumber != sTargetSMN )
        {
            if ( sConnectInfo->mDbc != NULL )
            {
                sConnectInfo->mLinkFailure = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC sdi::propagateRebuildShardMetaNumber( qcSession * aSession )
{
    sdiClientInfo  * sClientInfo          = NULL;
    sdiConnectInfo * sConnectInfo         = NULL;
    ULong            sTargetSMN           = SDI_NULL_SMN;
    ULong            sSentSMN             = SDI_NULL_SMN;
    UShort           i                    = 0;
    idBool           sFail                = ID_FALSE;

    IDE_DASSERT( aSession != NULL );

    sClientInfo = aSession->mQPSpecific.mClientInfo;
    if ( sClientInfo != NULL )
    {
        sTargetSMN = sClientInfo->mTargetShardMetaNumber;

        sConnectInfo = sClientInfo->mConnectInfo;
        for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
        {
            sSentSMN = PDL_MAX( sConnectInfo->mShardMetaNumber,
                                sConnectInfo->mRebuildShardMetaNumber );
            if ( ( sConnectInfo->mNodeInfo.mSMN == sTargetSMN ) &&
                 ( sSentSMN != sTargetSMN ) )
            {
                if ( sConnectInfo->mDbc != NULL )
                {
                    if ( sdl::setTargetShardMetaNumber( sClientInfo,
                                                        sConnectInfo )
                         == IDE_SUCCESS )
                    {
                        if ( sdl::setConnAttr( sConnectInfo,
                                               SDL_ALTIBASE_REBUILD_SHARD_META_NUMBER,
                                               sTargetSMN,
                                               NULL,
                                               & sConnectInfo->mLinkFailure )
                             == IDE_SUCCESS )
                        {
                            sConnectInfo->mRebuildShardMetaNumber = sTargetSMN;
                        }
                        else
                        {
                            sFail = ID_TRUE;
                        }
                    }
                    else
                    {
                        sFail = ID_TRUE;
                    }
                }
                else
                {
                    /* DBC == NULL, Success case */
                    sConnectInfo->mRebuildShardMetaNumber = sTargetSMN;
                }
            }
            else
            {
                /* sConnectInfo->mShardMetaNumber == sTargetSMN
                 * Nothing to do */
            }
        } /* FOR sConnectInfo++ */
    }
    else
    {
        /* sClientInfo == NULL
         * Nothing to do */
    }

    IDE_TEST( sFail == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    sConnectInfo = sClientInfo->mConnectInfo;
    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
    {
        sSentSMN = PDL_MAX( sConnectInfo->mShardMetaNumber,
                            sConnectInfo->mRebuildShardMetaNumber );
        if ( sSentSMN != sTargetSMN )
        {
            if ( sConnectInfo->mDbc != NULL )
            {
                sConnectInfo->mLinkFailure = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_FAILURE;
}

idBool sdi::isReshardOccurred( ULong aSessionSMN,
                               ULong aLastSessionSMN )
{
    /* Meta+Data 구조 (TC Shard1) 에서는 FALSE 를 반한해야 한다.
     *  SHARD_ENABLE=0 인 경우 Shard Meta 정보가 없기때문에 Resharding 할수 없다. */
    return ( ( SDU_SHARD_ENABLE == 1 ) &&
             ( aLastSessionSMN != aSessionSMN ) )
           ? ID_TRUE
           : ID_FALSE;
}

IDE_RC sdi::makeShardSession( qcSession             * aSession,
                              void                  * aDkiSession,
                              smiTrans              * aTrans,
                              idBool                  aIsShardMetaChanged,
                              ULong                   aSessionSMN,
                              ULong                   aLastSessionSMN,
                              sdiRebuildPropaOption   aRebuildPropaOpt,
                              idBool                  aIsPartialCoord )
{
    UInt sShardSessionType = SDI_SESSION_TYPE_USER;

    if ( aSession->mQPSpecific.mClientInfo != NULL )
    {
        IDE_TEST_RAISE( aSessionSMN < aLastSessionSMN,
                        ErrInvalidSMN );
    }

    sShardSessionType = qci::mSessionCallback.mGetShardSessionType( aSession->mMmSession );
        
    if ( isReshardOccurred( aSessionSMN, aLastSessionSMN ) == ID_TRUE )
    {
        /* TASK-7219 Non-shard DML
         * Partial coordinator에 의해 생성된 client info를 고려한다.
         * Partial coordinator는 non-shard DML statement의 execution 시(first init) 생성될 수 있고,
         * 이로 인해 이미 생성되어 있는 경우
         *     1. 그대로 사용하거나 ( isReshardOccured == false )
         *     2. 갱신한다. ( isReshardOccured == true )
         * 일반 상황(isReshardOccured == false) 에서는
         * isPartialCoordinator()에 의해 결정되어 전달된 aIsPartialCoord에 의해
         * initializeSession()의 수행여부가 결정된다.
         *
         */
        if ( ( sShardSessionType == SDI_SESSION_TYPE_USER ) ||
             ( aIsPartialCoord == ID_TRUE ) )
        {
            if ( aSession->mQPSpecific.mClientInfo == NULL )
            {
                IDE_TEST( initializeSession( aSession,
                                             aDkiSession,
                                             aTrans,
                                             aIsShardMetaChanged,
                                             aLastSessionSMN )
                          != IDE_SUCCESS );
            }

            if ( aSession->mQPSpecific.mClientInfo == NULL )
            {
                IDE_TEST( initializeSession( aSession,
                                             aDkiSession,
                                             aTrans,
                                             aIsShardMetaChanged,
                                             aSessionSMN )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( updateSession( aSession,
                                         aDkiSession,
                                         aTrans,
                                         aIsShardMetaChanged,
                                         aSessionSMN )
                          != IDE_SUCCESS );

                if ( aRebuildPropaOpt == SDI_REBUILD_SMN_PROPAGATE )
                {
                    IDE_TEST( propagateRebuildShardMetaNumber( aSession )
                              != IDE_SUCCESS );
                }

            }
        }
        else
        {
            if ( aSession->mQPSpecific.mClientInfo != NULL )
            {
                IDE_TEST( updateSession( aSession,
                                         aDkiSession,
                                         aTrans,
                                         aIsShardMetaChanged,
                                         aSessionSMN )
                          != IDE_SUCCESS );

                if ( aRebuildPropaOpt == SDI_REBUILD_SMN_PROPAGATE )
                {
                    IDE_TEST( propagateRebuildShardMetaNumber( aSession )
                              != IDE_SUCCESS );
                }
            }
        }
    }
    else
    {
        if ( ( sShardSessionType == SDI_SESSION_TYPE_USER ) ||
             ( aIsPartialCoord == ID_TRUE ) )
        {
            if ( aSession->mQPSpecific.mClientInfo == NULL )
            {
                IDE_TEST( initializeSession( aSession,
                                             aDkiSession,
                                             aTrans,
                                             ID_FALSE,
                                             aSessionSMN )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* Nothing to do. */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ErrInvalidSMN )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SESSION_SMN_REVERSED ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::checkTargetSMN( sdiClientInfo * aClientInfo, sdiConnectInfo * aConnectInfo )
{
    IDE_TEST_RAISE( ( ( aClientInfo->mTargetShardMetaNumber
                         != aConnectInfo->mRebuildShardMetaNumber ) &&
                      ( aClientInfo->mTargetShardMetaNumber
                         != aConnectInfo->mShardMetaNumber ) ),
                    ERR_SMN_MISMATCH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SMN_MISMATCH )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_FAILED_TO_PROPAGATE_SHARD_META_NUMBER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// TASK-7244 PSM partial rollback in Sharding
IDE_RC sdi::rollbackForPSM ( qcStatement   * aStatement,
                             sdiClientInfo * aClientInfo )
{
    sdiConnectInfo * sConnectInfo = NULL;
    UInt i;

    if ( aClientInfo != NULL )
    {
        sConnectInfo = aClientInfo->mConnectInfo;

        for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++ )
        {
            if ( (sConnectInfo->mFlag & SDI_CONNECT_PSM_SVP_SET_MASK) == SDI_CONNECT_PSM_SVP_SET_TRUE )
            {
                if ( sdi::rollback( sConnectInfo,
                                    SAVEPOINT_FOR_SHARD_GLOBAL_PROC_PARTIAL_ROLLBACK )
                     != IDE_SUCCESS )
                {
                    sdi::setTransactionBroken( QCG_GET_SESSION_IS_AUTOCOMMIT( aStatement ),
                                               (void*)QCG_GET_DATABASE_LINK_SESSION( aStatement ),
                                               QC_SMI_STMT(aStatement)->mTrans );
                }
            }
        }
    }

    return IDE_SUCCESS;
}

// TASK-7244 PSM partial rollback in Sharding
IDE_RC sdi::clearPsmSvp( sdiClientInfo * aClientInfo )
{
    UInt i;
    sdiConnectInfo * sConnectInfo = NULL;

    if ( aClientInfo != NULL )
    {
        sConnectInfo = aClientInfo->mConnectInfo;

        for ( i = 0; i < aClientInfo->mCount; i++, sConnectInfo++ )
        {
            sConnectInfo->mFlag &= ~SDI_CONNECT_PSM_SVP_SET_MASK;
            sConnectInfo->mFlag |= SDI_CONNECT_PSM_SVP_SET_FALSE;
        }
    }

    return IDE_SUCCESS;
}

IDE_RC sdi::checkShardObjectForDDL( qcStatement * aQcStmt,
                                    sdiDDLType    aDDLType,
                                    SChar       * aObjectName )
{
    qdTableParseTree  * sQdParseTree = NULL;
    qsProcParseTree   * sQsParseTree = NULL;
    qdIndexParseTree  * sIndexParseTree = NULL;
    qsDropParseTree   * sProcDropParseTree = NULL;
    qdDropParseTree   * sDropParseTree = NULL;
    qdDisjoinTableParseTree * sDisJoinParseTree = NULL;

    qcNamePosition    * sUserNamePos = NULL;
    qcNamePosition    * sObjectNamePos = NULL;

    switch( aDDLType )
    {
        case SDI_DDL_TYPE_TABLE:
            sQdParseTree = (qdTableParseTree *)aQcStmt->myPlan->parseTree;
            sUserNamePos = &(sQdParseTree->userName);
            sObjectNamePos = &(sQdParseTree->tableName);
            break;

        case SDI_DDL_TYPE_PROCEDURE:
            sQsParseTree = (qsProcParseTree *)aQcStmt->myPlan->parseTree;
            sUserNamePos = &(sQsParseTree->userNamePos);
            sObjectNamePos = &(sQsParseTree->procNamePos);
            break;

        case SDI_DDL_TYPE_INDEX:
            sIndexParseTree = (qdIndexParseTree *)aQcStmt->myPlan->parseTree;
            sUserNamePos = &(sIndexParseTree->userNameOfTable);
            sObjectNamePos = &(sIndexParseTree->tableName);
            break;

        case SDI_DDL_TYPE_PROCEDURE_DROP:
            sProcDropParseTree = (qsDropParseTree*)aQcStmt->myPlan->parseTree;
            sUserNamePos = &(sProcDropParseTree->userNamePos);
            sObjectNamePos = &(sProcDropParseTree->procNamePos);
            break;

        case SDI_DDL_TYPE_DROP:
            sDropParseTree = (qdDropParseTree*)aQcStmt->myPlan->parseTree;
            sUserNamePos = &(sDropParseTree->userName);
            sObjectNamePos = &(sDropParseTree->objectName);
            break;

        case SDI_DDL_TYPE_DISJOIN:
            sDisJoinParseTree = (qdDisjoinTableParseTree*)aQcStmt->myPlan->parseTree;
            sUserNamePos = &(sDisJoinParseTree->userName);
            sObjectNamePos = &(sDisJoinParseTree->tableName);
            break;

        case SDI_DDL_TYPE_INDEX_DROP:
            sDropParseTree = (qdDropParseTree*)aQcStmt->myPlan->parseTree;
            sUserNamePos = &(sDropParseTree->userName);
            sObjectNamePos = &(sDropParseTree->objectName);
            break;

        default:
            IDE_CONT( NORMAL_EXIT );
            break;
    }

    IDE_TEST( checkShardObjectForDDLInternal( aQcStmt,
                                              *sUserNamePos,
                                              *sObjectNamePos,
                                               aObjectName ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::checkShardObjectForDDLInternal( qcStatement    * aQcStmt,
                                            qcNamePosition   aUserNamePos,
                                            qcNamePosition   aObjectNamePos,
                                            SChar          * aObjectName )
{
    qcNamePosition    * sUserNamePos = NULL;
    qcNamePosition    * sObjectNamePos = NULL;
    qcNamePosition      sBakObjectNamePos;
    SChar               sUserName[QC_MAX_OBJECT_NAME_LEN + 1] = {0,};
    SChar               sObjectName[QC_MAX_OBJECT_NAME_LEN + 1] = {0,};
    UInt                sUserID = 0;
    sdiTableInfo        sObjectInfo;
    sdiGlobalMetaInfo   sMetaNodeInfo = { ID_ULONG(0) };
    ULong               sSMN = 0;
    SInt                sState = 0;
    smiTrans            sSmiTrans;
    smiStatement      * sDummySmiStmt = NULL;
    smiStatement      * sSmiStmtOrg = NULL;
    smiStatement        sSmiStmt;
    UInt                sSmiStmtFlag = 0;
    idBool              sIsObjectFound = ID_FALSE;

    sUserNamePos = &aUserNamePos;
    sObjectNamePos = &aObjectNamePos;

    if( ( SDU_SHARD_ENABLE != 1 ) ||
        ( sdi::checkMetaCreated() == ID_FALSE ) ||
        ( gShardUserID == ID_UINT_MAX ) ||  /* metaCreate 도중 호출 하는 경우 때문에 추가 */
        ( QCG_GET_SESSION_IS_SHARD_INTERNAL_LOCAL_OPERATION( aQcStmt ) == ID_TRUE ) )
    {
        IDE_CONT( NORMAL_EXIT );
    }

    /* set UserName */
    if ( (sUserNamePos->size) > 0 && (sUserNamePos->size <= QC_MAX_OBJECT_NAME_LEN ) )
    {
        QC_STR_COPY( sUserName, *sUserNamePos );
    }
    else
    {
        sUserID = QCG_GET_SESSION_USER_ID(aQcStmt);
        IDE_TEST(qciMisc::getUserName(aQcStmt, sUserID, sUserName) != IDE_SUCCESS);
    }

    if( idlOS::strMatch( sUserName, idlOS::strlen( sUserName ), "SYS_SHARD", 9 ) == 0 )
    {
        /* 해당 object가 SYS_SHARD user의 것일 경우 실패처리 해야한다. */
        IDE_RAISE( ERR_SHARD_OBJECT );
    }

    /* set ObjectName */
    if( aObjectName != NULL )
    {
        idlOS::strncpy( sObjectName, aObjectName, QC_MAX_OBJECT_NAME_LEN + 1 );
    }
    else
    {
        if ( (sObjectNamePos->size > 0) && (sObjectNamePos->size <= QC_MAX_OBJECT_NAME_LEN ) )
        {
            if ( idlOS::strncmp( sObjectNamePos->stmtText + sObjectNamePos->offset,
                                 SDI_BACKUP_TABLE_PREFIX,
                                 idlOS::strlen( SDI_BACKUP_TABLE_PREFIX ) ) == 0 )
            {
                /* _BAK_ 테이블처리 */
                sBakObjectNamePos = *sObjectNamePos;
                sBakObjectNamePos.offset += 5;
                sBakObjectNamePos.size -= 5;

                QC_STR_COPY( sObjectName, sBakObjectNamePos );
            }
            else
            {
                QC_STR_COPY( sObjectName, *sObjectNamePos );
            }
        }

    }

    IDE_TEST( sSmiTrans.initialize() != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sSmiTrans.begin( &sDummySmiStmt,
                               aQcStmt->mStatistics )
              != IDE_SUCCESS );
    sState = 2;

    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_UNTOUCHABLE;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    sSmiStmtFlag &= ~SMI_TRANS_GCTX_MASK;
    sSmiStmtFlag |= SMI_TRANS_GCTX_ON;

    IDE_TEST( sSmiStmt.begin( aQcStmt->mStatistics, sDummySmiStmt, sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 3;

    /* swap smistatement */
    qcg::getSmiStmt( aQcStmt, &sSmiStmtOrg );
    qcg::setSmiStmt( aQcStmt, &sSmiStmt );
    sState = 4;

    IDE_TEST( sdm::getGlobalMetaInfoCore( &sSmiStmt, &sMetaNodeInfo ) != IDE_SUCCESS );

    sSMN = sMetaNodeInfo.mShardMetaNumber;

    IDE_TEST( sdm::getTableInfo( &sSmiStmt,
                                 sUserName,
                                 sObjectName,
                                 sSMN,
                                 &sObjectInfo,
                                 &sIsObjectFound )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsObjectFound == ID_TRUE, ERR_SHARD_OBJECT );

    /* restore */
    sState = 3;
    qcg::setSmiStmt( aQcStmt, sSmiStmtOrg );

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS )!= IDE_SUCCESS );
    sState = 2;

    sState = 1;
    IDE_TEST( sSmiTrans.commit() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sSmiTrans.destroy( aQcStmt->mStatistics ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SHARD_OBJECT)
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_SHARD_OBJECT_CANNOT_EXEC_DDL ) );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 4:
            qcg::setSmiStmt( aQcStmt, sSmiStmtOrg );
        case 3:
            (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            (void)sSmiTrans.rollback();
        case 1:
            (void)sSmiTrans.destroy( aQcStmt->mStatistics );
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC sdi::checkShardReplication( qcStatement * aQcStmt )
{
    qriParseTree      * sParseTree = NULL;
    sdiReplicaSetInfo   sReplicaSetInfo;
    ULong               sSMN = 0;
    SChar               sReplName[QC_MAX_NAME_LEN + 1] = {0,};
    UInt                i = 0;

    /* 1. 현재 sharding 환경인지 체크한다. */
    if( ( SDU_SHARD_ENABLE != 1 ) ||
        ( sdi::checkMetaCreated() == ID_FALSE ) ||
        ( gShardUserID == ID_UINT_MAX ) ||  //metaCreate 도중 호출 하는 경우를 제외하기 위해 추가
        ( QCG_GET_SESSION_IS_SHARD_INTERNAL_LOCAL_OPERATION( aQcStmt ) == ID_TRUE ) )
    {
        IDE_CONT( NORMAL_EXIT );
    }

    /* 2. 구문에서 replication 이름을 가져온다. */
    sParseTree = (qriParseTree *)aQcStmt->myPlan->parseTree;
    QC_STR_COPY( sReplName, sParseTree->replName );

    /* 3. meta의 replicaSet을 가져온다. */
    sSMN = sdi::getSMNForDataNode();
    IDE_TEST( sdm::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( aQcStmt ),
                                                      &sReplicaSetInfo,
                                                      sSMN )
              != IDE_SUCCESS );

    /* 4. replicaSet을 순회하면서 해당하는 이름의 replication이 있는지 확인한다. */
    for( i = 0; i < sReplicaSetInfo.mCount; i++ )
    {
        if( idlOS::strMatch( sReplName,
                             idlOS::strlen( sReplName ),
                             sReplicaSetInfo.mReplicaSets[i].mFirstReplName,
                             idlOS::strlen( sReplicaSetInfo.mReplicaSets[i].mFirstReplName ) ) == 0 )
        {
            IDE_RAISE( ERR_SHARD_REPLICATION );
        }

        if( idlOS::strMatch( sReplName,
                             idlOS::strlen( sReplName ),
                             sReplicaSetInfo.mReplicaSets[i].mSecondReplName,
                             idlOS::strlen( sReplicaSetInfo.mReplicaSets[i].mSecondReplName ) ) == 0 )
        {
            IDE_RAISE( ERR_SHARD_REPLICATION );
        }

        /* 차후 failover project로 시스템에서 사용할 replication이 추가될 경우
         * 해당 replication도 체크해야 한다. */
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_REPLICATION )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_SHARD_REPL_CANNOT_EXEC_DCLDDL ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 해당함수는 qdsd::resetShardMeta와 내용이 유사 */
IDE_RC sdi::resetShardMetaWithDummyStmt()
{
    smiTrans       sTrans;
    smiStatement   sSmiStmt;
    smiStatement * sDummySmiStmt = NULL;
    SChar          sSqlStr[QD_MAX_SQL_LENGTH + 1];
    vSLong         sRowCnt       = 0;
    UInt           sStage        = 0;
    UInt           sSmiStmtFlag  = 0;

    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_NORMAL;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sTrans.begin( &sDummySmiStmt,
                            NULL,
                            ( SMI_ISOLATION_NO_PHANTOM     |
                              SMI_TRANSACTION_NORMAL       |
                              SMI_TRANSACTION_REPL_DEFAULT |
                              SMI_COMMIT_WRITE_NOWAIT ) )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              ( SMI_STATEMENT_NORMAL |
                                SMI_STATEMENT_MEMORY_CURSOR ) )
              != IDE_SUCCESS );
    sStage = 3;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.NODES_ " );

    IDE_TEST( qciMisc::runDMLforDDL( &sSmiStmt,
                                     sSqlStr,
                                     &sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.OBJECTS_ " );

    IDE_TEST( qciMisc::runDMLforDDL( &sSmiStmt,
                                     sSqlStr,
                                     &sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.RANGES_ " );

    IDE_TEST( qciMisc::runDMLforDDL( &sSmiStmt,
                                     sSqlStr,
                                     &sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.CLONES_ " );

    IDE_TEST( qciMisc::runDMLforDDL( &sSmiStmt,
                                     sSqlStr,
                                     &sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.SOLOS_ " );

    IDE_TEST( qciMisc::runDMLforDDL( &sSmiStmt,
                                     sSqlStr,
                                     &sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.REPLICA_SETS_ " );

    IDE_TEST( qciMisc::runDMLforDDL( &sSmiStmt,
                                     sSqlStr,
                                     &sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.FAILOVER_HISTORY_ " );

    IDE_TEST( qciMisc::runDMLforDDL( &sSmiStmt,
                                     sSqlStr,
                                     &sRowCnt )
              != IDE_SUCCESS );

    /* SMN은 일단 건드리지 않는다. */
    //sdi::setSMNCacheForMetaNode( ID_ULONG(1) );
    //sdi::setSMNForDataNode( ID_ULONG(1) );

    // reset shard sequence
    IDE_TEST( qdsd::resetShardSequence( &sSmiStmt,
                                        (SChar*)"NEXT_NODE_ID" )
              != IDE_SUCCESS );

    IDE_TEST( qdsd::resetShardSequence( &sSmiStmt,
                                        (SChar*)"NEXT_SHARD_ID" )
              != IDE_SUCCESS );

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sTrans.commit() != IDE_SUCCESS );
    sStage = 1;

    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 3:
            ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            ( void )sTrans.rollback();
        case 1:
            ( void )sTrans.destroy( NULL );
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC sdi::closeSessionForShardDrop( qcStatement * aQcStmt )
{
    smiTrans        sTrans;
    smiStatement    sSmiStmt;
    smiStatement  * sDummySmiStmt = NULL;
    sdiDatabaseInfo sShardDBInfo;
    SChar           sSqlStr[QD_MAX_SQL_LENGTH + 1];
    UInt            sStage        = 0;
    UInt            sSmiStmtFlag  = 0;

    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_NORMAL;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    sShardDBInfo = sdi::getShardDBInfo();

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sTrans.begin( &sDummySmiStmt,
                            NULL,
                            ( SMI_ISOLATION_NO_PHANTOM     |
                              SMI_TRANSACTION_NORMAL       |
                              SMI_TRANSACTION_REPL_DEFAULT |
                              SMI_COMMIT_WRITE_NOWAIT ) )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sSmiStmt.begin( aQcStmt->mStatistics,
                              QC_SMI_STMT( aQcStmt ),
                              ( SMI_STATEMENT_NORMAL |
                                SMI_STATEMENT_ALL_CURSOR ) )
              != IDE_SUCCESS );
    sStage = 3;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "ALTER DATABASE %s SESSION CLOSE ALL", sShardDBInfo.mDBName );

    /* 별도의 tx를 열어 처리하므로 기존 statement의 mMmSession 정보를 새 statment에 연결해야 한다. */
    IDE_TEST( qciMisc::runDDLforInternalWithMmSession( sTrans.getStatistics(),
                                                       aQcStmt->session->mMmSession,
                                                       &sSmiStmt,
                                                       QC_EMPTY_USER_ID,
                                                       QCI_SESSION_INTERNAL_DDL_TRUE,
                                                       sSqlStr )
              != IDE_SUCCESS );

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sTrans.commit() != IDE_SUCCESS );
    sStage = 1;

    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 3:
            ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            ( void )sTrans.rollback();
        case 1:
            ( void )sTrans.destroy( NULL );
        default:
            break;
    }

    return IDE_FAILURE;
}

// TASK-7219 Non-shard DML
IDE_RC sdi::getParseTreeAnalysis( qcParseTree       * aParseTree,
                                  sdiShardAnalysis ** aAnalysis )
{
    IDE_TEST( sda::getParseTreeAnalysis( aParseTree,
                                         aAnalysis )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdi::findRequestNodeNGetResultWithoutSession( ID_XID * aXID, 
                                                     idBool * aFindRequestNode,
                                                     idBool * aIsCommit,
                                                     smSCN  * aGlobalCommitSCN )
{
    void           * sMmSession = NULL;
    idBool           sIsAllocMmSession = ID_FALSE;
    UShort           i = 0;
    qciUserInfo      sUserInfo;

    idBool           sIsRequireUpdateMaxNodeSCN = ID_FALSE;
    smSCN            sSCN;
    smSCN            sGlobalCommitSCN;
    idBool           sNeedUpdateSCN = ID_FALSE;
    SInt             sValueLen = 0;
    sdlRemoteStmt  * sRemoteStmt  = NULL;
    mtdIntegerType   sTransResult = MTD_INTEGER_NULL;
    mtdIntegerType   sIsRequestNode = MTD_INTEGER_NULL;
    QCD_HSTMT        sHstmt;

    qcStatement    * sStatement;

    SShort           sFetchResult = ID_SSHORT_MAX;
    sdiClientInfo  * sClientInfo  = NULL;

    qciStmtType      sStmtType;

    smiStatement     sSmiStmt;
    smiStatement   * sDummySmiStmt;
    UInt             sSmiStmtFlag;
    UInt             sStage = 0;

    sdiStatement   * sSdStmt = NULL;
    idBool           sIsStmtAlloc = ID_FALSE;
    idBool           sIsTransBegin  = ID_FALSE;
    SChar          * sDummyPrepareSQL = (SChar*)"select 1 from dual";

    const SChar    * sSql = "SELECT REQUEST_TRANSACTION, TRANSACTION_STATE, COMMIT_SCN FROM " 
                            "V$DBLINK_SHARD_TRANSACTION_INFO WHERE GLOBAL_XID LIKE '%s%%'";

    UChar            sGlobalXIDStr[ID_XID_DATA_MAX_LEN]; 
    SChar            sQuery[ ID_SIZEOF(sSql) + ID_SIZEOF(sGlobalXIDStr)+ 1] = {0, };

    sdiConnectInfo * sConnectInfo      = NULL;
    smiTrans       * sTrans = NULL;

    sdiDataNodes     sDataInfo;
    sdiDataNode    * sDataNode = sDataInfo.mNodes;

    (void)idaXaConvertGlobalXIDToString( NULL, aXID, sGlobalXIDStr, ID_XID_DATA_MAX_LEN );

    SMI_INIT_SCN( &sGlobalCommitSCN );

    idlOS::memcpy( &sUserInfo,
                   &sdiZookeeper::mUserInfo,
                   ID_SIZEOF( qciUserInfo ) );

    IDE_TEST( qci::mSessionCallback.mAllocInternalSessionWithUserInfo( &sMmSession,
                                                                       (void*)&sUserInfo )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sMmSession == NULL, ERR_INVALID_CONDITION );
    sIsAllocMmSession = ID_TRUE;

    qci::mSessionCallback.mSetNewShardPIN( sMmSession );

    qci::mSessionCallback.mSetShardMetaNumber( sMmSession,
                                               sdi::getSMNForDataNode());

    sTrans = qci::mSessionCallback.mGetTransWithBegin( sMmSession );
    IDE_TEST_RAISE( sTrans == NULL, ERR_INVALID_CONDITION );
    sIsTransBegin = ID_TRUE;

    IDE_TEST( qcd::allocStmtNoParent( sMmSession,
                                      ID_TRUE,  // dedicated mode
                                      & sHstmt )
              != IDE_SUCCESS );
    sIsStmtAlloc = ID_TRUE;

    IDE_TEST( qcd::getQcStmt( sHstmt,
                              &sStatement )
              != IDE_SUCCESS );

    IDE_TEST( qcd::prepare( sHstmt,
                            NULL,
                            NULL,
                            & sStmtType,
                            sDummyPrepareSQL,
                            ID_SIZEOF(sDummyPrepareSQL),
                            ID_TRUE )  // direct-execute mode
              != IDE_SUCCESS );

    sDummySmiStmt = sTrans->getStatement();

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    IDE_TEST( sSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sStage = 1;

    QC_SMI_STMT(sStatement) = &sSmiStmt;

    IDE_TEST( sdi::checkShardLinker( sStatement ) != IDE_SUCCESS );

    IDE_DASSERT( sStatement->session != NULL );

    sSdStmt = (sdiStatement *)qci::mSessionCallback.mGetShardStmt( QC_MM_STMT( sStatement ) );

    sClientInfo = sStatement->session->mQPSpecific.mClientInfo;
    sConnectInfo = sClientInfo->mConnectInfo;

    buildDataNodes( sClientInfo, &sDataInfo, NULL, NULL );

    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sDataNode->mState != SDI_NODE_STATE_EXECUTE_SELECTED )  /* PROJ-2733-DistTxInfo */
        {
            continue;
        }

        sNeedUpdateSCN = (sConnectInfo->mDbc == NULL) ? ID_TRUE : ID_FALSE;

        // open shard connection
        if ( qdkOpenShardConnection( sConnectInfo ) != IDE_SUCCESS )
        {
            sDataNode->mState = SDI_NODE_STATE_NONE;
            continue;
        }

        /* PROJ-2733-DistTxInfo Connection 이후 각 노드 SCN을 얻어 온다. */
        if ( sNeedUpdateSCN == ID_TRUE )
        {
            IDE_TEST( sdl::getSCN( sConnectInfo, &sSCN ) != IDE_SUCCESS );

            SM_SET_MAX_SCN( &(sClientInfo->mGCTxInfo.mCoordSCN), &sSCN );
        }

        // PROJ-2727
        IDE_TEST( setShardSessionProperty( sStatement->session,
                                           sClientInfo,
                                           sConnectInfo )
                  != IDE_SUCCESS );

    }
    /* PROJ-2733-DistTxInfo 분산정보를 계산한다. */
    calculateGCTxInfo( QC_PRIVATE_TMPLATE( sStatement ),
                       &sDataInfo,
                       ID_FALSE,
                       SDI_COORD_PLAN_INDEX_NONE );

    /* PROJ-2733-DistTxInfo Node에 분산정보를 전파한다. */
    IDE_TEST( propagateDistTxInfoToNodes( sStatement, sClientInfo, &sDataInfo ) != IDE_SUCCESS );

    sConnectInfo = sClientInfo->mConnectInfo;
    sDataNode = sDataInfo.mNodes;

    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sDataNode->mState != SDI_NODE_STATE_EXECUTE_SELECTED )  /* PROJ-2733-DistTxInfo */
        {
            continue;
        }

        
        IDE_TEST( sdlStatementManager::allocRemoteStatement( sSdStmt,
                                                             sConnectInfo->mNodeId,
                                                             &sRemoteStmt )
                  != IDE_SUCCESS );

        IDE_TEST(setInternalOp(SDI_INTERNAL_OP_NOT,
                               sConnectInfo,
                               &(sConnectInfo->mLinkFailure))
                 != IDE_SUCCESS);

        IDE_TEST( sdl::allocStmt( sConnectInfo,
                                  sRemoteStmt,
                                  SDI_SHARD_PARTIAL_EXEC_TYPE_NONE, /* sPartialCoordExecType */
                                  &(sConnectInfo->mLinkFailure) )
                  != IDE_SUCCESS );

        // add shard transaction
        IDE_TEST( qdkAddShardTransaction( sStatement->mStatistics,
                                          QC_SMI_STMT(sStatement)->mTrans->getTransID(),
                                          sClientInfo,
                                          sConnectInfo )
                  != IDE_SUCCESS );

        idlOS::snprintf( sQuery, ID_SIZEOF(sQuery), sSql, sGlobalXIDStr );

        IDE_TEST( sdl::bindCol( sConnectInfo,
                                sRemoteStmt,
                                1,
                                MTD_BIGINT_ID,
                                ID_SIZEOF(mtdBigintType),
                                (void *)&sIsRequestNode,
                                ID_SIZEOF(sIsRequestNode),
                                &sValueLen,
                                &(sConnectInfo->mLinkFailure) ) != IDE_SUCCESS );

        IDE_TEST( sdl::bindCol( sConnectInfo,
                                sRemoteStmt,
                                2,
                                MTD_BIGINT_ID,
                                ID_SIZEOF(mtdBigintType),
                                (void *)&sTransResult,
                                ID_SIZEOF(sTransResult),
                                &sValueLen,
                                &(sConnectInfo->mLinkFailure) ) != IDE_SUCCESS );

        IDE_TEST( sdl::bindCol( sConnectInfo,
                                sRemoteStmt,
                                3,
                                MTD_BIGINT_ID,
                                ID_SIZEOF(mtdBigintType),
                                (void *)&sGlobalCommitSCN,
                                ID_SIZEOF(sGlobalCommitSCN),
                                &sValueLen,
                                &(sConnectInfo->mLinkFailure) ) != IDE_SUCCESS );

        sIsRequireUpdateMaxNodeSCN = ID_TRUE;
        if ( sdl::execDirect( sConnectInfo,
                              sRemoteStmt,
                              sQuery,
                              idlOS::strlen(sQuery),
                              sClientInfo,
                              &(sConnectInfo->mLinkFailure) )
             == IDE_SUCCESS )
        {
            sDataNode->mState = SDI_NODE_STATE_EXECUTED;  /* PROJ-2733-DistTxInfo */
        }
        else
        {
            // 수행이 실패한 경우
            sDataNode->mState = SDI_NODE_STATE_EXECUTE_CANDIDATED;  /* PROJ-2733-DistTxInfo */
        }

        IDE_TEST( sdl::fetch( sConnectInfo,
                              sRemoteStmt,
                              &sFetchResult,
                              &(sConnectInfo->mLinkFailure) )
                    != IDE_SUCCESS );
        IDE_TEST_RAISE(sValueLen == SQL_NULL_DATA, ERR_GET_ROW_COUNT);

        if ( sdl::freeStmt( sConnectInfo,
                            sRemoteStmt,
                            SQL_DROP,
                            &(sConnectInfo->mLinkFailure) )
             != IDE_SUCCESS )
        {
            ideLog::log( IDE_SD_17,"freeStmt fail" );
        }

        if ( ( sConnectInfo->mLinkFailure != ID_TRUE ) &&
             ( sConnectInfo->mDbc != NULL ) )
        {
            IDE_TEST(setInternalOp( SDI_INTERNAL_OP_NOT,
                                    sConnectInfo,
                                    &(sConnectInfo->mLinkFailure))
                     != IDE_SUCCESS);
            ideLog::log(IDE_SD_17,"[SHARD_META] Internal Remote SQL: %s", sQuery);
        }

        sdlStatementManager::setUnused( &sRemoteStmt );

        if ( sFetchResult != SQL_NO_DATA_FOUND )
        {
            if ( sIsRequestNode == 1 )
            {
                *aFindRequestNode = ID_TRUE;
                if ( (smiDtxLogType)sTransResult == SMI_DTX_COMMIT )
                {
                    *aIsCommit = ID_TRUE;
                    *aGlobalCommitSCN = sGlobalCommitSCN;
                }
                else
                {
                    *aIsCommit = ID_FALSE;
                }

                break;
            }
        }
    }

    sIsRequireUpdateMaxNodeSCN = ID_FALSE;
    IDE_TEST( updateMaxNodeSCNToCoordSCN( sStatement, sClientInfo, &sDataInfo ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sIsStmtAlloc = ID_FALSE;
    IDE_TEST( qcd::freeStmt( sHstmt,
                             ID_TRUE )  // free & drop
              != IDE_SUCCESS );

    if ( qci::mSessionCallback.mCommit( sMmSession, ID_FALSE ) != IDE_SUCCESS )
    {
        ideLog::log( IDE_QP_0, "[Temporary SQL: COMMIT FAILURE] ERR-%05X : %s",
                     E_ERROR_CODE(ideGetErrorCode()),
                     ideGetErrorMsg(ideGetErrorCode()));
        
        (void)qci::mSessionCallback.mRollback( sMmSession, NULL, ID_FALSE );
    }
    sIsTransBegin = ID_FALSE;

    sIsAllocMmSession = ID_FALSE;
    qci::mSessionCallback.mFreeInternalSession( sMmSession,
                                                ID_TRUE /* aIsSuccess */ );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_CONDITION)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sdi::findRequestNodeNGetResultWithoutSession",
                                "invliad condition"));
    }
    IDE_EXCEPTION( ERR_GET_ROW_COUNT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdi::findRequestNodeNGetResultWithoutSession",
                                  "get row count fetch error" ) );
        ideLog::log(IDE_SD_31,"[DEBUG] Shard get row count: %"ID_INT32_FMT, sFetchResult);
    }

    IDE_EXCEPTION_END;

    if ( sIsRequireUpdateMaxNodeSCN == ID_TRUE )
    {
        (void) updateMaxNodeSCNToCoordSCN( sStatement, sClientInfo, &sDataInfo );
    }

    if ( sRemoteStmt != NULL )
    {
        if ( sRemoteStmt->mStmt != NULL )
        {
            (void)sdl::freeStmt( sConnectInfo,
                                 sRemoteStmt,
                                 SQL_DROP,
                                 &(sConnectInfo->mLinkFailure) );
        }
        sdlStatementManager::setUnused( &sRemoteStmt );
    }

    if ( sStage > 0 )
    {
        switch ( sStage )
        {
            case 1:
                ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
            default:
                break;
        }
    }


    if ( sIsStmtAlloc != ID_FALSE )
    {
        (void)qcd::freeStmt( sHstmt, ID_TRUE );
    }

    if ( sIsTransBegin == ID_TRUE )
    {
        (void)qci::mSessionCallback.mRollback( sMmSession, NULL, ID_FALSE );
    }

    if ( sIsAllocMmSession == ID_TRUE )
    {
        qci::mSessionCallback.mFreeInternalSession( sMmSession,
                                                    ID_FALSE /* aIsSuccess */ );
    }

    return IDE_FAILURE;

}

IDE_RC sdi::getAllBranchWithoutSession( void * aDtxInfo )
{
    UShort            i = 0;

    void            * sMmSession = NULL;
    idBool            sIsAllocMmSession = ID_FALSE;
    qciUserInfo       sUserInfo;

    idBool            sNeedUpdateSCN = ID_FALSE;
    smSCN             sSCN;

    qciStmtType       sStmtType;
    QCD_HSTMT         sHstmt;
    qcStatement     * sStatement    = NULL;
    smiTrans        * sTrans        = NULL;
    smiStatement    * sDummySmiStmt = NULL;
    smiStatement      sSmiStmt;
    UInt              sSmiStmtFlag;
    UInt              sStage = 0;

    idBool            sIsStmtAlloc     = ID_FALSE;
    idBool            sIsTransBegin    = ID_FALSE;
    SChar           * sDummyPrepareSQL = (SChar*)"select 1 from dual";

    sdiConnectInfo  * sConnectInfo = NULL;
    sdiClientInfo   * sClientInfo  = NULL;
    sdiDataNodes      sDataInfo;
    sdiDataNode     * sDataNode    = sDataInfo.mNodes;

    idlOS::memcpy( &sUserInfo,
                   &sdiZookeeper::mUserInfo,
                   ID_SIZEOF( qciUserInfo ) );

    IDE_TEST( qci::mSessionCallback.mAllocInternalSessionWithUserInfo( &sMmSession,
                                                                       (void*)&sUserInfo )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sMmSession == NULL, ERR_INVALID_CONDITION );
    sIsAllocMmSession = ID_TRUE;

    qci::mSessionCallback.mSetNewShardPIN( sMmSession );

    qci::mSessionCallback.mSetShardMetaNumber( sMmSession,
                                               sdi::getSMNForDataNode());

    sTrans = qci::mSessionCallback.mGetTransWithBegin( sMmSession );
    IDE_TEST_RAISE( sTrans == NULL, ERR_INVALID_CONDITION );
    sIsTransBegin = ID_TRUE;

    IDE_TEST( qcd::allocStmtNoParent( sMmSession,
                                      ID_TRUE,  // dedicated mode
                                      & sHstmt )
              != IDE_SUCCESS );
    sIsStmtAlloc = ID_TRUE;

    IDE_TEST( qcd::getQcStmt( sHstmt,
                              &sStatement )
              != IDE_SUCCESS );

    IDE_TEST( qcd::prepare( sHstmt,
                            NULL,
                            NULL,
                            & sStmtType,
                            sDummyPrepareSQL,
                            ID_SIZEOF(sDummyPrepareSQL),
                            ID_TRUE )  // direct-execute mode
              != IDE_SUCCESS );

    sDummySmiStmt = sTrans->getStatement();
    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    IDE_TEST( sSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sStage = 1;

    QC_SMI_STMT(sStatement) = &sSmiStmt;

    IDE_TEST( sdi::checkShardLinker ( sStatement ) != IDE_SUCCESS );

    sClientInfo = sStatement->session->mQPSpecific.mClientInfo;
    sConnectInfo = sClientInfo->mConnectInfo;

    buildDataNodes( sClientInfo, &sDataInfo, NULL, NULL );
    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        if ( sDataNode->mState != SDI_NODE_STATE_EXECUTE_SELECTED )  /* PROJ-2733-DistTxInfo */
        {
            continue;
        }

        sNeedUpdateSCN = (sConnectInfo->mDbc == NULL) ? ID_TRUE : ID_FALSE;

        // open shard connection
        if ( qdkOpenShardConnection( sConnectInfo ) != IDE_SUCCESS )
        {
            sDataNode->mState = SDI_NODE_STATE_NONE;
            continue;
        }

        /* PROJ-2733-DistTxInfo Connection 이후 각 노드 SCN을 얻어 온다. */
        if ( sNeedUpdateSCN == ID_TRUE )
        {
            IDE_TEST( sdl::getSCN( sConnectInfo, &sSCN ) != IDE_SUCCESS );

            SM_SET_MAX_SCN( &(sClientInfo->mGCTxInfo.mCoordSCN), &sSCN );
        }

        // PROJ-2727
        IDE_TEST( setShardSessionProperty( sStatement->session,
                                           sClientInfo,
                                           sConnectInfo )
                  != IDE_SUCCESS );

    }
    
    /* PROJ-2733-DistTxInfo 분산정보를 계산한다. */
    calculateGCTxInfo( QC_PRIVATE_TMPLATE( sStatement ),
                       &sDataInfo,
                       ID_FALSE,
                       SDI_COORD_PLAN_INDEX_NONE );

    /* PROJ-2733-DistTxInfo Node에 분산정보를 전파한다. */
    IDE_TEST( propagateDistTxInfoToNodes( sStatement, sClientInfo, &sDataInfo ) != IDE_SUCCESS );

    sConnectInfo = sClientInfo->mConnectInfo;
    sDataNode = sDataInfo.mNodes;

    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++, sDataNode++ )
    {
        IDE_TEST( qdkAddDtxBranchTx( aDtxInfo,
                                     (UChar)sConnectInfo->mCoordinatorType,
                                     sConnectInfo->mNodeName,
                                     sConnectInfo->mUserName,
                                     sConnectInfo->mUserPassword,
                                     sConnectInfo->mServerIP,
                                     sConnectInfo->mPortNo,
                                     sConnectInfo->mConnectType )
                  != IDE_SUCCESS );
    }

    sStage = 0;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sIsStmtAlloc = ID_FALSE;
    IDE_TEST( qcd::freeStmt( sHstmt,
                             ID_TRUE )  // free & drop
              != IDE_SUCCESS );

    /* just need node info */
    (void)qci::mSessionCallback.mRollback( sMmSession, NULL, ID_FALSE );
    sIsTransBegin = ID_FALSE;

    sIsAllocMmSession = ID_FALSE;
    qci::mSessionCallback.mFreeInternalSession( sMmSession,
                                                ID_TRUE /* aIsSuccess */ );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_CONDITION)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sdi::shardExecTempSQLWithoutSession",
                                "invliad condition"));
    }
    IDE_EXCEPTION_END;

    if ( sStage > 0 )
    {
        switch ( sStage )
        {
            case 1:
                ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
            default:
                break;
        }
    }

    if ( sIsStmtAlloc != ID_FALSE )
    {
        (void)qcd::freeStmt( sHstmt, ID_TRUE );
    }

    if ( sIsTransBegin == ID_TRUE )
    {
        (void)qci::mSessionCallback.mRollback( sMmSession, NULL, ID_FALSE );
    }

    if ( sIsAllocMmSession == ID_TRUE )
    {
        qci::mSessionCallback.mFreeInternalSession( sMmSession,
                                                    ID_FALSE /* aIsSuccess */ );
    }

    return IDE_FAILURE;
}

IDE_RC sdi::systemPropertyForShard( qcStatement * aStatement,
                                    SChar       * aNodeName,
                                    SChar       * aSqlStr )
{
    void              * sMmSession = NULL;
    idBool              sIsAllocMmSession = ID_FALSE;

    qciUserInfo         sUserInfo;

    QCD_HSTMT           sHstmt;

    qcStatement       * sInternalStatement;

    sdiClientInfo     * sClientInfo  = NULL;

    UInt                sExecCount = 0;
    qciStmtType         sStmtType;

    smiStatement        sSmiStmt;
    smiStatement      * sDummySmiStmt;
    UInt                sSmiStmtFlag;
    UInt                sStage = 0;

    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];

    iduList           * sNodeList;
    idBool              sIsAllocList = ID_FALSE;
    idBool              sIsStmtAlloc = ID_FALSE;
    idBool              sIsTransBegin  = ID_FALSE;
    SChar             * sDummyPrepareSQL = (SChar*)"select 1 from dual";

    smiTrans          * sTrans = NULL;
    sdiLocalMetaInfo    sLocalMetaInfo;

    if( SDU_SHARD_ZOOKEEPER_TEST == 1 )
    {            
        idlOS::memcpy( &sUserInfo,
                       &sdiZookeeper::mUserInfo,
                       ID_SIZEOF( qciUserInfo ) );
    }
    else
    {
        qci::mSessionCallback.mGetUserInfo( aStatement->session->mMmSession,
                                            &sUserInfo );
    }
        
    IDE_TEST( qci::mSessionCallback.mAllocInternalSessionWithUserInfo( &sMmSession,
                                                                       (void*)&sUserInfo )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sMmSession == NULL, ERR_INVALID_CONDITION );
    sIsAllocMmSession = ID_TRUE;

    qci::mSessionCallback.mSetNewShardPIN( sMmSession );

    qci::mSessionCallback.mSetShardMetaNumber( sMmSession,
                                               sdi::getSMNForDataNode());

    ideLog::log(IDE_SD_17,"[SHARD SYSTEM PROPERTY] : %s", aSqlStr);

    sTrans     = qci::mSessionCallback.mGetTransWithBegin( sMmSession );
    IDE_TEST_RAISE( sTrans == NULL, ERR_INVALID_CONDITION );
    sIsTransBegin = ID_TRUE;

    IDE_TEST( qcd::allocStmtNoParent( sMmSession,
                                      ID_TRUE,  // dedicated mode
                                      & sHstmt )
              != IDE_SUCCESS );
    sIsStmtAlloc = ID_TRUE;

    IDE_TEST( qcd::getQcStmt( sHstmt,
                              &sInternalStatement )
              != IDE_SUCCESS );

    sInternalStatement->session->mQPSpecific.mFlag &= ~QC_SESSION_TEMP_SQL_MASK;
    sInternalStatement->session->mQPSpecific.mFlag |= QC_SESSION_TEMP_SQL_TRUE;

    IDE_TEST( qcd::prepare( sHstmt,
                            NULL,
                            NULL,
                            & sStmtType,
                            sDummyPrepareSQL,
                            ID_SIZEOF(sDummyPrepareSQL),
                            ID_TRUE )  // direct-execute mode
              != IDE_SUCCESS );

    sDummySmiStmt = sTrans->getStatement();

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    IDE_TEST( sSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sStage = 1;

    QC_SMI_STMT(sInternalStatement) = &sSmiStmt;
    
    if( SDU_SHARD_ZOOKEEPER_TEST == 1 )
    {
        IDE_TEST( sdiZookeeper::getAliveNodeNameList( &sNodeList ) != IDE_SUCCESS );
        sIsAllocList = ID_TRUE;
    }
    
    IDE_TEST( sdi::checkShardLinkerWithNodeList ( sInternalStatement,
                                                  QCG_GET_SESSION_SHARD_META_NUMBER( sInternalStatement ),
                                                  sTrans, 
                                                  sNodeList ) != IDE_SUCCESS );
    
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    if ( QCG_GET_SESSION_GTX_LEVEL( sInternalStatement ) < 1 )
    {
        sInternalStatement->session->mBakSessionProperty.mGlobalTransactionLevel = QCG_GET_SESSION_GTX_LEVEL( sInternalStatement );

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "ALTER SESSION SET  GLOBAL_TRANSACTION_LEVEL = 1 " );

        IDE_TEST( qciMisc::runDCLforInternal( sInternalStatement,
                                              sSqlStr,
                                              sInternalStatement->session->mMmSession )
                  != IDE_SUCCESS );
    }

    sClientInfo = sInternalStatement->session->mQPSpecific.mClientInfo;

    // 전체 Node 대상으로 Meta Update 실행
    if ( sClientInfo != NULL )
    {   
        if ( idlOS::strMatch( sLocalMetaInfo.mNodeName,
                              idlOS::strlen( sLocalMetaInfo.mNodeName ),
                              aNodeName,
                              idlOS::strlen( aNodeName )) == 0 )
        {
            IDE_TEST( qcg::setSessionIsInternalLocalOperation( sInternalStatement,
                                                               SDI_INTERNAL_OP_NORMAL)
                      != IDE_SUCCESS);
            
            IDE_TEST( qciMisc::runDCLforInternal( sInternalStatement,
                                                  aSqlStr,
                                                  sInternalStatement->session->mMmSession )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( sdi::shardExecDirectNested( sInternalStatement,
                                                  aNodeName,
                                                  (SChar*)aSqlStr,
                                                  (UInt) idlOS::strlen( aSqlStr ),
                                                  SDI_INTERNAL_OP_NORMAL,
                                                  &sExecCount)
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sExecCount != 1, ERR_REMOTE_EXECUTION );    
        }
    }

    sStage = 0;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sIsStmtAlloc = ID_FALSE;
    IDE_TEST( qcd::freeStmt( sHstmt,
                             ID_TRUE )  // free & drop
              != IDE_SUCCESS );

    IDE_TEST_RAISE( qci::mSessionCallback.mCommit( sMmSession, ID_FALSE )
                    != IDE_SUCCESS, COMMIT_FAIL );
    sIsTransBegin = ID_FALSE;

    sIsAllocMmSession = ID_FALSE;
    qci::mSessionCallback.mFreeInternalSession( sMmSession,
                                                ID_TRUE /* aIsSuccess */ );

    if( SDU_SHARD_ZOOKEEPER_TEST == 1 )
    {            
        sIsAllocList = ID_FALSE;
        sdiZookeeper::freeList( sNodeList, SDI_ZKS_LIST_NODENAME );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(COMMIT_FAIL)
    {
        ideLog::log( IDE_QP_0, "[SHARD SYSTEM PROPERTY: COMMIT FAILURE] ERR-%05X : %s",
                     E_ERROR_CODE(ideGetErrorCode()),
                     ideGetErrorMsg(ideGetErrorCode()));
    }
    IDE_EXCEPTION( ERR_REMOTE_EXECUTION );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_EXECUTE_REMOTE_SQL_FAILED, aSqlStr ) );
    }
    IDE_EXCEPTION(ERR_INVALID_CONDITION)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sdi::systemPropertyForShard",
                                "invliad condition"));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    if ( sStage > 0 )
    {
        switch ( sStage )
        {
            case 1:
                ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
            default:
                break;
        }
    }

    if ( sIsStmtAlloc != ID_FALSE )
    {
        (void)qcd::freeStmt( sHstmt, ID_TRUE );
    }

    if ( sIsTransBegin == ID_TRUE )
    {
        (void)qci::mSessionCallback.mRollback( sMmSession, NULL, ID_FALSE );
    }

    if ( sIsAllocMmSession == ID_TRUE )
    {
        qci::mSessionCallback.mFreeInternalSession( sMmSession,
                                                    ID_FALSE /* aIsSuccess */ );
    }
    
    if( SDU_SHARD_ZOOKEEPER_TEST == 1 )
    {    
        if ( sIsAllocList == ID_TRUE )
        {
            sdiZookeeper::freeList( sNodeList, SDI_ZKS_LIST_NODENAME );
        }
    }
    
    IDE_POP();
    
    return IDE_FAILURE;
}

IDE_RC sdi::checkFailoverHistoryOverSMN( ULong         aSMN,
                                         idBool      * aIsExist )
{
    smiTrans       sTrans;
    smiStatement   sSmiStmt;
    smiStatement * sDummySmiStmt;
    UInt           sStage = 0;

    /* PROJ-2446 ONE SOURCE MM 에서 statistics정보를 넘겨 받아야 한다.
     * 추후 작업 */
    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sTrans.begin( &sDummySmiStmt, NULL )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              (SMI_STATEMENT_UNTOUCHABLE |
                               SMI_STATEMENT_MEMORY_CURSOR) )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( sdm::checkFailoverHistoryOverSMN( &sSmiStmt,
                                                aSMN,
                                                aIsExist ) != IDE_SUCCESS );

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sTrans.commit() != IDE_SUCCESS );
    sStage = 1;

    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 3:
            ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
            /* fall through */
        case 2:
            ( void )sTrans.rollback();
            /* fall through */
        case 1:
            ( void )sTrans.destroy( NULL );
            /* fall through */
        default:
            break;
    }

    ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. errorcode 0x%05"ID_XINT32_FMT" %s\n",
                           E_ERROR_CODE(ideGetErrorCode()),
                           ideGetErrorMsg(ideGetErrorCode()));
    return IDE_FAILURE;
}


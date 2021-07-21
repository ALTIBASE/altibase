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

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>

#include <iduMemMgr.h>

#include <smiTrans.h>
#include <smiStatement.h>

#include <qci.h>

#include <rpDef.h>

#include <rpdMeta.h>
#include <rpdCatalog.h>

#include <rpuProperty.h>

#include <rpdLockTableManager.h>

extern "C" int compareRpdMetaItemByTableOID( const void * aElem1,
                                             const void * aElem2 )
{
    int sJudge = 0;

    if ( ((rpdMetaItem*)aElem1)->mItem.mTableOID >
         ((rpdMetaItem*)aElem2)->mItem.mTableOID )
    {
        sJudge = 1;
    }
    else if ( ((rpdMetaItem*)aElem1)->mItem.mTableOID <
              ((rpdMetaItem*)aElem2)->mItem.mTableOID )
    {
        sJudge = -1;
    }
    else
    {
        sJudge = 0;
    }

    return sJudge;
}

extern "C" int compareRpdLockTableInfoByTableOID( const void * aElem1,
                                                  const void * aElem2 )
{
    int sJudge = 0;

    if ( ((rpdLockTableInfo*)aElem1)->mTableOID >
         ((rpdLockTableInfo*)aElem2)->mTableOID )
    {
        sJudge = 1;
    }
    else if ( ((rpdLockTableInfo*)aElem1)->mTableOID <
              ((rpdLockTableInfo*)aElem2)->mTableOID )
    {
        sJudge = -1;
    }
    else
    {
        sJudge = 0;
    }

    return sJudge;
}

static IDE_RC getTableHandleByName( idvSQL          * aStatistics,
                                    smiStatement    * aSmiStatement,
                                    SChar           * aUsername,
                                    SChar           * aTableName,
                                    void           ** aTableHandle,
                                    smSCN           * aTableSCN )
{
    UInt                sUserID = 0;

    void              * sTableHandle = NULL;
    smSCN               sTableSCN = 0;

    IDE_TEST( qciMisc::getUserID( aStatistics,
                                  aSmiStatement,
                                  aUsername,
                                  (UInt)idlOS::strlen( aUsername ),
                                  &sUserID )
              != IDE_SUCCESS );

    IDE_TEST( qciMisc::getTableHandleByName( aSmiStatement,
                                             sUserID,
                                             (UChar*)aTableName,
                                             (SInt)idlOS::strlen( aTableName ),
                                             &sTableHandle,
                                             &sTableSCN )
              != IDE_SUCCESS );

    *aTableHandle = sTableHandle;
    *aTableSCN = sTableSCN;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdLockTableManager::equalReplItem( rpdReplItems   *  aItem1,
                                           rpdReplItems   *  aItem2 )
{
    IDE_TEST_RAISE( aItem1->mTableOID != aItem2->mTableOID, ERR_MISSMATCH_TABLE_OID );

    IDE_TEST_RAISE( idlOS::strncmp( aItem1->mRepName,
                                    aItem2->mRepName,
                                    QC_MAX_OBJECT_NAME_LEN + 1 ) != 0,
                    ERR_MISSMATCH_REP_NAME );

    IDE_TEST_RAISE( idlOS::strncmp( aItem1->mLocalUsername,
                                    aItem2->mLocalUsername,
                                    QC_MAX_OBJECT_NAME_LEN + 1 ) != 0,
                    ERR_MISSMATCH_LOCAL_USER_NAME );

    IDE_TEST_RAISE( idlOS::strncmp( aItem1->mLocalTablename,
                                    aItem2->mLocalTablename,
                                    QC_MAX_OBJECT_NAME_LEN + 1 ) != 0,
                    ERR_MISSMATCH_LOCAL_TABLE_NAME );

    IDE_TEST_RAISE( idlOS::strncmp( aItem1->mLocalPartname,
                                    aItem2->mLocalPartname,
                                    QC_MAX_OBJECT_NAME_LEN + 1 ) != 0,
                    ERR_MISSMATCH_LOCAL_PARTITION_NAME );

    IDE_TEST_RAISE( idlOS::strncmp( aItem1->mRemoteUsername,
                                    aItem2->mRemoteUsername,
                                    QC_MAX_OBJECT_NAME_LEN + 1 ) != 0,
                    ERR_MISSMATCH_REMOTE_USER_NAME );

    IDE_TEST_RAISE( idlOS::strncmp( aItem1->mRemoteTablename,
                                    aItem2->mRemoteTablename,
                                    QC_MAX_OBJECT_NAME_LEN + 1 ) != 0,
                    ERR_MISSMATCH_REMOTE_TABLE_NAME );

    IDE_TEST_RAISE( idlOS::strncmp( aItem1->mRemotePartname,
                                    aItem2->mRemotePartname,
                                    QC_MAX_OBJECT_NAME_LEN + 1 ) != 0,
                    ERR_MISSMATCH_REMOTE_PARTITION_NAME );

    /* 아래 정보들은 비교할 필요 없다. 
    IDE_TEST_RAISE( idlOS::strncmp( aItem1->mIsPartition,
                                    aItem2->mIsPartition,
                                    2 ) != 0,
                    ERR_MISSMATCH_IS_PARTITION );

    IDE_TEST_RAISE( aItem1->mInvalidMaxSN != aItem2->mInvalidMaxSN,
                    ERR_MISSMATCH_INVALID_MAX_SN );

    IDE_TEST_RAISE( aItem1->mTSType != aItem2->mTBSType,
                    ERR_MISSMATCH_TBS_TYPE );

    IDE_TEST_RAISE( idlOS::strncmp( aItem1->mReplicationUnit,
                                    aItem2->mReplicationUnit,
                                    2 ) != 0,
                    ERR_MISSMATCH_REPLICATION_UNIT );

    IDE_TEST_RAISE( aItem1->mIsConditionSynced != aItem2->mIsConditionSynced,
                    ERR_MISSMATCH_IS_CONDITION_SYNCED );
    */

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MISSMATCH_TABLE_OID )
    {
    }
    IDE_EXCEPTION( ERR_MISSMATCH_REP_NAME )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_REPLICATION_NAME_MISMATCH,
                                  aItem1->mRepName,
                                  aItem2->mRepName ) );
    }
    IDE_EXCEPTION( ERR_MISSMATCH_LOCAL_USER_NAME )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_USER_NAME_MISMATCH,
                                  aItem1->mLocalTablename,
                                  aItem1->mLocalUsername,
                                  aItem2->mLocalTablename,
                                  aItem2->mLocalUsername ) );
    }
    IDE_EXCEPTION( ERR_MISSMATCH_LOCAL_TABLE_NAME )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_TABLE_NAME_MISMATCH,
                                  aItem1->mLocalTablename,
                                  aItem2->mLocalTablename ) );
    }
    IDE_EXCEPTION( ERR_MISSMATCH_LOCAL_PARTITION_NAME )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_PARTITION_NAME_MISMATCH,
                                  aItem1->mLocalPartname,
                                  aItem2->mLocalPartname ) );
    }
    IDE_EXCEPTION( ERR_MISSMATCH_REMOTE_USER_NAME )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_USER_NAME_MISMATCH,
                                  aItem1->mRemoteTablename,
                                  aItem1->mRemoteUsername,
                                  aItem2->mRemoteTablename,
                                  aItem2->mRemoteUsername ) );
    }
    IDE_EXCEPTION( ERR_MISSMATCH_REMOTE_TABLE_NAME )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_TABLE_NAME_MISMATCH,
                                  aItem1->mRemoteTablename,
                                  aItem2->mRemoteTablename ) );
    }
    IDE_EXCEPTION( ERR_MISSMATCH_REMOTE_PARTITION_NAME )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_PARTITION_NAME_MISMATCH,
                                  aItem1->mRemotePartname,
                                  aItem2->mRemotePartname ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdLockTableManager::equalMetaItemArray( rpdMetaItem   * aMetaItemArray1,
                                                UInt            aMetaItemArrayCount1,
                                                rpdMetaItem   * aMetaItemArray2,
                                                UInt            aMetaItemArrayCount2 )
{
    UInt              i = 0;

    // rpdReplications 으로 먼저 검사 완료
    IDE_DASSERT( aMetaItemArrayCount1 == aMetaItemArrayCount2 );

    for ( i = 0; i < aMetaItemArrayCount1; i++ )
    {
        IDE_TEST( equalReplItem( &(aMetaItemArray1[i].mItem),
                                 &(aMetaItemArray2[i].mItem) )
                  != IDE_SUCCESS )
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt rpdLockTableManager::getLockTableCount( void )
{
    UInt        sTableCount = 0;
    UInt        i = 0;

    for ( i = 0; i < mMetaItemArrayCount; i++ )
    {
        sTableCount++;

        if ( mMetaItemArray[i].mItem.mLocalPartname[0] != '\0' )
        {
            sTableCount++;
        }
    }

    return sTableCount;
}

IDE_RC rpdLockTableManager::buildLockTable( idvSQL                * aStatistics,
                                            smiStatement          * aSmiStatement,
                                            rpdReplItems          * aReplItem,
                                            rpdLockTableInfo      * aLockTableInfo,
                                            UInt                  * aAddTableCount )
{
    UInt     sIndex = 0;

    if ( aReplItem->mLocalPartname[0] != '\0' )
    {
        IDE_TEST( getTableHandleByName( aStatistics,
                                        aSmiStatement,
                                        aReplItem->mLocalUsername,
                                        aReplItem->mLocalTablename,
                                        &(aLockTableInfo[sIndex].mTableHandle),
                                        &(aLockTableInfo[sIndex].mTableSCN) )
                  != IDE_SUCCESS );
        aLockTableInfo[sIndex].mTableOID = smiGetTableId( aLockTableInfo[sIndex].mTableHandle );
        sIndex++;

    }

    aLockTableInfo[sIndex].mTableOID = (smOID)aReplItem->mTableOID;
    aLockTableInfo[sIndex].mTableHandle = (void *)smiGetTable( (smOID)(aReplItem->mTableOID) );
    aLockTableInfo[sIndex].mTableSCN = smiGetRowSCN( aLockTableInfo[sIndex].mTableHandle );
    sIndex++;

    *aAddTableCount = sIndex;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdLockTableManager::buildLockTableArray( idvSQL           * aStatistics,
                                                 iduVarMemList    * aMemory,
                                                 smiStatement     * aSmiStatement )
{
    UInt                  i = 0;
    UInt                  sTableCount = 0;
    UInt                  sAddTableCount = 0;
    UInt                  sIndex = 0;
    
    sTableCount = getLockTableCount();
    IDE_TEST( aMemory->cralloc( ID_SIZEOF(rpdLockTableInfo) * sTableCount,
                                (void**)&mTableInfoArray )
              != IDE_SUCCESS );
    mTableInfoCount = sTableCount;

    for ( i = 0; i < mMetaItemArrayCount; i++ )
    {
        IDE_TEST( buildLockTable( aStatistics,
                                  aSmiStatement,
                                  &(mMetaItemArray[i].mItem),
                                  &mTableInfoArray[sIndex],
                                  &sAddTableCount )
                  != IDE_SUCCESS );
        sIndex += sAddTableCount;
    }

    idlOS::qsort( mTableInfoArray,
                  mTableInfoCount,
                  ID_SIZEOF(rpdLockTableInfo),
                  compareRpdLockTableInfoByTableOID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdLockTableManager::build( idvSQL               * aStatistics,
                                   iduVarMemList        * aMemory,
                                   smiStatement         * aSmiStatement,
                                   SChar                * aRepName,
                                   RP_META_BUILD_TYPE     aMetaBuildType )
{
    vSLong      sOldItemCount = 0;

    mMetaItemArray = NULL;
    mMetaItemArrayCount = 0;

    mTableInfoArray = NULL;
    mTableInfoCount = 0;

    IDE_TEST( rpdCatalog::selectRepl( aSmiStatement,
                                      aRepName,
                                      &mReplication,
                                      ID_FALSE )
              != IDE_SUCCESS );

    if ( rpdMeta::needLastMetaItemInfo( aMetaBuildType, &mReplication ) == ID_TRUE )
    {
        mIsLastMetaItem = ID_TRUE;;
        mMetaItemArrayCount = mReplication.mItemCount;

        IDE_TEST_CONT( mMetaItemArrayCount == 0, NORMAL_EXIT );

        IDE_TEST( aMemory->cralloc( ID_SIZEOF(rpdMetaItem) * mMetaItemArrayCount,
                                    (void**)&mMetaItemArray )
                  != IDE_SUCCESS );

        IDE_TEST( buildLastMetaItemArray( aSmiStatement,
                                          aRepName,
                                          ID_FALSE,
                                          mMetaItemArray,
                                          mMetaItemArrayCount )
                  != IDE_SUCCESS );
    }
    else
    {
        mIsLastMetaItem = ID_FALSE;

        IDE_TEST( rpdCatalog::getReplOldItemsCount( aSmiStatement,
                                                    aRepName,
                                                    &sOldItemCount )
                  != IDE_SUCCESS );
        mMetaItemArrayCount = sOldItemCount;

        IDE_TEST_CONT( mMetaItemArrayCount == 0, NORMAL_EXIT );

        IDE_TEST( aMemory->cralloc( ID_SIZEOF(rpdMetaItem) * mMetaItemArrayCount,
                                    (void**)&mMetaItemArray )
                  != IDE_SUCCESS );

        IDE_TEST( buildOldMetaItemArray( aMemory,
                                         aSmiStatement,
                                         aRepName,
                                         mMetaItemArray,
                                         mMetaItemArrayCount )
                  != IDE_SUCCESS );
    }

    IDE_TEST( buildLockTableArray( aStatistics,
                                   aMemory,
                                   aSmiStatement )
              != IDE_SUCCESS );

    RP_LABEL( NORMAL_EXIT );

    mIsValidateComplete = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    mMetaItemArray = NULL;
    mMetaItemArrayCount = 0;
    mTableInfoArray = NULL;
    mTableInfoCount = 0;

    return IDE_FAILURE;
}

IDE_RC rpdLockTableManager::build( idvSQL                * aStatistics,
                                   iduVarMemList         * aMemory,
                                   SChar                 * aRepName,
                                   RP_META_BUILD_TYPE      aMetaBuildType )
{
    smiTrans                sTrans;

    smiStatement          * sRootSmiStatement = NULL;
    smiStatement            sSmiStatement;

    UInt                    sStage = 0;
    idBool                  sIsTxBegin = ID_FALSE;

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sTrans.begin( &sRootSmiStatement,
                            aStatistics,
                            (UInt)RPU_ISOLATION_LEVEL       |
                            SMI_TRANSACTION_NORMAL          |
                            SMI_TRANSACTION_REPL_REPLICATED |
                            SMI_COMMIT_WRITE_NOWAIT,
                            RP_UNUSED_RECEIVER_INDEX )
              != IDE_SUCCESS );
    sIsTxBegin = ID_TRUE;
    sStage = 2;

    IDE_TEST( sSmiStatement.begin( NULL,
                                   sRootSmiStatement,
                                   SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( build( aStatistics,
                     aMemory,
                     &sSmiStatement,
                     aRepName,
                     aMetaBuildType ) 
              != IDE_SUCCESS );

    IDE_TEST( sSmiStatement.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sStage = 2;

    sStage = 1;
    IDE_TEST( sTrans.commit() != IDE_SUCCESS );
    sIsTxBegin = ID_FALSE;

    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 3:
            (void)sSmiStatement.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            IDE_ASSERT( sTrans.rollback() == IDE_SUCCESS );
            sIsTxBegin = ID_FALSE;
        case 1:
            if ( sIsTxBegin == ID_TRUE )
            {
                IDE_ASSERT( sTrans.rollback() == IDE_SUCCESS );
                sIsTxBegin = ID_FALSE;
            }
            (void)sTrans.destroy( NULL );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpdLockTableManager::validateAndLock( smiTrans            * aTrans,
                                             smiTBSLockValidType   aTBSLvType,
                                             smiTableLockMode      aLockMode )
{
    UInt    i = 0;

    for ( i = 0; i < mTableInfoCount; i++ )
    {
        IDE_TEST( smiValidateAndLockObjects( aTrans,
                                             mTableInfoArray[i].mTableHandle,
                                             mTableInfoArray[i].mTableSCN,
                                             aTBSLvType,
                                             aLockMode,
                                             smiGetDDLLockTimeOut(aTrans),
                                             ID_FALSE )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdLockTableManager::validateLockTable( idvSQL               * aStatistics,
                                               iduVarMemList        * aMemory,
                                               smiStatement         * aParentSmiStatement,
                                               SChar                * aRepName,
                                               RP_META_BUILD_TYPE     aMetaBuildType )
{
    rpdReplications   sReplication;
    rpdMetaItem     * sMetaItemArray = NULL;
    UInt              sMetaItemArrayCount = 0;
    smiStatement      sSmiStatement;
    idBool            sIsBegin = ID_FALSE;

    /* validate 한 이후는 이미 table 에 lock 이 잡혀 있어
     * 중간에 ddl 를 수행할수 없기 때문에 검사 할 필요가 없다. 
     */
    IDE_TEST_CONT( mIsValidateComplete == ID_TRUE, SKIP_VALIDATE );

    IDE_TEST( sSmiStatement.begin( aStatistics,
                                   aParentSmiStatement,
                                   SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );
    sIsBegin = ID_TRUE;

    /* SYS_REPLICATIONS_ */
    IDE_TEST( rpdCatalog::selectRepl( &sSmiStatement,
                                      aRepName,
                                      &sReplication,
                                      ID_FALSE )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( mReplication.mItemCount != sReplication.mItemCount,
                    ERR_MISSMATCH_ITEM_COUNT );


    IDE_TEST_CONT( mReplication.mItemCount == 0, NORMAL_EXIT );

    /* check replication reset */
    if ( sReplication.mXSN == SM_SN_NULL )
    {
        /* after reset, mXSN = SM_SN_NULL */
        IDE_TEST_RAISE( mReplication.mXSN != SM_SN_NULL, ERR_MISSMATCH_LOCK_TABLE );
    }

    /* SYS_REPL_ITEMS SYS_REPL_OLD_ITEMS_ */
    IDE_TEST_RAISE( rpdMeta::needLastMetaItemInfo( aMetaBuildType, &sReplication )
                    != mIsLastMetaItem, ERR_MISSMATCH_LOCK_TABLE );

    if ( mIsLastMetaItem == ID_TRUE )
    {
        sMetaItemArrayCount = sReplication.mItemCount;

        IDE_TEST( aMemory->cralloc( ID_SIZEOF(rpdMetaItem) * sMetaItemArrayCount,
                                    (void**)&sMetaItemArray )
                  != IDE_SUCCESS );

        IDE_TEST( buildLastMetaItemArray( &sSmiStatement,
                                          aRepName,
                                          ID_FALSE,
                                          sMetaItemArray,
                                          sMetaItemArrayCount )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( rpdCatalog::getReplOldItemsCount( &sSmiStatement,
                                                    aRepName,
                                                    (vSLong*)&sMetaItemArrayCount )
                  != IDE_SUCCESS );

        IDE_TEST_CONT( sMetaItemArrayCount == 0, NORMAL_EXIT );

        IDE_TEST( aMemory->cralloc( ID_SIZEOF(rpdMetaItem) * sMetaItemArrayCount,
                                    (void**)&sMetaItemArray )
                  != IDE_SUCCESS );

        IDE_TEST( buildOldMetaItemArray( aMemory,
                                         &sSmiStatement,
                                         aRepName,
                                         sMetaItemArray,
                                         sMetaItemArrayCount )
                  != IDE_SUCCESS );
    }

    IDE_TEST_RAISE( equalMetaItemArray( mMetaItemArray,
                                        mMetaItemArrayCount,
                                        sMetaItemArray,
                                        sMetaItemArrayCount )
                    != IDE_SUCCESS, ERR_MISSMATCH_LOCK_TABLE );

    RP_LABEL( NORMAL_EXIT );

    mIsValidateComplete = ID_TRUE;

    IDE_TEST( sSmiStatement.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsBegin = ID_FALSE;

    RP_LABEL( SKIP_VALIDATE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MISSMATCH_ITEM_COUNT )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_REPLICATION_ITEM_COUNT_MISMATCH,
                                  mReplication.mItemCount,
                                  sReplication.mItemCount ) );

        IDE_SET( ideSetErrorCode( rpERR_REBUILD_RPD_MISS_MATCH_LOCK_TABLE_AND_META ) );
    }
    IDE_EXCEPTION( ERR_MISSMATCH_LOCK_TABLE )
    {
        IDE_SET( ideSetErrorCode( rpERR_REBUILD_RPD_MISS_MATCH_LOCK_TABLE_AND_META ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsBegin == ID_TRUE )
    {
        sIsBegin = ID_FALSE;
        (void)sSmiStatement.end( SMI_STATEMENT_RESULT_FAILURE );
    }

    /* retry 시 qmxMem 메모리를 해제 */
    if ( ( ideIsRetry() == ID_TRUE ) || ( ideIsRebuild() == ID_TRUE ) )
    {
        IDE_SET( ideSetErrorCode( rpERR_REBUILD_RPD_MISS_MATCH_LOCK_TABLE_AND_META ) );
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpdLockTableManager::buildLastMetaItemArray( smiStatement        * aSmiStatement,
                                                    SChar               * aRepName,
                                                    idBool                aIsUpdateFlag,
                                                    rpdMetaItem         * aMetaItemArray,
                                                    UInt                  aMetaItemArrayCount )
{
    IDE_TEST( rpdCatalog::selectReplItems( aSmiStatement,
                                           aRepName,
                                           aMetaItemArray,
                                           aMetaItemArrayCount,
                                           aIsUpdateFlag )
              != IDE_SUCCESS );

    idlOS::qsort( aMetaItemArray,
                  aMetaItemArrayCount,
                  ID_SIZEOF(rpdMetaItem),
                  compareRpdMetaItemByTableOID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* SYS_REPLICATIONS_ 에서  item count 을 select  한 시점과
     * SYS_REPL_ITEMS_ 에 저장된 item 개수가 변경 되었다.
     */
    if ( ( ideGetErrorCode() == rpERR_ABORT_RPD_TOO_MANY_REPLICATION_ITEMS ) ||
         ( ideGetErrorCode() == rpERR_ABORT_RPD_NOT_ENOUGH_REPLICATION_ITEMS ) )
    {
        IDE_SET( ideSetErrorCode( rpERR_REBUILD_RPD_MISS_MATCH_LOCK_TABLE_AND_META ) );
    }

    return IDE_FAILURE;
}

IDE_RC rpdLockTableManager::buildOldMetaItemArray( iduVarMemList       * aMemory,
                                                   smiStatement        * aSmiStatement,
                                                   SChar               * aRepName,
                                                   rpdMetaItem         * aMetaItemArray,
                                                   UInt                  aMetaItemArrayCount )
{
    rpdOldItem  * sOldItemArray = NULL;
    UInt          i = 0;

    IDE_TEST( aMemory->cralloc( ID_SIZEOF(rpdOldItem) * aMetaItemArrayCount,
                                (void**)&sOldItemArray )
              != IDE_SUCCESS );

    IDE_TEST( rpdCatalog::selectReplOldItems( aSmiStatement,
                                              aRepName,
                                              sOldItemArray,
                                              aMetaItemArrayCount )
              != IDE_SUCCESS );

    for ( i = 0; i < aMetaItemArrayCount; i++ )
    {
        rpdMeta::fillOldMetaItem( &aMetaItemArray[i], &sOldItemArray[i] );
    }

    idlOS::qsort( aMetaItemArray,
                  aMetaItemArrayCount,
                  ID_SIZEOF(rpdMetaItem),
                  compareRpdMetaItemByTableOID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* SYS_REPLICATIONS_ 에서  item count 을 select  한 시점과
     * SYS_REPL_OLD_ITEMS_ 에 저장된 item 개수가 변경 되었다.
     */
    if ( ( ideGetErrorCode() == rpERR_ABORT_RPD_TOO_MANY_REPLICATION_ITEMS ) ||
         ( ideGetErrorCode() == rpERR_ABORT_RPD_NOT_ENOUGH_REPLICATION_ITEMS ) )
    {
        IDE_SET( ideSetErrorCode( rpERR_REBUILD_RPD_MISS_MATCH_LOCK_TABLE_AND_META ) );
    }

    return IDE_FAILURE;
}


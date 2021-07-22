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
 * $Id: smiTrans.h 90824 2021-05-13 05:35:21Z minku.kang $
 **********************************************************************/

#ifndef _O_SMI_TRANS_H_
# define _O_SMI_TRANS_H_ 1

# include <idv.h>
# include <smiDef.h>
# include <smiStatement.h>
# include <smxTrans.h>

# define SMI_TRANS_SLOT_ID_NULL ID_UINT_MAX

# define SMI_MAX_SVPNAME_SIZE SMX_MAX_SVPNAME_SIZE

# define SMI_DDL_BEGIN_SAVEPOINT       SM_DDL_BEGIN_SAVEPOINT
# define SMI_DDL_INFO_SAVEPOINT        SM_DDL_INFO_SAVEPOINT

typedef enum smiTransInitOpt
{
    SMI_TRANS_INIT,
    SMI_TRANS_ATTACH
} smiTransInitOpt;

typedef enum smiBackupTableInfoType
{
    SMI_BACKUP_NONE,
    SMI_BACKUP_OLD_TABLE_INFO,
    SMI_BACKUP_NEW_TABLE_INFO,
    SMI_BACKUP_IMPOSSIBLE,
} smiBackupTableInfoType;

class smiTrans
{
    friend class smiTable;
    friend class smiObject;
    friend class smiStatement;
    friend class smiTableCursor;

 public:  /* POD class type should make non-static data members as public */

    void*            mTrans;
    UInt             mFlag;
    smiStatement   * mStmtListHead;
    smiTableCursor   mCursorListHead;

    //PROJ-1608
    smLSN             mBeginLSN;
    smLSN             mCommitLSN;

    /* BUG-46786 : FOR SHARDING
       smiStatement::end() 시 implicit savepoint를 해제하지 않고, 이 변수에 저장해둔다.
       샤딩에서 rollback 시 사용한다. */
    smxSavepoint    * mImpSVP4Shard;

    static smiTransactionalDDLCallback mTransactionalDDLCallback;
 
 public:
    IDE_RC initialize();

    IDE_RC initializeInternal( void );

    IDE_RC destroy( idvSQL * aStatistics );
 
    // For Local & Global Transaction
    IDE_RC begin( smiStatement** aStatement,
                  idvSQL        *aStatistics,
                  UInt           aFlag = ( SMI_ISOLATION_CONSISTENT     |
                                           SMI_TRANSACTION_NORMAL       |
                                           SMI_TRANSACTION_REPL_DEFAULT |
                                           SMI_COMMIT_WRITE_NOWAIT),
                  UInt           aReplID        = SMX_NOT_REPL_TX_ID,
                  idBool         aIgnoreRetry   = ID_FALSE,
                  idBool         aIsServiceTX   = ID_FALSE );

    void setDistTxInfo( smiDistTxInfo * aDistTxInfo );

    IDE_RC rollback( const SChar* aSavePoint = NULL, UInt aTransReleasePolicy = SMI_RELEASE_TRANSACTION );
    /*
      PROJ-1677 DEQ
                                    (sm-commit& mm-commit )
                                     |
                                     V                         
      dequeue statement begin  ----------------- > execute .... 
                                          ^
                                          |
                                        get queue stamp (session의 queue stamp에 저장)
                                        
      dequeue statement begin과 execute 바로 직전에 queue item이 commit되었다면,
      dequeue execute시에  해당 queue item을 MVCC때문에 볼수 없어서 대기 상태로 간다.
      그리고 session의 queue timestamp와 queue timestamp과 같아서  다음 enqueue
      event가 발생할때 까지  queue에 데이타가 있음에도 불구하고 dequeue를 할수 없다 .
      이문제를 해결 하기 위하여  commitSCN을 두었다. */
   
    inline idBool isPrepared() 
    { 
        return getSmxTrans()->isPrepared();
    }

    IDE_RC commit(smSCN *aCommitSCN = NULL, UInt aTransReleasePolicy = SMI_RELEASE_TRANSACTION);

    // For Global Transaction 
    /* BUG-18981 */
    IDE_RC prepare( ID_XID *aXID, smSCN * aPrepareSCN = NULL, idBool aLogging = ID_TRUE );
    IDE_RC isReadOnly(idBool *aIsReadOnly);
    IDE_RC attach( SInt aSlotID );
    IDE_RC dettach();
    IDE_RC realloc( idvSQL *aStatistics, idBool aIgnoreRetry );
    void   showOIDList();
    
    IDE_RC savepoint(const SChar* aSavePoint,
                     smiStatement *aStatement = NULL );

    // For BUG-12512
    void   reservePsmSvp( idBool aIsShard );
    void   clearPsmSvp( );
    IDE_RC abortToPsmSvp( );

    idBool isExistExpSavepoint(const SChar *aSavepointName);  /* BUG-48489 */

    // TASK-7244 PSM partial rollback in Sharding
    idBool isShardPsmSvpReserved();
    
    smTID  getTransID();
    inline smiStatement* getStatement( void );
    inline void* getTrans( void );
    inline UInt  getIsolationLevel();
    inline UInt  getReplicationMode();
    inline UInt  getTransactionMode();

    UInt   getFirstUpdateTime();

    // QP에서 Meta가 접근된 경우 이 함수를 호출하여
    // Transaction에 Meta접근 여부를 세팅한다
    IDE_RC setMetaTableModified();
    smSN getBeginSN();
    smSN getCommitSN();

    // DDL Transaction을 표시하는 Log Record를 기록한다.
    IDE_RC writeDDLLog();

    void  setStatistics( idvSQL * aStatistics );
    idvSQL * getStatistics( void );
    IDE_RC setReplTransLockTimeout( UInt aReplTransLockTimeout );
    UInt getReplTransLockTimeout( );

    idBool isBegin();

    idBool isReusableRollback( void );
    void   setCursorHoldable( void );

    IDE_RC setExpSvpForBackupDDLTargetTableInfo( smOID   aOldTableOID,
                                                 UInt    aOldPartOIDCount,
                                                 smOID * aOldPartOID,
                                                 smOID   aNewTableOID, 
                                                 UInt    aNewPartOIDCount,
                                                 smOID * aNewPartOID );

    IDE_RC allocNSetDDLTargetTableInfo( UInt                     aTableID,
                                        void                   * aOldTableInfo,
                                        void                   * aNewTableInfo,
                                        idBool                   aIsReCreated,
                                        smiDDLTargetTableInfo ** aInfo );
    void   freeDDLTargetTableInfo( smiDDLTargetTableInfo * aDDLTargetTableInfo );

    IDE_RC allocNSetDDLTargetPartTableInfo( smiDDLTargetTableInfo * aInfo,
                                            UInt                    aTableID,
                                            idBool                  aIsReCreated,
                                            void                  * aPartOldTableInfo,
                                            void                  * aPartNewTableInfo );
    void   freeDDLTargetPartTableInfo( smiDDLTargetTableInfo * aInfo );
    void   setGlobalSMNChangeFunc( smTransApplyShardMetaChangeFunc aFunc );

    /* BUG-46786 */
    static idBool checkImpSVP4Shard( smiTrans * aTrans );
    static IDE_RC abortToImpSVP4Shard( smiTrans * aTrans );
    
    static void   setTransactionalDDLCallback( smiTrasactionalDDLCallback * aTransactionalDDLCallback );

    static smiBackupTableInfoType checkBackupTableInfoType( smOID aOldTableOID, smOID aNewTableOID );

    static IDE_RC backupDDLTargetTableInfo( smiTrans               * aTrans,
                                            smOID                    aOldTableOID,
                                            UInt                     aOldPartOIDCount,
                                            smOID                  * aOldPartOIDArray,
                                            smOID                    aNewTableOID,
                                            UInt                     aNewPartOIDCount,
                                            smOID                  * aNewPartOIDArray,
                                            smiDDLTargetTableInfo ** aDDLTargetTableInfo );
    static void   removeDDLTargetTableInfo( smiTrans * aTrans, smiDDLTargetTableInfo * aDDLTargetTableInfo );
    static void   restoreDDLTargetOldTableInfo( smiDDLTargetTableInfo * aDDLTargetTableInfo );
    static void   destroyDDLTargetNewTableInfo( smiDDLTargetTableInfo * aDDLTargetTableInfo );

    /* BUG-48250 */
    void   setIndoubtFetchTimeout( UInt aTimeout );
    void   setIndoubtFetchMethod( UInt aMethod );
    /* BUG-48829 */
    void   setGlobalTransactionLevel( idBool aIsGCTx );

    void   setInternalTableSwap();

private:
    smxTrans* getSmxTrans();
};

inline void smiTrans::setGlobalSMNChangeFunc( smTransApplyShardMetaChangeFunc aFunc )
{
    getSmxTrans()->setGlobalSMNChangeFunc( aFunc );
    return ;
}

inline smxTrans* smiTrans::getSmxTrans( void )
{
    return (smxTrans*)mTrans;
}

inline void* smiTrans::getTrans( void )
{
    return mTrans;
}

inline smiStatement* smiTrans::getStatement( void )
{
    return mStmtListHead;
}

inline UInt smiTrans::getIsolationLevel( void )
{
    return mFlag & SMI_ISOLATION_MASK;
}

inline UInt smiTrans::getReplicationMode()
{
    return mFlag & SMI_TRANSACTION_REPL_MASK;
}

inline UInt smiTrans::getTransactionMode( void )
{
    return mFlag & SMI_TRANSACTION_MASK;
}

inline void smiTrans::setTransactionalDDLCallback( smiTrasactionalDDLCallback * aTransactionalDDLCallback )
{
    mTransactionalDDLCallback = *aTransactionalDDLCallback;
}

inline smiBackupTableInfoType smiTrans::checkBackupTableInfoType( smOID aOldTableOID, smOID aNewTableOID )
{
    smiBackupTableInfoType sType = SMI_BACKUP_NONE;

    if ( ( aOldTableOID == SM_OID_NULL ) && ( aNewTableOID == SM_OID_NULL ) )
    {
        sType = SMI_BACKUP_NONE;
    }
    else if ( ( aOldTableOID != SM_OID_NULL ) && ( aNewTableOID == SM_OID_NULL ) )
    {        
        sType = SMI_BACKUP_OLD_TABLE_INFO;
    }
    else if ( ( aOldTableOID == SM_OID_NULL ) && ( aNewTableOID != SM_OID_NULL ) )
    {
        sType = SMI_BACKUP_NEW_TABLE_INFO;
    }
    else
    {
        sType = SMI_BACKUP_IMPOSSIBLE;
    }

    return sType;
}

inline IDE_RC smiTrans::backupDDLTargetTableInfo( smiTrans               * aTrans, 
                                                  smOID                    aOldTableOID,
                                                  UInt                     aOldPartOIDCount,
                                                  smOID                  * aOldPartOIDArray,
                                                  smOID                    aNewTableOID,
                                                  UInt                     aNewPartOIDCount,
                                                  smOID                  * aNewPartOIDArray,
                                                  smiDDLTargetTableInfo ** aDDLTargetTableInfo )
{
    smiBackupTableInfoType sType = checkBackupTableInfoType( aOldTableOID, aNewTableOID );

    switch ( sType )
    {
        case SMI_BACKUP_OLD_TABLE_INFO:
            IDE_TEST( mTransactionalDDLCallback.backupDDLTargetOldTableInfo( aTrans, 
                                                                             aOldTableOID,
                                                                             aOldPartOIDCount,
                                                                             aOldPartOIDArray,
                                                                             aDDLTargetTableInfo )
                      != IDE_SUCCESS );
            break;

        case SMI_BACKUP_NEW_TABLE_INFO:
            IDE_TEST( mTransactionalDDLCallback.backupDDLTargetNewTableInfo( aTrans, 
                                                                             aNewTableOID,
                                                                             aNewPartOIDCount,
                                                                             aNewPartOIDArray,
                                                                             aDDLTargetTableInfo )
                      != IDE_SUCCESS );
            break;

        case SMI_BACKUP_NONE:
            break;

        case SMI_BACKUP_IMPOSSIBLE:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline void smiTrans::removeDDLTargetTableInfo( smiTrans * aTrans, smiDDLTargetTableInfo * aDDLTargetTableInfo )
{
    mTransactionalDDLCallback.removeDDLTargetTableInfo( aTrans, aDDLTargetTableInfo );
}

inline void smiTrans::restoreDDLTargetOldTableInfo( smiDDLTargetTableInfo * aDDLTargetTableInfo )
{
    mTransactionalDDLCallback.restoreDDLTargetOldTableInfo( aDDLTargetTableInfo );
}

inline void smiTrans::destroyDDLTargetNewTableInfo( smiDDLTargetTableInfo * aDDLTargetTableInfo )
{
    mTransactionalDDLCallback.destroyDDLTargetNewTableInfo( aDDLTargetTableInfo );
}

/* BUG-48586 */
inline void smiTrans::setInternalTableSwap()
{
    getSmxTrans()->setInternalTableSwap();
}

typedef struct smiTransNode
{
    smiTrans    mSmiTrans;
    iduListNode mNode;
} smiTransNode;


#endif /* _O_SMI_TRANS_H_ */

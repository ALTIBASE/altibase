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

#include <mmtAuditManager.h>
#include <mmcSession.h>
#include <idvAudit.h>
#include <qci.h>
#include <mmErrorCode.h>

/* class members */
idBool             mmtAuditManager::mIsAuditStarted;

iduLatch           mmtAuditManager::mAuditMetaLatch;

mmtAuditMetaInfo   mmtAuditManager::mMetaInfo;

/* extern "C" */
IDL_EXTERN_C int
compareAuditOperationByObject( const void* aElem1,  /* key */
                               const void* aElem2 ) /* base */
{
    qciAuditOperation  * sElem1 = (qciAuditOperation*)aElem1;  
    qciAuditOperation  * sElem2 = (qciAuditOperation*)aElem2; 
    
    IDE_ASSERT( sElem1 != NULL );
    IDE_ASSERT( sElem2 != NULL );

    if ( sElem1->userID > sElem2->userID )
    {
        return 1;
    }
    else if ( sElem1->userID < sElem2->userID )
    {
        return -1;
    }
    else
    {
        return idlOS::strncmp( sElem1->objectName,
                               sElem2->objectName,
                               QCI_MAX_OBJECT_NAME_LEN + 1 );
    }
}

IDL_EXTERN_C int
compareAuditOperationByUser( const void* aElem1,  /* key */
                             const void* aElem2 ) /* base */
{
    qciAuditOperation  * sElem1 = (qciAuditOperation*)aElem1;
    qciAuditOperation  * sElem2 = (qciAuditOperation*)aElem2;
    
    IDE_ASSERT( sElem1 != NULL );
    IDE_ASSERT( sElem2 != NULL );
    
    if ( sElem1->userID > sElem2->userID )
    {
        return 1;
    }
    else if ( sElem1->userID < sElem2->userID )
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

/* BUG-38204 Server crashes when calling the bsearch() function on Solaris */
inline static qciAuditOperation *getCondition( qciAuditOperation *aSearch, 
                                               qciAuditOperation *aMetaInfo,  
                                               UInt               aMetaInfoCount,
                                               PDL_COMPARE_FUNC   aCompareFunc )
{
    qciAuditOperation *sCondition=NULL;

    if( aMetaInfoCount > 0 )
    {
        sCondition = (qciAuditOperation *)
            idlOS::bsearch( aSearch, 
                            aMetaInfo,
                            aMetaInfoCount,
                            ID_SIZEOF(qciAuditOperation), 
                            aCompareFunc );
    }
    else
    {
        sCondition = NULL;
    }

    return sCondition;
}

inline static qciAuditOperation* getObjectCondition( qciAuditOperation *aSearch, mmtAuditMetaInfo *aMetaInfo )
{
    return getCondition( aSearch, 
                         aMetaInfo->mObjectOperations, 
                         aMetaInfo->mObjectOperationCount,
                         compareAuditOperationByObject );
}

inline static qciAuditOperation* getUserCondition( qciAuditOperation *aSearch, mmtAuditMetaInfo *aMetaInfo )
{
    return getCondition( aSearch, 
                         aMetaInfo->mUserOperations,
                         aMetaInfo->mUserOperationCount,
                         compareAuditOperationByUser );
}

/* BUG-39311 Prioritization of audit conditions */
inline static qciAuditOperation *getAllUserConditionFromMeta( mmtAuditMetaInfo *aMetaInfo )
{
    qciAuditOperation *sCondition = NULL;
    UInt               i;

    for ( i = 0; i < aMetaInfo->mUserOperationCount; i++ )
    {
        if( aMetaInfo->mUserOperations[i].userID == 0 )
        {
            sCondition = &aMetaInfo->mUserOperations[i];
            break;
        }
        else
        {
            /* do nothing */
        }
    }

    return sCondition;
}

/* mmtAuditManager::initialize()
 * This initializes itself when server starts up.
 *
 * @param  void
 * @return IDE_RC
 * @see
 */
IDE_RC mmtAuditManager::initialize()
{
    idBool                   sIsStarted;
    qciAuditManagerCallback  sCallback;
    SInt                     sState = 0;

    sCallback.mReloadAuditCond = mmtAuditManager::reloadAuditCondCallback;
    sCallback.mStopAudit       = mmtAuditManager::stopAuditCallback;

    IDE_TEST(mAuditMetaLatch.initialize((SChar *)"MMT_AUDIT_LATCH")
             != IDE_SUCCESS);
    sState = 1;

    IDE_TEST(idvAudit::initialize() != IDE_SUCCESS);

    /* set callback functions */
    IDE_TEST(qci::setAuditManagerCallback( &sCallback ) != IDE_SUCCESS );

    /* iduMemMgr로 alloc했다. */
    IDE_TEST( qciMisc::getAuditMetaInfo( &sIsStarted,
                                         &mMetaInfo.mObjectOperations, 
                                         &mMetaInfo.mObjectOperationCount,
                                         &mMetaInfo.mUserOperations, 
                                         &mMetaInfo.mUserOperationCount )
              != IDE_SUCCESS );
    sState = 2;

    /* BUG-39311 Prioritization of audit conditions */
    mMetaInfo.mAllUserCondition = getAllUserConditionFromMeta( &mMetaInfo );

    /* set audit status */
    setAuditStarted( sIsStarted );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            (void) mAuditMetaLatch.destroy();
            break;

        case 2:
            finalize();
            break;
    }

    return IDE_FAILURE;
}

IDE_RC mmtAuditManager::finalize()
{
    stopAuditCallback();

    /* destory mutexes */
    (void) mAuditMetaLatch.destroy();

    return IDE_SUCCESS;
}

idBool mmtAuditManager::isAuditStarted( void )
{
    return mIsAuditStarted;
}   

/* mmtAuditManager::isAuditWriteByAccess() 
 * This judges if a statement in execution will be written or not as an audit trail. 
 *
 * @param  [in] aAuditObject   The object list of a statement in execution.
 * @param  [in] aOperIndex     Operation Index
 * @param  [in] aExecutionFlag 0:failure, 1:rebuild, 2:retry, 3:queue empty, 4:success
 * @return ID_TRUE will be returned if the statement should be logged or ID_FALSE if not.
 * @see
 * @throws
 */
idBool mmtAuditManager::isAuditWriteObjectByAccess( qciAuditRefObject *aAuditObject,
                                                    UInt               aOperIndex,
                                                    mmcExecutionFlag   aExecutionFlag )
{
    idBool             sIsWriteLog = ID_FALSE;  /* set default value, ID_FALSE */
    qciAuditOperation  sSearch;
    qciAuditOperation *sCondition = NULL;
    UInt               sOperation;

    if( aOperIndex < QCI_AUDIT_OPER_MAX )
    {
        sSearch.userID = aAuditObject->userID;
        idlOS::strncpy( sSearch.objectName, aAuditObject->objectName, QCI_MAX_OBJECT_NAME_LEN + 1 );

        /* BUG-38204 */
        sCondition = getObjectCondition( &sSearch, &mMetaInfo );
        if( sCondition != NULL )
        {
            sOperation = sCondition->operation[aOperIndex];
        
            if( aExecutionFlag == MMC_EXECUTION_FLAG_SUCCESS )
            {
                /* in case of success */
                if( (sOperation & QCI_AUDIT_LOGGING_ACCESS_SUCCESS_MASK)
                    == QCI_AUDIT_LOGGING_ACCESS_SUCCESS_TRUE )
                {
                    sIsWriteLog = ID_TRUE;
                }
            }
            else if( aExecutionFlag == MMC_EXECUTION_FLAG_FAILURE )
            {
                /* in case of failure */
                if( (sOperation & QCI_AUDIT_LOGGING_ACCESS_FAILURE_MASK)
                    == QCI_AUDIT_LOGGING_ACCESS_FAILURE_TRUE )
                {
                    sIsWriteLog = ID_TRUE;
                }
            }
            else
            {
                /* not logging */
            }
        }
    }

    return sIsWriteLog;
}

/* mmtAuditManager::isAuditWriteBySession() 
 * This judges if a statement in execution will be written or not as an audit trail. 
 *
 * @param  [in] aAuditObject   The object list of a statement in execution.
 * @param  [in] aOperIndex     
 * @param  [in] aStat          Statistics information
 * @return ID_TRUE will be returned if the statement should be logged or ID_FALSE if not.
 * @see
 * @throws
 */
idBool mmtAuditManager::isAuditWriteObjectBySession( qciAuditRefObject *aAuditObject,
                                                     UInt               aOperIndex,
                                                     idvSQL            *aStat )
{
    idBool             sIsWriteLog = ID_FALSE;  /* set default value, ID_FALSE */
    qciAuditOperation  sSearch;
    qciAuditOperation *sCondition = NULL;
    UInt               sOperation;

    if( aOperIndex < QCI_AUDIT_OPER_MAX )
    {
        sSearch.userID = aAuditObject->userID;
        idlOS::strncpy( sSearch.objectName, aAuditObject->objectName, QCI_MAX_OBJECT_NAME_LEN + 1 );

        /* BUG-38204 */
        sCondition = getObjectCondition( &sSearch, &mMetaInfo );
        if( sCondition != NULL )
        {
            sOperation = sCondition->operation[aOperIndex];
        
            /* in case of success */
            if( ((sOperation & QCI_AUDIT_LOGGING_SESSION_SUCCESS_MASK)
                 == QCI_AUDIT_LOGGING_SESSION_SUCCESS_TRUE)
                && ( aStat->mExecuteSuccessCount > 0) )
            {
                sIsWriteLog = ID_TRUE;
            }
            
            /* in case of failure */
            if( ((sOperation & QCI_AUDIT_LOGGING_SESSION_FAILURE_MASK)
                 == QCI_AUDIT_LOGGING_SESSION_FAILURE_TRUE)
                && ( aStat->mExecuteFailureCount > 0) )
            {
                sIsWriteLog = ID_TRUE;
            }
        }
    }

    return sIsWriteLog;
}

/* mmtAuditManager::isAuditWriteUserByAccess() 
 * This judges if a statement in execution will be written or not as an audit trail. 
 *
 * @param  [in] aAuditObject   The object list of a statement in execution.
 * @param  [in] aExecutionFlag 0:failure, 1:rebuild, 2:retry, 3:queue empty, 4:success
 * @return ID_TRUE will be returned if the statement should be logged or ID_FALSE if not.
 * @see
 * @throws
 */
idBool mmtAuditManager::isAuditWriteUserByAccess( UInt               aUserID,
                                                  UInt               aOperIndex,
                                                  mmcExecutionFlag   aExecutionFlag )
{
    idBool             sIsWriteLog = ID_FALSE;  /* set default value, ID_FALSE */
    qciAuditOperation *sCondition = NULL;
    qciAuditOperation  sSearch;
    UInt               sOperation;

    if( aOperIndex < QCI_AUDIT_OPER_MAX )
    {
        sSearch.userID = aUserID;

        /* BUG-38204 */
        sCondition = getUserCondition( &sSearch, &mMetaInfo );
        if( sCondition != NULL )
        {
            sOperation = sCondition->operation[aOperIndex];
            
            if( aExecutionFlag == MMC_EXECUTION_FLAG_SUCCESS )
            {
                /* in case of success */
                if( (sOperation & QCI_AUDIT_LOGGING_ACCESS_SUCCESS_MASK)
                    == QCI_AUDIT_LOGGING_ACCESS_SUCCESS_TRUE )
                {   
                    sIsWriteLog = ID_TRUE;
                }
            }
            else if( aExecutionFlag == MMC_EXECUTION_FLAG_FAILURE )
            {
                /* in case of failure */
                if( (sOperation & QCI_AUDIT_LOGGING_ACCESS_FAILURE_MASK)
                    == QCI_AUDIT_LOGGING_ACCESS_FAILURE_TRUE )
                {
                    sIsWriteLog = ID_TRUE;
                }
            }
            else
            {
                /* not logging */
            }
        }
    }
    
    return sIsWriteLog;
}

/* mmtAuditManager::isAuditWriteBySession() 
 *
 * @param  
 * @return ID_TRUE will be returned if the statement should be logged or ID_FALSE if not.
 */
idBool mmtAuditManager::isAuditWriteUserBySession( UInt          aUserID,
                                                   UInt          aOperIndex,
                                                   idvSQL       *aStat )
{
    idBool             sIsWriteLog = ID_FALSE;  /* set default value, ID_FALSE */
    qciAuditOperation  sSearch;
    qciAuditOperation *sCondition = NULL;
    UInt               sOperation;

    if( aOperIndex < QCI_AUDIT_OPER_MAX )
    {
        sSearch.userID = aUserID;

        /* BUG-38204 */
        sCondition = getUserCondition( &sSearch, &mMetaInfo );
        if( sCondition != NULL )
        {
            sOperation = sCondition->operation[aOperIndex];
     
            /* in case of success */
            if( ((sOperation & QCI_AUDIT_LOGGING_SESSION_SUCCESS_MASK)
                 == QCI_AUDIT_LOGGING_SESSION_SUCCESS_TRUE)
                && ( aStat->mExecuteSuccessCount > 0) )
            {   
                sIsWriteLog = ID_TRUE;
            }
           
            /* in case of failure */
            if( ((sOperation & QCI_AUDIT_LOGGING_SESSION_FAILURE_MASK)
                 == QCI_AUDIT_LOGGING_SESSION_FAILURE_TRUE)
                && ( aStat->mExecuteFailureCount > 0) )
            {
                sIsWriteLog = ID_TRUE;
            }
        }
    }

    return sIsWriteLog;
}

/* BUG-39311 Prioritization of audit conditions */
idBool mmtAuditManager::isAuditWriteAllBySession( UInt          aOperIndex,
                                                  idvSQL       *aStat )
{
    idBool             sIsWriteLog = ID_FALSE;  /* set default value, ID_FALSE */
    qciAuditOperation *sCondition = NULL;
    UInt               sOperation;

    if( aOperIndex < QCI_AUDIT_OPER_MAX )
    {
        sCondition = mMetaInfo.mAllUserCondition;

        if( sCondition != NULL )
        {
            sOperation = sCondition->operation[aOperIndex];
     
            /* in case of success */
            if( ((sOperation & QCI_AUDIT_LOGGING_SESSION_SUCCESS_MASK)
                 == QCI_AUDIT_LOGGING_SESSION_SUCCESS_TRUE)
                && ( aStat->mExecuteSuccessCount > 0) )
            {   
                sIsWriteLog = ID_TRUE;
            }
           
            /* in case of failure */
            if( ((sOperation & QCI_AUDIT_LOGGING_SESSION_FAILURE_MASK)
                 == QCI_AUDIT_LOGGING_SESSION_FAILURE_TRUE)
                && ( aStat->mExecuteFailureCount > 0) )
            {
                sIsWriteLog = ID_TRUE;
            }
        }
    }

    return sIsWriteLog;
}

/* BUG-39311 Prioritization of audit conditions */
idBool mmtAuditManager::isAuditWriteAllByAccess( UInt               aOperIndex,
                                                 mmcExecutionFlag   aExecutionFlag )
{
    idBool             sIsWriteLog = ID_FALSE;  /* set default value, ID_FALSE */
    qciAuditOperation *sCondition = NULL;
    UInt               sOperation;

    if( aOperIndex < QCI_AUDIT_OPER_MAX )
    {
        sCondition = mMetaInfo.mAllUserCondition;

        if( sCondition != NULL )
        {
            sOperation = sCondition->operation[aOperIndex];
            
            if( aExecutionFlag == MMC_EXECUTION_FLAG_SUCCESS )
            {
                /* in case of success */
                if( (sOperation & QCI_AUDIT_LOGGING_ACCESS_SUCCESS_MASK)
                    == QCI_AUDIT_LOGGING_ACCESS_SUCCESS_TRUE )
                {   
                    sIsWriteLog = ID_TRUE;
                }
            }
            else if( aExecutionFlag == MMC_EXECUTION_FLAG_FAILURE )
            {
                /* in case of failure */
                if( (sOperation & QCI_AUDIT_LOGGING_ACCESS_FAILURE_MASK)
                    == QCI_AUDIT_LOGGING_ACCESS_FAILURE_TRUE )
                {
                    sIsWriteLog = ID_TRUE;
                }
            }
            else
            {
                /* not logging */
            }
        }
    }
    
    return sIsWriteLog;
}

void mmtAuditManager::auditByAccess( mmcStatement     *aStatement,
                                     mmcExecutionFlag  aExecutionFlag )
{
    qciStatement      *sQciStmt;
    qciAuditRefObject *sAuditObjects;
    UInt               sAuditObjectCount;
    idvAuditTrail     *sAuditTrail;
    idvSQL            *sStat;
    mmcSessionInfo    *sSessInfo;
    SChar             *sQueryString;
    UInt               sQueryLen;
    UShort             sResultSetCount;
    mmcFetchFlag       sFetchFlag;
    idBool             sIsWriteLog = ID_FALSE;
    idBool             sNeedUnlock = ID_FALSE;
    UInt               sIdx;

    IDE_TEST_CONT( isAuditStarted() != ID_TRUE, AUDIT_NOT_STARTED );

    sQciStmt          = aStatement->getQciStmt();
    sStat             = aStatement->getStatistics();
    sSessInfo         = aStatement->getSession()->getInfo();
    sQueryString      = aStatement->getQueryString();
    sQueryLen         = aStatement->getQueryLen();
    sResultSetCount   = aStatement->getResultSetCount();

    lockAuditMetaRead();
    sNeedUnlock = ID_TRUE;

    /* BUG-49194 [mm-altiaudit] INC-45592 방어코드 추가 
     * audit 관련 변수를 lock 이후에 가져온다 */
    sAuditObjects     = aStatement->getAuditObjects();
    sAuditObjectCount = aStatement->getAuditObjectCount();
    sAuditTrail       = aStatement->getAuditTrail();

    IDE_TEST_CONT( isAuditStarted() != ID_TRUE, AUDIT_NOT_STARTED );

    /* audit all - BUG-39311 Prioritization of audit conditions */
    sIsWriteLog = isAuditWriteAllByAccess( sAuditTrail->mActionCode, aExecutionFlag );

    /* user auditing */
    if ( sIsWriteLog == ID_FALSE )
    {
        sIsWriteLog = isAuditWriteUserByAccess( sSessInfo->mUserInfo.userID,
                                                sAuditTrail->mActionCode,
                                                aExecutionFlag );
    }

    /* object auditing */
    for( sIdx = 0;
         (sIdx < sAuditObjectCount) && (sIsWriteLog == ID_FALSE);
         sIdx++, sAuditObjects++ )
    {
        sIsWriteLog = isAuditWriteObjectByAccess( sAuditObjects,
                                                  sAuditTrail->mActionCode, 
                                                  aExecutionFlag );
    }

    unlockAuditMeta();
    sNeedUnlock = ID_FALSE;

    if ( sIsWriteLog == ID_TRUE )
    {
        if (sResultSetCount == 0)
        {
            sFetchFlag = MMC_FETCH_FLAG_NO_RESULTSET;
        }
        else
        {
            sFetchFlag = aStatement->getFetchFlag();
        }
            
        /* set transaction info */
        sAuditTrail->mExecutionFlag = (UInt)aExecutionFlag;
        sAuditTrail->mFetchFlag     = (UInt)sFetchFlag;
        sAuditTrail->mReturnCode    = aStatement->getExecuteResult( aExecutionFlag );

        /* set stat info */
        aStatement->setAuditTrailStatStmt( sAuditTrail, sQciStmt, aExecutionFlag, sStat );

        idvAudit::writeAuditEntries( sAuditTrail,
                                     sQueryString,
                                     sQueryLen );
    }

    IDE_EXCEPTION_CONT( AUDIT_NOT_STARTED );
    if ( sNeedUnlock == ID_TRUE )
    {
        unlockAuditMeta();
    }
}

void mmtAuditManager::auditBySession( mmcStatement *aStatement )
{
    qciAuditRefObject *sAuditObjects;
    UInt               sAuditObjectCount;
    idvAuditTrail     *sAuditTrail;
    idvSQL            *sStat;
    mmcSessionInfo    *sSessInfo;
    SChar             *sQueryString;
    UInt               sQueryLen;
    idBool             sIsWriteLog = ID_FALSE;
    idBool             sNeedUnlock = ID_FALSE;
    UInt               sIdx;

    UInt               sRefObjectCount = 0;

    IDE_TEST_CONT( isAuditStarted() != ID_TRUE, AUDIT_NOT_STARTED );

    sStat             = aStatement->getStatistics();
    sSessInfo         = aStatement->getSession()->getInfo();
    sQueryString      = aStatement->getQueryString();
    sQueryLen         = aStatement->getQueryLen();

    lockAuditMetaRead();
    sNeedUnlock = ID_TRUE;

    /* BUG-49194 [mm-altiaudit] INC-45592 방어코드 추가 
     * audit 관련 변수를 lock 이후에 가져온다 */
    sAuditObjects     = aStatement->getAuditObjects();
    sAuditObjectCount = aStatement->getAuditObjectCount();
    sAuditTrail       = aStatement->getAuditTrail();

    IDE_TEST_CONT( isAuditStarted() != ID_TRUE, AUDIT_NOT_STARTED );

    /* audit all - BUG-39311 Prioritization of audit conditions */
    sIsWriteLog = isAuditWriteAllBySession( sAuditTrail->mActionCode, sStat );

    /* user auditing */
    if ( sIsWriteLog == ID_FALSE )
    {
        sIsWriteLog = isAuditWriteUserBySession( sSessInfo->mUserInfo.userID,
                                                 sAuditTrail->mActionCode,
                                                 sStat );
    }

    /* BUG-49194 [mm-altiaudit] INC-45592 방어코드 추가 
     * auditObjectCount와 refObjectCount 를 비교하여 
     * 잘못된 sAuditObjects 메모리 접근을 방어한다. */
    qci::getAllRefObjectCount( aStatement->getQciStmt(), &sRefObjectCount );
    IDE_TEST_RAISE( sRefObjectCount != sAuditObjectCount, invalidAuditObjectCount );

    /* object auditing */
    for( sIdx = 0;
         (sIdx < sAuditObjectCount) && (sIsWriteLog == ID_FALSE);
         sIdx++, sAuditObjects++ )
    {
        sIsWriteLog = isAuditWriteObjectBySession( sAuditObjects,
                                                   sAuditTrail->mActionCode,
                                                   sStat );
    }

    unlockAuditMeta();
    sNeedUnlock = ID_FALSE;

    if ( sIsWriteLog == ID_TRUE )
    {
        /* set transaction info */
        sAuditTrail->mExecutionFlag = (UInt)MMC_EXECUTION_FLAG_SUCCESS;
        sAuditTrail->mFetchFlag     = (UInt)MMC_FETCH_FLAG_CLOSE;
        sAuditTrail->mReturnCode    = 0;

        /* set stat info */
        aStatement->setAuditTrailStatSess( sAuditTrail, sStat );
            
        idvAudit::writeAuditEntries( sAuditTrail,
                                     sQueryString,
                                     sQueryLen );
    }

    /* session 정보를 reset한다. */
    aStatement->resetStatistics();

    IDE_EXCEPTION_CONT( AUDIT_NOT_STARTED );
    if ( sNeedUnlock == ID_TRUE )
    {
        unlockAuditMeta();
    }

    return;

    IDE_EXCEPTION (invalidAuditObjectCount)
    {
        ideLog::log( IDE_ERR_0, "invalid auditObjectCount : auditObjectCount(%d) != sRefObjectCount(%d)", sAuditObjectCount, sRefObjectCount );
    }
    IDE_EXCEPTION_END;

    if ( sNeedUnlock == ID_TRUE )
    {
        unlockAuditMeta();
    }

    return;
}

/* BUG-41986 User connection info can be audited.
 * In order to be used by both mmt and mmt5, 
 * this function must be located here.
 */
void mmtAuditManager::initAuditConnInfo( mmcSession    *aSession, 
                                         idvAuditTrail *aAuditTrail,
                                         qciUserInfo   *aUserInfo,
                                         UInt           aResultCode,
                                         UInt           aActionCode )
{
    mmcSessionInfo *sSessInfo  = NULL;
    SChar          *sOperation = NULL;

    /* initialize an audit trail structure */
    idlOS::memset( aAuditTrail, 0x00, ID_SIZEOF(idvAuditTrail) );
    aAuditTrail->mActionCode = aActionCode;

    /* set user info */
    idlOS::snprintf( aAuditTrail->mLoginIP, 
                     IDV_MAX_IP_LEN, 
                     aUserInfo->loginIP );

    idlOS::snprintf( aAuditTrail->mLoginID, 
                     IDV_MAX_NAME_LEN, 
                     aUserInfo->loginID); 

    aAuditTrail->mReturnCode = aResultCode;

    if ( aAuditTrail->mReturnCode == 0 )
    {    
        /* success */
        aAuditTrail->mExecutionFlag       = (UInt)MMC_EXECUTION_FLAG_SUCCESS;
        aAuditTrail->mExecuteSuccessCount = 1; 

        sSessInfo = aSession->getInfo();

        /* set session info */
        aAuditTrail->mClientPID      = sSessInfo->mClientPID;
        aAuditTrail->mSessionID      = sSessInfo->mSessionID;
        aAuditTrail->mXaSessionFlag  = sSessInfo->mXaSessionFlag;
        aAuditTrail->mAutoCommitFlag = aSession->getCommitMode();

        idlOS::snprintf( aAuditTrail->mClientType,
                         IDV_MAX_CLIENT_TYPE_LEN,
                         "%s",
                         sSessInfo->mClientType );

        idlOS::snprintf( aAuditTrail->mClientAppInfo, 
                         IDV_MAX_CLIENT_APP_LEN, 
                         "%s",
                         sSessInfo->mClientAppInfo );
    }
    else
    {
        /* In case of failure, the connection doesn't have any session information
         * such as session id, client type, app info, and so on. */
        aAuditTrail->mExecutionFlag       = (UInt)MMC_EXECUTION_FLAG_FAILURE;
        aAuditTrail->mExecuteFailureCount = 1;
    }

    aAuditTrail->mFetchFlag = (UInt)MMC_FETCH_FLAG_NO_RESULTSET;

    /* set the operation */
    qci::getAuditOperationByOperID( (qciAuditOperIdx)aActionCode,
                                    (const SChar **)&sOperation );

    if ( sOperation != NULL )
    {
        idlOS::snprintf( aAuditTrail->mAction, 
                         IDV_MAX_ACTION_LEN,
                         "%s",
                         sOperation );
    }
    else
    {
        aAuditTrail->mActionCode = QCI_AUDIT_OPER_MAX;
    }

    return;
}


void mmtAuditManager::auditConnectInfo( idvAuditTrail  *aAuditTrail )
{
    idBool             sIsWriteLog = ID_FALSE;
    idBool             sNeedUnlock = ID_FALSE;
    UInt               sUserID     = 0;

    IDE_TEST_CONT( isAuditStarted() != ID_TRUE, AUDIT_NOT_STARTED );

    lockAuditMetaRead();
    sNeedUnlock = ID_TRUE;
    IDE_TEST_CONT( isAuditStarted() != ID_TRUE, AUDIT_NOT_STARTED );

    /* audit all users */
    sIsWriteLog = isAuditWriteAllByAccess( aAuditTrail->mActionCode, 
                                           (mmcExecutionFlag)aAuditTrail->mExecutionFlag );

    /* audit a specific user */
    if ( sIsWriteLog == ID_FALSE )
    {   
        if ( qciMisc::getUserIdByName( aAuditTrail->mLoginID, &sUserID ) == IDE_SUCCESS )
        {
            sIsWriteLog = isAuditWriteUserByAccess( sUserID,
                                                    aAuditTrail->mActionCode, 
                                                    (mmcExecutionFlag)aAuditTrail->mExecutionFlag );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }

    unlockAuditMeta();
    sNeedUnlock = ID_FALSE;

    IDE_TEST_CONT( sIsWriteLog != ID_TRUE, NO_NEED_TO_LOG );

    idvAudit::writeAuditEntries( aAuditTrail,                                 /* audit info */
                                 aAuditTrail->mAction,                        /* query      */
                                 (UInt)idlOS::strlen(aAuditTrail->mAction) ); /* query len  */

    IDE_EXCEPTION_CONT( NO_NEED_TO_LOG );

    IDE_EXCEPTION_CONT( AUDIT_NOT_STARTED );

    if ( sNeedUnlock == ID_TRUE )
    {
        unlockAuditMeta();
    }
    else
    {
        /* do nothing */
    }

    return;
}

void mmtAuditManager::setAuditStarted( idBool aStatus )
{
    mIsAuditStarted = aStatus;

    IDL_MEM_BARRIER;
}

inline static void clearAuditMetaInfo( mmtAuditMetaInfo *aMetaInfo )
{
    if ( aMetaInfo->mObjectOperations != NULL )
    {
        (void) iduMemMgr::free( aMetaInfo->mObjectOperations );
        aMetaInfo->mObjectOperations = NULL;
    }
    aMetaInfo->mObjectOperationCount = 0;

    if ( aMetaInfo->mUserOperations != NULL )
    {
        (void) iduMemMgr::free( aMetaInfo->mUserOperations );
        aMetaInfo->mUserOperations = NULL;
    }
    aMetaInfo->mUserOperationCount = 0;

    aMetaInfo->mAllUserCondition = NULL;
}

/* mmtAuditManager::reloadAuditCondCallback()
 * This reloads and updates audit conditions from the meta table SYSTEM_.SYS_AUDIT_OPTS_.  
 */
void mmtAuditManager::reloadAuditCondCallback( qciAuditOperation *aObjectOperations,
                                               UInt               aObjectOperationCount,
                                               qciAuditOperation *aUserOperations,
                                               UInt               aUserOperationCount )
{
    lockAuditMetaWrite();

    clearAuditMetaInfo( &mMetaInfo );

    mMetaInfo.mObjectOperations     = aObjectOperations;
    mMetaInfo.mObjectOperationCount = aObjectOperationCount;
    mMetaInfo.mUserOperations       = aUserOperations;
    mMetaInfo.mUserOperationCount   = aUserOperationCount;
    
    /* BUG-39311 Prioritization of audit conditions */ 
    mMetaInfo.mAllUserCondition = getAllUserConditionFromMeta( &mMetaInfo );

    setAuditStarted( ID_TRUE );

    unlockAuditMeta();
}

void mmtAuditManager::stopAuditCallback()
{
    setAuditStarted( ID_FALSE );

    lockAuditMetaWrite();

    clearAuditMetaInfo( &mMetaInfo );

    unlockAuditMeta();

    (void)idvAudit::flushAllBufferToFile();
}


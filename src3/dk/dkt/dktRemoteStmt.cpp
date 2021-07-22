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
 * $id$
 **********************************************************************/

#include <dktRemoteStmt.h>
#include <dkErrorCode.h>
#include <dkdMisc.h>

/************************************************************************
 * Description : 이 remote statement 객체를 type 에 따라 초기화해준다.
 *
 *  aGTxId      - [IN] 이 remote statement 가 속한 global transaction 의 
 *                     id
 *  aRTxId      - [IN] 이 remote statement 가 속한 remote transaction 의
 *                     id
 *  aId         - [IN] Remote statement 의 id
 *  aStmtType   - [IN] Remote statement 의 type 으로 다음중 하나의 값을 
 *                     갖는다.
 *  
 *            ++ Remote Statement Types:
 *
 *              0. DKT_STMT_TYPE_REMOTE_TABLE
 *              1. DKT_STMT_TYPE_REMOTE_EXECUTE_IMMEDIATE
 *              2. DKT_STMT_TYPE_REMOTE_EXECUTE_QUERY_STATEMENT
 *              3. DKT_STMT_TYPE_REMOTE_EXECUTE_NON_QUERY_STATEMENT
 *              4. DKT_STMT_TYPE_REMOTE_TABLE_STORE
 *
 *  aStmtStr    - [IN] 원격서버에서 수행될 구문
 *
 ************************************************************************/
IDE_RC  dktRemoteStmt::initialize( UInt    aGlobalTxId,
                                   UInt     aLocalTxId,
                                   UInt     aRTxId,
                                   SLong    aId,
                                   UInt     aStmtType,
                                   SChar   *aStmtStr )
{
    UInt    sStmtLen = 0;

    mId             = aId;
    mStmtType       = aStmtType;
    mResult         = DKP_RC_FAILED;
    mDataMgr        = NULL;
    mRemoteTableMgr = NULL;
    mBuffSize       = 0;
    mBatchStatementMgr = NULL;
    mGlobalTxId     = aGlobalTxId;
    mLocalTxId      = aLocalTxId;
    mRTxId          = aRTxId;
    mStmtStr        = NULL;

    dkoLinkObjMgr::initCopiedLinkObject( &mExecutedLinkObj );

    switch ( aStmtType )
    {
        case DKT_STMT_TYPE_REMOTE_TABLE_STORE:
        case DKT_STMT_TYPE_REMOTE_EXECUTE_QUERY_STATEMENT:
            IDE_TEST( createDataManager() != IDE_SUCCESS );
            break;

        case DKT_STMT_TYPE_REMOTE_TABLE:
#ifdef ALTIBASE_PRODUCT_XDB
            IDE_TEST( createDataManager() != IDE_SUCCESS );
#else
            IDE_TEST( createRemoteTableManager() != IDE_SUCCESS );
#endif
            break;

        default:
            /* nothing to do */
            break;
    }

    idlOS::memset( &mErrorInfo, 0, ID_SIZEOF( dktErrorInfo ) );
    
    sStmtLen = idlOS::strlen( aStmtStr );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       sStmtLen + 1,
                                       (void **)&mStmtStr,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_STMT_STR );

    idlOS::strcpy( mStmtStr, aStmtStr );

    IDE_TEST( dkpBatchStatementMgrCreate( &mBatchStatementMgr ) != IDE_SUCCESS );
    mBatchExecutionResult = NULL;
    mBatchExecutionResultSize = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_STMT_STR );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( mBatchStatementMgr != NULL )
    {
        dkpBatchStatementMgrDestroy( mBatchStatementMgr );
        mBatchStatementMgr = NULL;
    }
    else
    {
        /* nothing to do */
    }
    
    if ( mRemoteTableMgr != NULL )
    {    
        destroyRemoteTableManager();
    }
    else
    {
        if ( mDataMgr != NULL )
        {
            destroyDataManager();
        }
        else
        {
            /* there is no data manager created */
        }
    }

    if ( mStmtStr != NULL )
    {
        (void)iduMemMgr::free( mStmtStr );
        mStmtStr = NULL;
    }
    else
    {
        /* do nothing */
    }
    
    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 이 remote statement 객체에 동적 할당된 자원을 해제해주고 
 *               멤버들을 초기화해준다. 내부적으로 clean() 을 호출한다.
 *
 *  BUG-37487 : return 값을 IDE_RC --> void 로 변경.
 *
 ************************************************************************/
void dktRemoteStmt::finalize()
{
    clean();
}

/************************************************************************
 * Description : 동적 할당된 자원을 해제해주고 멤버들을 초기화해준다.
 *
 *  BUG-37487 : return 값을 IDE_RC --> void 로 변경.
 *
 ************************************************************************/
void dktRemoteStmt::clean() 
{
    mId        = DK_INVALID_STMT_ID;
    mStmtType  = DK_INVALID_STMT_TYPE;
    mResult    = DKP_RC_FAILED;

    dkoLinkObjMgr::initCopiedLinkObject( &mExecutedLinkObj );

    if ( mRemoteTableMgr != NULL )
    {
        destroyRemoteTableManager();
    }
    else
    {
        if ( mDataMgr != NULL )
        {
            destroyDataManager();
        }
        else
        {
            /* do nothing */
        }
    }

    if ( mStmtStr != NULL )
    {
        (void)iduMemMgr::free( mStmtStr );
        mStmtStr = NULL;
    }
    else
    {
        /* do nothing */
    }

    mErrorInfo.mErrorCode = 0;

    (void)idlOS::memset( mErrorInfo.mSQLState, 
                         0, 
                         ID_SIZEOF( mErrorInfo.mSQLState ) );

    if ( mErrorInfo.mErrorDesc != NULL )
    {
        (void)iduMemMgr::free( mErrorInfo.mErrorDesc );
        mErrorInfo.mErrorDesc = NULL;
    }
    else
    {
        /* do nothing */
    }

    if ( mBatchExecutionResult != NULL )
    {
        (void)iduMemMgr::free( mBatchExecutionResult );
        mBatchExecutionResult = NULL;
        mBatchExecutionResultSize = 0;
    }
    else
    {
        /* nothing to do */
    }
     
    if ( mBatchStatementMgr != NULL )
    {
        dkpBatchStatementMgrDestroy( mBatchStatementMgr );
        mBatchStatementMgr = NULL;
    }
    else
    {
        /* nothing to do */
    }
}

/************************************************************************
 * Description : Remote statement 를 수행하기 위한 protocol operation 
 *               을 수행한다. 
 *
 *  aSession    - [IN] AltiLinker process 와의 연결을 위한 정보를 갖고 
 *                     있는 dksSession 구조체
 *  aSessionId  - [IN] 이 protocol operation 을 수행하려는 세션의 id
 *  aLinkObj    - [IN] 이 세션이 사용하는 database link 객체의 정보
 *  aRemoteSessionId - [IN] database link 를 통해 연결하려는 remote 
 *                          session 의 id
 *
 ************************************************************************/
IDE_RC  dktRemoteStmt::prepareProtocol( dksSession   * aSession, 
                                        UInt           aSessionId,
                                        idBool         aIsTxBegin,
                                        ID_XID       * aXID,
                                        dkoLink      * aLinkObj,
                                        UShort       * aRemoteSessionId )
{
    UInt            sResultCode = DKP_RC_SUCCESS;
    UInt            sTimeoutSec;
    UShort          sRemoteSessionId = DK_INIT_REMOTE_SESSION_ID;
    SInt            sReceiveTimeout = 0;
    SInt            sRemoteNodeRecvTimeout = 0;
    
    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerReceiveTimeoutFromConf( &sReceiveTimeout )
              != IDE_SUCCESS );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerRemoteNodeReceiveTimeoutFromConf( &sRemoteNodeRecvTimeout )
              != IDE_SUCCESS );
    
    sTimeoutSec = sReceiveTimeout + sRemoteNodeRecvTimeout;

    IDE_TEST( dkpProtocolMgr::sendPrepareRemoteStmt( aSession,
                                                     aSessionId,
                                                     aIsTxBegin,
                                                     aXID,
                                                     aLinkObj,
                                                     mId,
                                                     mStmtStr )
              != IDE_SUCCESS );

    IDE_TEST( dkpProtocolMgr::recvPrepareRemoteStmtResult( aSession,
                                                           aSessionId,
                                                           mId,
                                                           &sResultCode,
                                                           &sRemoteSessionId,
                                                           sTimeoutSec )
              != IDE_SUCCESS );

    /* Analyze result code */
    IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS, ERR_RECEIVE_RESULT );

    *aRemoteSessionId = sRemoteSessionId;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            aSession,
                                            aSessionId,
                                            mId,
                                            &mErrorInfo );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC  dktRemoteStmt::prepareBatchProtocol( dksSession    *aSession, 
                                             UInt           aSessionId,
                                             idBool         aIsTxBegin,
                                             ID_XID        *aXID,
                                             dkoLink       *aLinkObj,
                                             UShort        *aRemoteSessionId )
{
    UInt            sResultCode = DKP_RC_SUCCESS;
    UInt            sTimeoutSec = 0;
    UShort          sRemoteSessionId = DK_INVALID_REMOTE_SESSION_ID;
    SInt            sReceiveTimeout = 0;
    SInt            sRemoteNodeRecvTimeout = 0;
    
    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerReceiveTimeoutFromConf( &sReceiveTimeout )
              != IDE_SUCCESS );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerRemoteNodeReceiveTimeoutFromConf( &sRemoteNodeRecvTimeout )
              != IDE_SUCCESS );
    
    sTimeoutSec = sReceiveTimeout + sRemoteNodeRecvTimeout;

    IDE_TEST( dkpProtocolMgr::sendPrepareRemoteStmtBatch( aSession,
                                                          aSessionId,
                                                          aIsTxBegin,
                                                          aXID,
                                                          aLinkObj,
                                                          mId,
                                                          mStmtStr )
              != IDE_SUCCESS );

    IDE_TEST( dkpProtocolMgr::recvPrepareRemoteStmtBatchResult( aSession,
                                                                aSessionId,
                                                                mId,
                                                                &sResultCode,
                                                                &sRemoteSessionId,
                                                                sTimeoutSec )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS, ERR_RECEIVE_RESULT );
    
    *aRemoteSessionId = sRemoteSessionId;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            aSession,
                                            aSessionId,
                                            mId,
                                            &mErrorInfo );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC  dktRemoteStmt::prepare( dksSession    *aSession, 
                                UInt           aSessionId,
                                idBool         aIsTxBegin,
                                ID_XID        *aXID,
                                dkoLink       *aLinkObj,
                                UShort        *aRemoteSessionId )
{
    switch ( mStmtType )
    {
        case DKT_STMT_TYPE_REMOTE_TABLE:
        case DKT_STMT_TYPE_REMOTE_TABLE_STORE:
        case DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT:
            IDE_TEST( prepareProtocol( aSession,
                                       aSessionId,
                                       aIsTxBegin,
                                       aXID,
                                       aLinkObj,
                                       aRemoteSessionId )
                      != IDE_SUCCESS );
            break;
            
        case DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT_BATCH:
            IDE_TEST( prepareBatchProtocol( aSession,
                                            aSessionId,
                                            aIsTxBegin,
                                            aXID,
                                            aLinkObj,
                                            aRemoteSessionId )
                      != IDE_SUCCESS );            
            break;

        case DKT_STMT_TYPE_REMOTE_EXECUTE_IMMEDIATE:
        case DKT_STMT_TYPE_REMOTE_EXECUTE_QUERY_STATEMENT:
        case DKT_STMT_TYPE_REMOTE_EXECUTE_NON_QUERY_STATEMENT:
        default:
            IDE_RAISE( ERR_INVALID_REMOTE_STMT_TYPE );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT_TYPE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKT_INVALID_REMOTE_STMT_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Result set 의 meta 를 얻어오기 위한 protocol operation 
 *               을 수행한다. 
 *
 *  aSession    - [IN] AltiLinker process 와의 연결을 위한 정보를 갖고 
 *                     있는 dksSession 구조체
 *  aSessionId  - [IN] 이 protocol operation 을 수행하려는 세션의 id
 *
 ************************************************************************/
IDE_RC  dktRemoteStmt::requestResultSetMetaProtocol( dksSession *aSession, 
                                                     UInt        aSessionId )
{
    UInt            sColCnt     = 0;
    UInt            sResultCode = DKP_RC_SUCCESS;
    ULong           sTimeoutSec;
    dkpColumn      *sColMetaArr = NULL;
    SInt            sReceiveTimeout = 0;
    SInt            sRemoteNodeRecvTimeout = 0;
    
    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerReceiveTimeoutFromConf( &sReceiveTimeout )
              != IDE_SUCCESS );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerRemoteNodeReceiveTimeoutFromConf( &sRemoteNodeRecvTimeout )
              != IDE_SUCCESS );
    
    sTimeoutSec = sReceiveTimeout + sRemoteNodeRecvTimeout;
                 
    IDE_TEST( dkpProtocolMgr::sendRequestResultSetMeta( aSession,
                                                        aSessionId,
                                                        mId )
              != IDE_SUCCESS );

    IDE_TEST( dkpProtocolMgr::recvRequestResultSetMetaResult( aSession, 
                                                              aSessionId, 
                                                              mId, 
                                                              &sResultCode, 
                                                              &sColCnt,
                                                              &sColMetaArr,
                                                              sTimeoutSec )
              != IDE_SUCCESS );
     
    /* Analyze result code */
    IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS, ERR_RECEIVE_RESULT );
    
    /* Validate result column meta using DK converter */
    IDE_TEST( createTypeConverter( sColMetaArr, sColCnt ) != IDE_SUCCESS );

    if ( sColMetaArr != NULL )
    {
        (void)iduMemMgr::free( sColMetaArr );
        sColMetaArr = NULL;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            aSession,
                                            aSessionId,
                                            mId,
                                            &mErrorInfo );
    }
    IDE_EXCEPTION_END;

    /* >> BUG-37663 */
    IDE_PUSH();

    if ( sColMetaArr != NULL )
    {
        (void)iduMemMgr::free( sColMetaArr );
        sColMetaArr = NULL;
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();
    /* << BUG-37663 */

    return IDE_FAILURE;

}

/************************************************************************
 * Description : requestResultSetMetaProtocol 에서 원격서버로부터 가져온
 *               result set meta 를 이용해 type converter 를 생성한다.
 *
 *  aColMetaArr - [IN] 원격서버로부터 가져온 result set meta arrary
 *  aColCount   - [IN] result set 의 컬럼개수 
 *
 ************************************************************************/
IDE_RC  dktRemoteStmt::createTypeConverter( dkpColumn *aColMetaArr,
                                            UInt       aColCount )
{
    switch( mStmtType )
    {
        case DKT_STMT_TYPE_REMOTE_TABLE:
#ifdef ALTIBASE_PRODUCT_XDB
            IDE_TEST( mDataMgr->createTypeConverter( aColMetaArr, aColCount )
                      != IDE_SUCCESS );
#else
            IDE_TEST( mRemoteTableMgr->createTypeConverter( aColMetaArr, aColCount )
                      != IDE_SUCCESS );
#endif
            break;

        case DKT_STMT_TYPE_REMOTE_EXECUTE_QUERY_STATEMENT:
        case DKT_STMT_TYPE_REMOTE_TABLE_STORE:
            IDE_TEST( mDataMgr->createTypeConverter( aColMetaArr, aColCount ) 
                      != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_INVALID_REMOTE_STMT_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT_TYPE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKT_INVALID_REMOTE_STMT_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : type converter 를 반환한다.
 *
 ************************************************************************/
IDE_RC dktRemoteStmt::getTypeConverter( dkdTypeConverter ** aTypeConverter )
{
    dkdTypeConverter    *sTypeConverter = NULL;

    switch( mStmtType )
    {
        case DKT_STMT_TYPE_REMOTE_TABLE:
#ifdef ALTIBASE_PRODUCT_XDB
            sTypeConverter = mDataMgr->getTypeConverter();
#else
            sTypeConverter = mRemoteTableMgr->getTypeConverter();
#endif
            break;

        case DKT_STMT_TYPE_REMOTE_EXECUTE_QUERY_STATEMENT:
        case DKT_STMT_TYPE_REMOTE_TABLE_STORE:

            sTypeConverter = mDataMgr->getTypeConverter();
            break;

        default:
            break;
    }

    IDE_TEST_RAISE( sTypeConverter == NULL, ERR_GET_TYPE_CONVERTER_NULL );
    *aTypeConverter = sTypeConverter;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_GET_TYPE_CONVERTER_NULL )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR, 
                                  "[getTypeConverter] sTypeConverter is NULL" ) ); 
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : type converter 를 제거한다.
 *
 ************************************************************************/
IDE_RC  dktRemoteStmt::destroyTypeConverter()
{
    switch( mStmtType )
    {
        case DKT_STMT_TYPE_REMOTE_TABLE:
#ifdef ALTIBASE_PRODUCT_XDB
            IDE_TEST( mDataMgr->destroyTypeConverter() != IDE_SUCCESS );
#else
            IDE_TEST( mRemoteTableMgr->destroyTypeConverter() != IDE_SUCCESS );
#endif
            break;

        case DKT_STMT_TYPE_REMOTE_EXECUTE_QUERY_STATEMENT:
        case DKT_STMT_TYPE_REMOTE_TABLE_STORE:

            IDE_TEST( mDataMgr->destroyTypeConverter() != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_INVALID_REMOTE_STMT_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT_TYPE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKT_INVALID_REMOTE_STMT_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote statement 를 수행하기 위한 protocol operation 
 *               을 수행한다. 
 *
 *  aSession    - [IN] AltiLinker process 와의 연결을 위한 정보를 갖고 
 *                     있는 dksSession 구조체
 *  aSessionId  - [IN] 이 protocol operation 을 수행하려는 세션의 id
 *  aLinkObj    - [IN] 이 세션이 사용하는 database link 객체의 정보
 *  aResult     - [IN] Protocol operation 수행의 결과
 *
 ************************************************************************/
IDE_RC  dktRemoteStmt::executeProtocol( dksSession    *aSession,
                                        UInt           aSessionId,
                                        idBool         aIsTxBegin,
                                        ID_XID        *aXID,
                                        dkoLink       *aLinkObj,
                                        SInt          *aResult )
{
    SInt            sResult;
    UInt            sStmtType;
    UInt            sSendStmtType;
    UInt            sResultCode = DKP_RC_SUCCESS;
    ULong           sTimeoutSec;
    UShort          sRemoteSessionId;
    SInt            sReceiveTimeout = 0;
    SInt            sQueryTimeout = 0;
    
    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerReceiveTimeoutFromConf( &sReceiveTimeout )
              != IDE_SUCCESS );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerQueryTimeoutFromConf( &sQueryTimeout )
              != IDE_SUCCESS );
    
    sTimeoutSec = sReceiveTimeout + sQueryTimeout;
                 
    if ( mStmtType == DKT_STMT_TYPE_REMOTE_TABLE_STORE )
    {
        sSendStmtType = DKT_STMT_TYPE_REMOTE_TABLE;
    }
    else
    {
        sSendStmtType = mStmtType;
    }

    IDE_TEST( dkpProtocolMgr::sendExecuteRemoteStmt( aSession,
                                                     aSessionId,
                                                     aIsTxBegin,
                                                     aXID,
                                                     aLinkObj,
                                                     mId,
                                                     sSendStmtType,
                                                     mStmtStr )
              != IDE_SUCCESS );

    IDE_TEST( dkpProtocolMgr::recvExecuteRemoteStmtResult( aSession,
                                                           aSessionId,
                                                           mId,
                                                           &sResultCode,
                                                           &sRemoteSessionId,
                                                           &sStmtType,
                                                           &sResult,
                                                           sTimeoutSec )
              != IDE_SUCCESS );

    if ( ( mStmtType != DKT_STMT_TYPE_REMOTE_TABLE ) &&
         ( mStmtType != DKT_STMT_TYPE_REMOTE_TABLE_STORE ) )
    {
        mStmtType = sStmtType;
    }
    else
    {
        /* DKT_STMT_TYPE_REMOTE_TABLE / DKT_STMT_TYPE_REMOTE_TABLE_STORE */
    }

    mResult = sResultCode;

    switch ( sResultCode )
    {
        case DKP_RC_SUCCESS:
            break;
            
        case DKP_RC_FAILED_STMT_TYPE_ERROR:
            /* Anyway, write trace log */
            IDE_SET( ideSetErrorCode( dkERR_ABORT_DKT_INVALID_STMT_TYPE ) );            
            break;     
           
        default:
            IDE_RAISE( ERR_RECEIVE_RESULT );
            break;
    }

    if ( mStmtType == DKT_STMT_TYPE_REMOTE_TABLE )
    {
        if ( aLinkObj != &mExecutedLinkObj )
        {
            dkoLinkObjMgr::copyLinkObject( aLinkObj, &mExecutedLinkObj );
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

    *aResult = sResult;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            aSession,
                                            aSessionId,
                                            mId,
                                            &mErrorInfo );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 *
 */
IDE_RC dktRemoteStmt::addBatch( void )
{
    IDE_TEST( dkpBatchStatementMgrAddBatchStatement( mBatchStatementMgr, mId ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dktRemoteStmt::prepareAddBatch( dksSession *aSession,
                                       UInt aSessionId,
                                       ULong aTimeoutSec )
{
    UInt sResultCode = DKP_RC_SUCCESS;

    IDE_TEST( dkpProtocolMgr::sendPrepareAddBatch( aSession,
                                                   aSessionId,
                                                   mId,
                                                   mBatchStatementMgr )
              != IDE_SUCCESS );

    IDE_TEST( dkpProtocolMgr::recvPrepareAddBatch( aSession,
                                                   aSessionId,
                                                   mId,
                                                   &sResultCode,
                                                   aTimeoutSec )
              != IDE_SUCCESS );

    mResult = sResultCode;

    IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS, ERR_RECEIVE_RESULT );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            aSession,
                                            aSessionId,
                                            mId,
                                            &mErrorInfo );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dktRemoteStmt::addBatches( dksSession *aSession,
                                  UInt aSessionId,
                                  ULong aTimeoutSec )
{
    UInt sResultCode = DKP_RC_SUCCESS;
    UInt sProcessedBatchCount = 0;

    IDE_TEST( dkpProtocolMgr::sendAddBatches( aSession,
                                              aSessionId,
                                              mBatchStatementMgr )
              != IDE_SUCCESS );

    IDE_TEST( dkpProtocolMgr::recvAddBatchResult( aSession,
                                                  aSessionId,
                                                  mId,
                                                  &sResultCode,
                                                  &sProcessedBatchCount,
                                                  aTimeoutSec )
              != IDE_SUCCESS );

    mResult = sResultCode;

    IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS, ERR_RECEIVE_RESULT );

    if ( mBatchExecutionResult != NULL )
    {
        (void)iduMemMgr::free( mBatchExecutionResult );
        mBatchExecutionResult = NULL;
        mBatchExecutionResultSize = 0;
    }
    else
    {
        /* nothing to do */
    }
    
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       sProcessedBatchCount * ID_SIZEOF( SInt ),
                                       (void **)&mBatchExecutionResult,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );
    mBatchExecutionResultSize = sProcessedBatchCount;    
    

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            aSession,
                                            aSessionId,
                                            mId,
                                            &mErrorInfo );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    if ( mBatchExecutionResult != NULL )
    {
        (void)iduMemMgr::free( mBatchExecutionResult );
        mBatchExecutionResult = NULL;
    }
    else
    {
        /* do nothing */
    }
    
    return IDE_FAILURE;    
}

/*
 *
 */
IDE_RC dktRemoteStmt::executeBatch( dksSession *aSession,
                                    UInt aSessionId,
                                    ULong aTimeoutSec )
{
    UInt sResultCode = DKP_RC_SUCCESS;
    
    IDE_TEST( dkpProtocolMgr::sendExecuteBatch( aSession,
                                                aSessionId,
                                                mId )
              != IDE_SUCCESS );

    IDE_TEST( dkpProtocolMgr::recvExecuteBatch( aSession,
                                                aSessionId,
                                                mId,
                                                mBatchExecutionResult,
                                                &sResultCode,
                                                aTimeoutSec )
              != IDE_SUCCESS );

    /* TODO: Is need to check batch count ? */

    mResult = sResultCode;

    IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS, ERR_RECEIVE_RESULT );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            aSession,
                                            aSessionId,
                                            mId,
                                            &mErrorInfo );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/*
 *
 */
IDE_RC dktRemoteStmt::executeBatchProtocol( dksSession    *aSession, 
                                            UInt           aSessionId, 
                                            SInt          *aResult )
{
    ULong           sTimeoutSec = 0;
    SInt            sReceiveTimeout = 0;
    SInt            sQueryTimeout = 0;
    
    IDE_DASSERT( mStmtType == DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT_BATCH );
        
    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerReceiveTimeoutFromConf( &sReceiveTimeout )
              != IDE_SUCCESS );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerQueryTimeoutFromConf( &sQueryTimeout )
              != IDE_SUCCESS );
    
    sTimeoutSec = sReceiveTimeout + sQueryTimeout;

    if ( dkpBatchStatementMgrCountBatchStatement( mBatchStatementMgr ) > 0 )
    {
        IDE_TEST( prepareAddBatch( aSession, aSessionId, sTimeoutSec )
                  != IDE_SUCCESS );

        IDE_TEST( addBatches( aSession, aSessionId, sTimeoutSec )
                  != IDE_SUCCESS );

        IDE_TEST( dkpBatchStatementMgrClear( mBatchStatementMgr ) != IDE_SUCCESS );
        
        IDE_TEST( executeBatch( aSession, aSessionId, sTimeoutSec )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */        
        mResult = DKP_RC_SUCCESS;
    }
    
    *aResult = mResult;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote statement 를 abort 하기위한  protocol operation 
 *               을 수행한다.
 *
 *  aSession    - [IN] AltiLinker process 와의 연결을 위한 정보를 갖고 
 *                     있는 dksSession 구조체
 *  aSessionId  - [IN] 이 protocol operation 을 수행하려는 세션의 id
 *
 ************************************************************************/
IDE_RC  dktRemoteStmt::abortProtocol( dksSession    *aSession, 
                                      UInt           aSessionId )
{
    UInt            sResultCode = DKP_RC_SUCCESS;
    ULong           sTimeoutSec;
    SInt            sReceiveTimeout = 0;
    SInt            sRemoteNodeRecvTimeout = 0;
    
    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerReceiveTimeoutFromConf( &sReceiveTimeout )
              != IDE_SUCCESS );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerRemoteNodeReceiveTimeoutFromConf( &sRemoteNodeRecvTimeout )
              != IDE_SUCCESS );
    
    sTimeoutSec = sReceiveTimeout + sRemoteNodeRecvTimeout;
                  
    IDE_TEST( dkpProtocolMgr::sendAbortRemoteStmt( aSession, 
                                                   aSessionId, 
                                                   mId )
              != IDE_SUCCESS );

    IDE_TEST( dkpProtocolMgr::recvAbortRemoteStmtResult( aSession, 
                                                         aSessionId, 
                                                         mId, 
                                                         &sResultCode, 
                                                         sTimeoutSec )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS, ERR_RECEIVE_RESULT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            aSession,
                                            aSessionId,
                                            mId,
                                            &mErrorInfo );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote statement 에 binding 을 수행하기 위해 bind 
 *               protocol operation 을 수행한다. 
 *
 *  aSession     - [IN] AltiLinker process 와의 연결을 위한 정보를 갖고 
 *                     있는 dksSession 구조체
 *  aSessionId   - [IN] 이 protocol operation 을 수행하려는 세션의 id
 *  aBindVarIdx  - [IN] Bind 할 변수의 위치에 따라 부여되는 index
 *  aBindVarType - [IN] Bind 할 변수의 타입
 *  aBindVarLen  - [IN] Bind 할 변수의 길이 
 *  aBindVar     - [IN] Bind 할 변수값
 *
 ************************************************************************/
IDE_RC  dktRemoteStmt::bindProtocol( dksSession    *aSession, 
                                     UInt           aSessionId, 
                                     UInt           aBindVarIdx, 
                                     UInt           aBindVarType, 
                                     UInt           aBindVarLen, 
                                     SChar         *aBindVar )
{
    UInt    sResultCode = DKP_RC_SUCCESS;
    UInt    sTimeoutSec;
    SInt    sReceiveTimeout = 0;
    SInt    sRemoteNodeRecvTimeout = 0;
    
    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerReceiveTimeoutFromConf( &sReceiveTimeout )
              != IDE_SUCCESS );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerRemoteNodeReceiveTimeoutFromConf( &sRemoteNodeRecvTimeout )
              != IDE_SUCCESS );
    
    sTimeoutSec = sReceiveTimeout + sRemoteNodeRecvTimeout;

    IDE_TEST( dkpProtocolMgr::sendBindRemoteVariable( aSession, 
                                                      aSessionId,
                                                      mId,
                                                      aBindVarIdx,
                                                      aBindVarType,
                                                      aBindVarLen,
                                                      aBindVar )
              != IDE_SUCCESS );

    IDE_TEST( dkpProtocolMgr::recvBindRemoteVariableResult( aSession,
                                                            aSessionId,
                                                            mId,
                                                            &sResultCode,
                                                            sTimeoutSec )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS, ERR_RECEIVE_RESULT );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            aSession,
                                            aSessionId,
                                            mId,
                                            &mErrorInfo );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * TODO: Remove wrapping function.
 */
IDE_RC dktRemoteStmt::bindBatch( UInt           /* aSessionId */,
                                 UInt           aBindVarIdx, 
                                 UInt           aBindVarType, 
                                 UInt           aBindVarLen, 
                                 SChar         *aBindVar )
{
    IDE_TEST( dkpBatchStatementMgrAddBindVariable( mBatchStatementMgr,
                                                   aBindVarIdx,
                                                   aBindVarType,
                                                   aBindVarLen,
                                                   aBindVar )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC  dktRemoteStmt::bind( dksSession    *aSession, 
                             UInt           aSessionId, 
                             UInt           aBindVarIdx, 
                             UInt           aBindVarType, 
                             UInt           aBindVarLen, 
                             SChar         *aBindVar )
{
    switch ( mStmtType )
    {
        case DKT_STMT_TYPE_REMOTE_TABLE:
        case DKT_STMT_TYPE_REMOTE_TABLE_STORE:
        case DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT:
        case DKT_STMT_TYPE_REMOTE_EXECUTE_NON_QUERY_STATEMENT:
            IDE_TEST( bindProtocol( aSession,
                                    aSessionId,
                                    aBindVarIdx, 
                                    aBindVarType, 
                                    aBindVarLen, 
                                    aBindVar )
                      != IDE_SUCCESS );
            break;
            
        case DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT_BATCH:
            IDE_TEST( bindBatch( aSessionId,
                                 aBindVarIdx, 
                                 aBindVarType, 
                                 aBindVarLen, 
                                 aBindVar )
                      != IDE_SUCCESS );            
            break;

        case DKT_STMT_TYPE_REMOTE_EXECUTE_IMMEDIATE:
        case DKT_STMT_TYPE_REMOTE_EXECUTE_QUERY_STATEMENT:
        default:
            IDE_RAISE( ERR_INVALID_REMOTE_STMT_TYPE );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT_TYPE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKT_INVALID_REMOTE_STMT_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote query 의 수행결과를 fetch 해온다.
 *
 *  aSession     - [IN] AltiLinker process 와의 연결을 위한 정보를 갖고 
 *                      있는 dksSession 구조체
 *  aSessionId   - [IN] 이 protocol operation 을 수행하려는 세션의 id
 *  aQcStatement - [IN] insertRow 함수에 넘겨줄 인자
 *
 ************************************************************************/
IDE_RC  dktRemoteStmt::fetchProtocol( dksSession    *aSession, 
                                      UInt           aSessionId,
                                      void          *aQcStatement,
                                      idBool         aFetchAllRecord )
{
    idBool      sIsAllocated  = ID_FALSE;
    idBool      sIsEndOfFetch = ID_FALSE;
    UInt        i;
    UInt        sRowSize         = 0;
    UInt        sFetchRowSize    = 0;
    UInt        sFetchRowBufSize = 0;
    UInt        sResultCode      = DKP_RC_SUCCESS;
    UInt        sFetchRowCnt     = 0;
    UInt        sFetchedRowCnt   = 0;
    UInt        sRecvRowLen      = 0;
    UInt        sRecvRowCnt      = 0;
    UInt        sRecvRowBufferSize = 0;    
    SChar      *sFetchRowBuffer  = NULL;
    SChar      *sRecvRowBuffer   = NULL;
    SChar      *sRow             = NULL;
    ULong       sTimeoutSec;

    dkdTypeConverter *sConverter = NULL;
    mtcColumn        *sColMeta   = NULL;

    SInt    sReceiveTimeout = 0;
    SInt    sQueryTimeout = 0;
    
    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerReceiveTimeoutFromConf( &sReceiveTimeout )
              != IDE_SUCCESS );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerQueryTimeoutFromConf( &sQueryTimeout )
              != IDE_SUCCESS );
    
    sTimeoutSec = sReceiveTimeout + ( 2 * sQueryTimeout );

    sConverter = mDataMgr->getTypeConverter();
    IDE_TEST_RAISE( sConverter == NULL, ERR_GET_TYPE_CONVETER_NULL );

    IDE_TEST( dkdTypeConverterGetConvertedMeta( sConverter, &sColMeta )
              != IDE_SUCCESS );

    /* calculate fetch row count */
    IDE_TEST( dkdTypeConverterGetRecordLength( sConverter, &sFetchRowSize )
              != IDE_SUCCESS );

    if ( sFetchRowSize != 0 )
    {
        /* 1. pre-setting */
        if ( sFetchRowSize > DKP_ADLP_PACKET_DATA_MAX_LEN )
        {
            sFetchRowBufSize = sFetchRowSize;
        }
        else
        {
            sFetchRowBufSize = DK_MAX_PACKET_LEN;
        }

        IDE_TEST( dkdMisc::getFetchRowCnt( sFetchRowSize,
                                           mBuffSize,
                                           &sFetchRowCnt,
                                           mStmtType )
                  != IDE_SUCCESS );

        /* 2. allocate temporary buffer */
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                           sFetchRowBufSize,
                                           (void **)&sFetchRowBuffer,
                                           IDU_MEM_IMMEDIATE )
                      != IDE_SUCCESS, ERR_MEMORY_ALLOC_FETCH_ROW_BUFFER );

        sIsAllocated   = ID_TRUE;
        sRecvRowBuffer = sFetchRowBuffer;
        sRecvRowBufferSize = sFetchRowBufSize;

        /* 3. fetch all records from remote server */
        while ( sIsEndOfFetch != ID_TRUE )
        {
            sFetchedRowCnt = 0;

            IDE_TEST( dkpProtocolMgr::sendFetchResultSet( aSession,
                                                          aSessionId,
                                                          mId,
                                                          sFetchRowCnt,
                                                          mBuffSize ) 
                      != IDE_SUCCESS );

            while ( ( sFetchRowCnt > sFetchedRowCnt ) && 
                    ( sIsEndOfFetch != ID_TRUE ) )
            {
                /* BUG-37850 */
                if ( sResultCode == DKP_RC_SUCCESS_FRAGMENTED )
                {
                    sRecvRowCnt = DKP_FRAGMENTED_ROW;
                }
                else
                {
                    /* BUG-36837 */
                    sRecvRowCnt = 0;
                }

                IDE_TEST( dkpProtocolMgr::recvFetchResultRow( aSession,
                                                              aSessionId,
                                                              mId,
                                                              sRecvRowBuffer,
                                                              sRecvRowBufferSize,
                                                              &sRecvRowLen,
                                                              &sRecvRowCnt,
                                                              &sResultCode,
                                                              sTimeoutSec )
                          != IDE_SUCCESS );

                /* Analyze result code */
                switch ( sResultCode )
                {
                    case DKP_RC_SUCCESS:

                        sFetchedRowCnt += sRecvRowCnt;
                        sRecvRowBuffer = sFetchRowBuffer;
                        sRecvRowBufferSize = sFetchRowBufSize;

                        for ( i = 0; i < sRecvRowCnt; i++ )
                        {
                            /* get the size of a row */
                            idlOS::memcpy( &sRowSize, sRecvRowBuffer, ID_SIZEOF( sRowSize ) );

                            /* get the offset to read a row */
                            sRow = sRecvRowBuffer += ID_SIZEOF( sRowSize );
                            sRecvRowBufferSize -= ID_SIZEOF( sRowSize );
                            
                            /* insert row */
                            IDE_TEST( mDataMgr->insertRow( sRow, aQcStatement ) != IDE_SUCCESS );

                            /* move the offset to the starting point of the next row */
                            sRecvRowBuffer += sRowSize;
                            sRecvRowBufferSize -= sRowSize;

                            /* BUG-36837 */
                            sRowSize = 0;
                        }

                        /* clean temporary buffer */
                        idlOS::memset( sFetchRowBuffer, 0, sFetchRowBufSize );

                        sRecvRowBuffer = sFetchRowBuffer;
                        sRecvRowBufferSize = sFetchRowBufSize;

                        break;
                        
                    case DKP_RC_SUCCESS_FRAGMENTED:

                        sRecvRowBuffer += ID_SIZEOF( sRecvRowLen );
                        sRecvRowBufferSize -= ID_SIZEOF( sRecvRowLen );
                        sRecvRowBuffer += sRecvRowLen;
                        sRecvRowBufferSize -= sRecvRowLen;
                        break;

                    case DKP_RC_END_OF_FETCH:

                        sIsEndOfFetch = ID_TRUE;
                        mDataMgr->setEndOfFetch( sIsEndOfFetch );
                        break;

                    default:

                        IDE_RAISE( ERR_RECEIVE_RESULT );
                        break;
                }
            }
            if ( aFetchAllRecord == ID_FALSE )
            {
                break;
            }
            else
            {
                /* nothing to do */
            }
        }

        /* 4. free fetch row buffer */
        sIsAllocated = ID_FALSE;
        IDE_TEST( iduMemMgr::free( sFetchRowBuffer ) != IDE_SUCCESS );
    }
    else
    {
        /* row size is 0, success */
    }

    if ( aFetchAllRecord == ID_TRUE )
    {
        IDE_TEST( mDataMgr->restart() != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( mDataMgr->restartOnce() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            aSession,
                                            aSessionId,
                                            mId,
                                            &mErrorInfo );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_FETCH_ROW_BUFFER );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION( ERR_GET_TYPE_CONVETER_NULL )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR, 
                                  "[dktRemoteStmt::fetchProtocol] sConverter is NULL" ) ); 
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsAllocated == ID_TRUE )
    {
        (void)iduMemMgr::free( sFetchRowBuffer );
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote query 의 수행결과를 일부 fetch 해온다.
 *  
 *      @ Related Keyword : REMOTE_TABLE
 *
 *  aSession     - [IN] AltiLinker process 와의 연결을 위한 정보를 갖고 
 *                      있는 dksSession 구조체
 *  aSessionId   - [IN] 이 protocol operation 을 수행하려는 세션의 id
 *
 ************************************************************************/
IDE_RC  dktRemoteStmt::executeRemoteFetch( dksSession    *aSession, 
                                           UInt           aSessionId )
{
    UInt                i;
    UInt                sRowSize            = 0;
    UInt                sResultCode         = 0;
    UInt                sFetchRowSize       = 0;
    UInt                sFetchRowCnt        = 0;
    UInt                sFetchedRowCnt      = 0;
    UInt                sFetchRowBufSize    = 0;
    UInt                sRecvRowLen         = 0;
    UInt                sRecvRowCnt         = 0;
    UInt                sRecvRowBufferSize  = 0;
    SChar              *sRecvRowBuffer      = NULL;
    SChar              *sFetchRowBuffer     = NULL;
    SChar              *sRow                = NULL;
    dkdRemoteTableMgr  *sRemoteTableMgr     = NULL;
    ULong               sTimeoutSec;
    SInt                sReceiveTimeout     = 0;
    SInt                sQueryTimeout       = 0;
    
    IDE_TEST_RAISE( mStmtType != DKT_STMT_TYPE_REMOTE_TABLE,
                    ERR_INVALID_STMT_TYPE );
    
    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerReceiveTimeoutFromConf( &sReceiveTimeout )
              != IDE_SUCCESS );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerQueryTimeoutFromConf( &sQueryTimeout )
              != IDE_SUCCESS );

    sTimeoutSec = sReceiveTimeout + ( 2 * sQueryTimeout );

    IDE_ASSERT( ( sRemoteTableMgr = getRemoteTableMgr() ) != NULL );

    sFetchRowSize = sRemoteTableMgr->getFetchRowSize();

    if ( ( sFetchRowSize > 0 ) && 
         ( sRemoteTableMgr->getEndOfFetch() != ID_TRUE ) )
    {
        sFetchRowBufSize = sRemoteTableMgr->getConvertedRowBufSize();
        sFetchRowBuffer  = sRemoteTableMgr->getConvertedRowBuffer();
        sFetchRowCnt     = sRemoteTableMgr->getFetchRowCnt();
    
        IDE_TEST( dkpProtocolMgr::sendFetchResultSet( aSession, 
                                                      aSessionId, 
                                                      mId, 
                                                      sFetchRowCnt,
                                                      mBuffSize ) 
                  != IDE_SUCCESS );

        sRecvRowBuffer = sFetchRowBuffer;
        sRecvRowBufferSize = sFetchRowBufSize;
        sFetchedRowCnt = 0;

        while ( ( sFetchRowCnt > sFetchedRowCnt ) && 
                ( sRemoteTableMgr->getEndOfFetch() != ID_TRUE ) )
        {   
            /* BUG-37850 */
            if ( sResultCode == DKP_RC_SUCCESS_FRAGMENTED )
            {
                sRecvRowCnt = DKP_FRAGMENTED_ROW;
            }
            else
            {
                sRecvRowCnt = 0;
            }
            
            IDE_TEST( dkpProtocolMgr::recvFetchResultRow( aSession, 
                                                          aSessionId, 
                                                          mId, 
                                                          sRecvRowBuffer, 
                                                          sRecvRowBufferSize,
                                                          &sRecvRowLen, 
                                                          &sRecvRowCnt, 
                                                          &sResultCode, 
                                                          sTimeoutSec ) 
                      != IDE_SUCCESS );

            switch ( sResultCode )
            {
                case DKP_RC_SUCCESS:

                    sFetchedRowCnt += sRecvRowCnt;
                    sRecvRowBuffer = sFetchRowBuffer;
                    sRecvRowBufferSize = sFetchRowBufSize;
                    for ( i = 0; i < sRecvRowCnt; i++ )
                    {
                        idlOS::memcpy( &sRowSize, sRecvRowBuffer, ID_SIZEOF( sRowSize ) );

                        sRecvRowBuffer += ID_SIZEOF( sRowSize );
                        sRecvRowBufferSize -= ID_SIZEOF( sRowSize );
                        
                        sRow = sRecvRowBuffer;

                        IDE_TEST( mRemoteTableMgr->insertRow( sRow ) != IDE_SUCCESS );

                        sRecvRowBuffer += sRowSize;
                        sRecvRowBufferSize -= sRowSize;
                        sRowSize = 0;
                    }

                    idlOS::memset( sFetchRowBuffer, 0, sFetchRowBufSize );

                    sRecvRowBuffer = sFetchRowBuffer;
                    sRecvRowBufferSize = sFetchRowBufSize;
                    break;

                case DKP_RC_SUCCESS_FRAGMENTED:

                    sRecvRowBuffer += ID_SIZEOF( sRecvRowLen );
                    sRecvRowBufferSize -= ID_SIZEOF( sRecvRowLen );
                    sRecvRowBuffer += sRecvRowLen;
                    sRecvRowBufferSize -= sRecvRowLen;
                    break;

                case DKP_RC_END_OF_FETCH:

                    sRemoteTableMgr->setEndOfFetch( ID_TRUE );
                    break;

                default:

                    IDE_RAISE( ERR_RECEIVE_RESULT );
                    break;
            }
        }

        sRemoteTableMgr->moveFirst();
    }
    else
    {
        sRemoteTableMgr->setEndOfFetch( ID_TRUE );
        sRemoteTableMgr->setCurRowNull();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_STMT_TYPE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKT_REMOTE_STMT_TYPE ) );
    }
    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            aSession,
                                            aSessionId,
                                            mId,
                                            &mErrorInfo );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote statement 를 free 해주기 위해 내부적으로 protocol
 *               operation 을 호출하여 AltiLinker 프로세스의 자원을 
 *               정리하도록 한 뒤, dkt 에서 할당받은 자원들도 정리해준다.
 *
 *  aSession    - [IN] AltiLinker process 와의 연결을 위한 정보를 갖고 
 *                     있는 dksSession 구조체
 *  aSessionId  - [IN] 이 protocol operation 을 수행하려는 세션의 id
 *
 ************************************************************************/
IDE_RC  dktRemoteStmt::freeProtocol( dksSession    *aSession, 
                                     UInt           aSessionId )
{
    UInt        sResultCode = DKP_RC_SUCCESS;
    UInt        sTimeoutSec;
    SInt        sReceiveTimeout = 0;
    SInt        sRemoteNodeRecvTimeout = 0;
    
    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerReceiveTimeoutFromConf( &sReceiveTimeout )
              != IDE_SUCCESS );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerRemoteNodeReceiveTimeoutFromConf( &sRemoteNodeRecvTimeout )
              != IDE_SUCCESS );
    
    sTimeoutSec = sReceiveTimeout + sRemoteNodeRecvTimeout;

    IDE_TEST( dkpProtocolMgr::sendRequestFreeRemoteStmt( aSession, 
                                                         aSessionId, 
                                                         mId ) 
              != IDE_SUCCESS );

    IDE_TEST( dkpProtocolMgr::recvRequestFreeRemoteStmtResult( aSession, 
                                                               aSessionId, 
                                                               mId, 
                                                               &sResultCode, 
                                                               sTimeoutSec )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS, ERR_RECEIVE_RESULT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            aSession,
                                            aSessionId,
                                            mId,
                                            &mErrorInfo );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC  dktRemoteStmt::freeBatchProtocol( dksSession    *aSession, 
                                          UInt           aSessionId )
{
    UInt        sResultCode = DKP_RC_SUCCESS;
    UInt        sTimeoutSec = 0;
    SInt        sReceiveTimeout = 0;
    SInt        sRemoteNodeRecvTimeout = 0;
    
    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerReceiveTimeoutFromConf( &sReceiveTimeout )
              != IDE_SUCCESS );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerRemoteNodeReceiveTimeoutFromConf( &sRemoteNodeRecvTimeout )
              != IDE_SUCCESS );
    
    sTimeoutSec = sReceiveTimeout + sRemoteNodeRecvTimeout;

    IDE_TEST( dkpProtocolMgr::sendRequestFreeRemoteStmtBatch( aSession, 
                                                              aSessionId, 
                                                              mId ) 
              != IDE_SUCCESS );

    IDE_TEST( dkpProtocolMgr::recvRequestFreeRemoteStmtBatchResult( aSession, 
                                                                    aSessionId, 
                                                                    mId, 
                                                                    &sResultCode, 
                                                                    sTimeoutSec )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS, ERR_RECEIVE_RESULT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            aSession,
                                            aSessionId,
                                            mId,
                                            &mErrorInfo );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC  dktRemoteStmt::free( dksSession    *aSession, 
                             UInt           aSessionId )
{
    switch ( mStmtType )
    {
        case DKT_STMT_TYPE_REMOTE_TABLE:
        case DKT_STMT_TYPE_REMOTE_TABLE_STORE:
        case DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT:
        case DKT_STMT_TYPE_REMOTE_EXECUTE_IMMEDIATE:
        case DKT_STMT_TYPE_REMOTE_EXECUTE_QUERY_STATEMENT:
        case DKT_STMT_TYPE_REMOTE_EXECUTE_NON_QUERY_STATEMENT:
            IDE_TEST( freeProtocol( aSession,
                                    aSessionId )
                      != IDE_SUCCESS );
            break;
            
        case DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT_BATCH:
            IDE_TEST( freeBatchProtocol( aSession,
                                         aSessionId )
                      != IDE_SUCCESS );            
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dktRemoteStmt::getResultCountBatch( SInt *aResultCount )
{
    IDE_TEST_RAISE( mStmtType != DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT_BATCH,
                    INVALID_STMT_TYPE );
    
    *aResultCount = mBatchExecutionResultSize;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_STMT_TYPE )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKT_REMOTE_STMT_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dktRemoteStmt::getResultBatch( SInt           aIndex,
                                      SInt          *aResult )
{
    IDE_TEST_RAISE( mStmtType != DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT_BATCH,
                    INVALID_STMT_TYPE );

    IDE_TEST_RAISE( aIndex < 0, INVALID_INDEX_NUMBER );
    IDE_TEST_RAISE( aIndex > mBatchExecutionResultSize, INVALID_INDEX_NUMBER );    
    
    *aResult = mBatchExecutionResult[aIndex - 1];
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_STMT_TYPE )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKT_REMOTE_STMT_TYPE ) );
    }
    IDE_EXCEPTION( INVALID_INDEX_NUMBER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKT_INVALID_COLUMN_INDEX ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/************************************************************************
 * Description : 이 remote statement 가 query 구문을 위한 객체인 경우 
 *               원격서버로부터 전송받는 data 를 저장 및 관리하기 위해 
 *               필요한 data manager 를 생성한다.
 *
 ************************************************************************/
IDE_RC  dktRemoteStmt::createDataManager()
{
    idBool   sIsAllocated = ID_FALSE;

    destroyDataManager();
    
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( dkdDataMgr ),
                                       (void **)&mDataMgr,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_DATA_MGR );

    sIsAllocated = ID_TRUE;

    IDE_TEST( mDataMgr->initialize( &mBuffSize ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_DATA_MGR );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsAllocated == ID_TRUE )
    {
        (void)iduMemMgr::free( mDataMgr );
        mDataMgr = NULL;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 이 remote statement 가 query 구문을 위한 객체인 경우 
 *               생성시 할당되어 사용이 끝난 data manager 를 정리해
 *               주고 제거한다.
 *
 *  BUG-37487 : return 값을 IDE_RC --> void 로 변경.
 *
 ************************************************************************/
void dktRemoteStmt::destroyDataManager()
{
    if ( mDataMgr != NULL )
    {
        mDataMgr->finalize();

        (void)iduMemMgr::free( mDataMgr );
        mDataMgr = NULL;
    }
    else
    {
        /* already destroyed */
    }
}

/************************************************************************
 * Description : 원격서버로부터 전송받는 레코드를 관리하기 위해 
 *               필요한 remote table manager 를 생성한다.
 *
 ************************************************************************/
IDE_RC  dktRemoteStmt::createRemoteTableManager()
{
    if ( mRemoteTableMgr != NULL )
    {
        (void)iduMemMgr::free( mRemoteTableMgr );
        mRemoteTableMgr = NULL;
    }
    else
    {
        /* no remote table manager */
    }

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK, 
                                       ID_SIZEOF( dkdRemoteTableMgr ), 
                                       (void **)&mRemoteTableMgr, 
                                       IDU_MEM_IMMEDIATE ) 
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_REMOTE_TABLE_MGR ); 

    IDE_TEST( mRemoteTableMgr->initialize( &mBuffSize ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_REMOTE_TABLE_MGR );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( mRemoteTableMgr != NULL )
    {
        (void)iduMemMgr::free( mRemoteTableMgr );
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote statement type 에 따라 remote data manager 를 
 *               활성화 시킨다. 
 *
 *  aQcStatement - [IN] Remote statement type 이 REMOTE_TABLE_STORE 또는 
 *                      REMOTE_EXECUTE_QUERY_STATEMENT 인 경우 
 *                      data manager 에 넘겨줄 인자
 *
 ************************************************************************/
IDE_RC  dktRemoteStmt::activateRemoteDataManager( void  *aQcStatement )
{
    switch ( mStmtType )
    {
        case DKT_STMT_TYPE_REMOTE_TABLE:
#ifdef ALTIBASE_PRODUCT_XDB
            IDE_DASSERT( mDataMgr != NULL );
            IDE_TEST( mDataMgr->activate( aQcStatement ) != IDE_SUCCESS );
#else
            IDE_DASSERT( mRemoteTableMgr != NULL );
            IDE_TEST( mRemoteTableMgr->activate() != IDE_SUCCESS );
#endif
            break;

        case DKT_STMT_TYPE_REMOTE_TABLE_STORE:
        case DKT_STMT_TYPE_REMOTE_EXECUTE_QUERY_STATEMENT:
            IDE_DASSERT( mDataMgr != NULL );
            IDE_TEST( mDataMgr->activate( aQcStatement ) != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_INVALID_REMOTE_STMT_TYPE );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT_TYPE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKT_INVALID_REMOTE_STMT_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 이 remote statement 가 query 구문을 위한 객체인 경우 
 *               생성시 할당되어 사용이 끝난 remote table manager 를 
 *               정리해주고 제거한다.
 *
 ************************************************************************/
void dktRemoteStmt::destroyRemoteTableManager()
{
    if ( mRemoteTableMgr != NULL )
    {
        mRemoteTableMgr->finalize();

        (void)iduMemMgr::free( mRemoteTableMgr );
        mRemoteTableMgr = NULL;
    }
    else
    {
        /* already destroyed */
    }
}

/************************************************************************
 * Description : 이 remote statement 수행결과가 error 인 경우, 프로토콜
 *               을 통해 AltiLinker 프로세스로부터 원격서버의 error 정
 *               보를 받아올 수 있는데 이렇게 받아온 error 정보를 remote
 *               statement 객체에 저장하기 위해 수행되는 함수
 *
 *  aSession    - [IN] AltiLinker process 와의 연결을 위한 정보를 갖고 
 *                     있는 dksSession 구조체
 *  aSessionId  - [IN] 이 protocol operation 을 수행하려는 세션의 id
 *
 ************************************************************************/
IDE_RC  dktRemoteStmt::getErrorInfoFromProtocol( dksSession  *aSession,
                                                 UInt         aSessionId )
{
    UInt        sResultCode = DKP_RC_SUCCESS;
    ULong       sTimeoutSec;
    SInt        sReceiveTimeout = 0;
    
    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerReceiveTimeoutFromConf( &sReceiveTimeout )
              != IDE_SUCCESS );
    
    sTimeoutSec = sReceiveTimeout;

    IDE_TEST( dkpProtocolMgr::sendRequestRemoteErrorInfo( aSession,
                                                          aSessionId, 
                                                          mId ) 
              != IDE_SUCCESS );

    IDE_TEST( dkpProtocolMgr::recvRequestRemoteErrorInfoResult( aSession, 
                                                                aSessionId, 
                                                                &sResultCode, 
                                                                mId, 
                                                                &mErrorInfo, 
                                                                sTimeoutSec ) 
              != IDE_SUCCESS ); 

    IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS, ERR_RECEIVE_RESULT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            aSession,
                                            aSessionId,
                                            mId,
                                            &mErrorInfo );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Performance view 를 위해 이 remote statement 의 정보를 
 *               info 구조체에 담아주는 함수
 *
 *  aInfo       - [IN] Remote statement 정보를 담아줄 구조체
 *
 ************************************************************************/
IDE_RC  dktRemoteStmt::getRemoteStmtInfo( dktRemoteStmtInfo *aInfo )
{
    aInfo->mGlobalTxId = mGlobalTxId;
    aInfo->mLocalTxId  = mLocalTxId;
    aInfo->mRTxId  = mRTxId;
    aInfo->mStmtId = mId;
    idlOS::strcpy( aInfo->mStmtStr, mStmtStr );

    return IDE_SUCCESS;
}

/************************************************************************
 * Description : 입력받은 문자열을 이 remote statement 객체에 copy 한다.
 *               내부적으로 메모리를 할당하므로 다른 곳에서 메모리 해제를 
 *               해주어야 한다. 이 remote statement 객체를 정리할 때 같이
 *               해준다.
 *
 *  aStmtStr    - [IN] 입력받은 remote statement 구문
 *
 ************************************************************************/
IDE_RC  dktRemoteStmt::setStmtStr( SChar   *aStmtStr )
{
    idBool      sIsAllocated = ID_FALSE;
    UInt        sStmtLen     = 0;

    sStmtLen = idlOS::strlen( aStmtStr );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       sStmtLen + 1,
                                       (void **)&mStmtStr,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_REMOTE_STMT_STR );

    sIsAllocated = ID_TRUE;

    idlOS::strcpy( aStmtStr, mStmtStr );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_REMOTE_STMT_STR );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsAllocated == ID_TRUE )
    {
        (void)iduMemMgr::free( mStmtStr );
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

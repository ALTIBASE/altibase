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
 * $Id$ sdl.h
 **********************************************************************/

#ifndef _O_SDL_H_
#define _O_SDL_H_ 1

#include <sdi.h>
#include <sdSql.h>
#include <sdlStatement.h>
#include <sdlSqlConnAttrType.h>

#define SQL_FALSE               0
#define SQL_TRUE                1

#define SQL_HANDLE_ENV          1
#define SQL_HANDLE_DBC          2
#define SQL_HANDLE_STMT         3
#define SQL_HANDLE_DESC         4

#define SQL_NULL_HANDLE         0L

#define SQL_SUCCESS             0
#define SQL_SUCCESS_WITH_INFO   1
#define SQL_NO_DATA             100
#define SQL_NO_DATA_FOUND       100
#define SQL_NULL_DATA           ((SInt)-1)
#define SQL_ERROR               ((SInt)-1)
#define SQL_INVALID_HANDLE      ((SInt)-2)

#define SQL_CLOSE               0
#define SQL_DROP                1
#define SQL_UNBIND              2
#define SQL_RESET_PARAMS        3

#define SQL_NTS                 (-3)

#define MAX_ODBC_ERROR_MSG_LEN   1024
#define MAX_ODBC_MSG_LEN         512

#define MAX_HOST_NUM    128

#define HOST_MAX_LEN    SDI_SERVER_IP_SIZE
#define USER_ID_LEN     128
#define MAX_PWD_LEN     32
#define MAX_HOST_INFO_LEN (HOST_MAX_LEN + 3 + USER_ID_LEN + MAX_PWD_LEN + 8)
       // 3=(:::), 8=(port length)

typedef void * LIBRARY_HANDLE;

typedef struct
{
    UInt    mOldErrorCode;
    SChar   mErrorMsgBuffer[MAX_LAST_ERROR_MSG_LEN];
    UInt    mErrorMsgBufferLen;
} sdlNestedErrorMgr;

class sdl
{
public:

    static IDE_RC initOdbcLibrary();
    static void   finiOdbcLibrary();

    static idBool hasNodeAlternate( sdiNode * aNode );

    static IDE_RC allocConnect( sdiConnectInfo * aConnectInfo );

    static IDE_RC internalConnectCore( void            * aDbc,
                                       sdiConnectInfo  * aConnectInfo,
                                       UChar           * aConnectString );

    static IDE_RC disconnect( sdiConnectInfo * aConnectInfo,
                              idBool         * aIsLinkFailure = NULL );

    static IDE_RC disconnectLocal( sdiConnectInfo * aConnectInfo,
                                   idBool         * aIsLinkFailure = NULL );

    static IDE_RC freeConnect( sdiConnectInfo * aConnectInfo,
                               idBool         * aIsLinkFailure = NULL);

    static IDE_RC allocStmt( sdiConnectInfo          * aConnectInfo,
                             sdlRemoteStmt           * aRemoteStmt,
                             sdiShardPartialExecType   aPartialExecType,
                             idBool                  * aIsLinkFailure = NULL );

    static IDE_RC freeStmt( sdiConnectInfo * aConnectInfo,
                            sdlRemoteStmt  * aRemoteStmt,
                            SShort           aOption,
                            idBool         * aIsLinkFailure = NULL );

    static IDE_RC addPrepareCallback( void           ** aCallback,
                                      UInt              aNodeIndex,
                                      sdiConnectInfo  * aConnectInfo,
                                      sdlRemoteStmt   * aRemoteStmt,
                                      SChar           * aQuery,
                                      SInt              aLength,
                                      idBool          * aIsLinkFailure = NULL );

    static IDE_RC describeCol( sdiConnectInfo * aConnectInfo,
                               sdlRemoteStmt  * aRemoteStmt,
                               UInt             aColNo,
                               SChar          * aColName,
                               SInt             aBufferLength,
                               UInt           * aNameLength,
                               UInt           * aTypeId,
                               SInt           * aPrecision,
                               SShort         * aScale,
                               SShort         * aNullable,
                               idBool         * aIsLinkFailure = NULL );

    static IDE_RC getParamsCount( sdiConnectInfo * aConnectInfo,
                                  sdlRemoteStmt  * aRemoteStmt,
                                  UShort         * aParamCount,
                                  idBool         * aIsLinkFailure = NULL );

    static IDE_RC bindParam( sdiConnectInfo * aConnectInfo,
                             sdlRemoteStmt  * aRemoteStmt,
                             UShort           aParamNum,
                             UInt             aInOutType,
                             UInt             aValueType,
                             void           * aParamValuePtr,
                             SInt             aBufferLen,
                             SInt             aPrecision,
                             SInt             aScale,
                             SInt           * aIndicator,
                             idBool         * aIsLinkFailure = NULL );

    static IDE_RC bindCol( sdiConnectInfo * aConnectInfo,
                           sdlRemoteStmt  * aRemoteStmt,
                           UInt             aColNo,
                           UInt             aColType,
                           UInt             aColSize,
                           void           * aValueBuffer,
                           UInt             aValueBufferMaxLen,
                           SInt           * aValueLen,
                           idBool         * aIsLinkFailure = NULL );

    static IDE_RC execDirect( sdiConnectInfo * aConnectInfo,
                              sdlRemoteStmt  * aRemoteStmt,
                              SChar          * aQuery,
                              SInt             aQueryLen,
                              sdiClientInfo  * aClientInfo,
                              idBool         * aIsLinkFailure = NULL );

    static IDE_RC addExecDirectCallback( void           ** aCallback,
                                         UInt              aNodeIndex,
                                         sdiConnectInfo  * aConnectInfo,
                                         sdlRemoteStmt   * aRemoteStmt,
                                         SChar           * aQuery,
                                         SInt              aLength,
                                         idBool          * aIsLinkFailure = NULL );

    static IDE_RC addExecuteCallback( void           ** aCallback,
                                      UInt              aNodeIndex,
                                      sdiConnectInfo  * aConnectInfo,
                                      sdiDataNode     * aNode,
                                      idBool          * aIsLinkFailure = NULL );

    static IDE_RC addPrepareTranCallback( void          ** aCallback,
                                          UInt             aNodeIndex,
                                          sdiConnectInfo * aConnectInfo,
                                          ID_XID         * aXID,
                                          UChar          * aReadOnly,
                                          idBool         * aIsLinkFailure = NULL );

    static IDE_RC addEndPendingTranCallback( void           ** aCallback,
                                             UInt              aNodeIndex,
                                             sdiConnectInfo  * aConnectInfo,
                                             ID_XID          * aXID,
                                             idBool            aIsCommit,
                                             idBool          * aIsLinkFailure );

    static IDE_RC addEndTranCallback( void          ** aCallback,
                                      UInt             aNodeIndex,
                                      sdiConnectInfo * aConnectInfo,
                                      idBool           aIsCommit,
                                      idBool         * aIsLinkFailure = NULL );

    static void doCallback( void * aCallback );

    static IDE_RC resultCallback( void           * aCallback,
                                  UInt             aNodeIndex,
                                  sdiConnectInfo * aConnectInfo,
                                  idBool           aReCall,
                                  sdiClientInfo  * aClientInfo,
                                  SShort           aHandleType,
                                  void           * aHandle,
                                  SChar          * aFuncName,
                                  idBool         * aIsLinkFailure = NULL );

    static void removeCallback( void * aCallback );

    static IDE_RC setFailoverSuspend( sdiConnectInfo         * aConnectInfo,
                                     sdiFailoverSuspendType   aSuspendOnType,
                                     UInt                     aNewErrorCode );

    static IDE_RC fetch( sdiConnectInfo * aConnectInfo,
                         sdlRemoteStmt  * aRemoteStmt,
                         SShort         * aResult,
                         idBool         * aIsLinkFailure = NULL );

    static IDE_RC rowCount( sdiConnectInfo * aConnectInfo,
                            sdlRemoteStmt  * aRemoteStmt,
                            vSLong         * aNumRows,
                            idBool         * aIsLinkFailure = NULL );

    static IDE_RC getPlan( sdiConnectInfo  * aConnectInfo,
                           sdlRemoteStmt   * aRemoteStmt,
                           SChar          ** aPlan,
                           idBool          * aIsLinkFailure = NULL );

    static IDE_RC setAutoCommit( void   * aDbc,
                                 idBool   aIsAuto,
                                 idBool * aIsLinkFailure = NULL );

    static IDE_RC setConnAttr( sdiConnectInfo * aConnectInfo,
                               UShort           aAttrType,
                               ULong            aValue,
                               SChar          * aValueStr,                               
                               idBool         * aIsLinkFailure = NULL );

    static IDE_RC getConnAttr( sdiConnectInfo * aConnectInfo,
                               UShort           aAttrType,
                               void           * aValuePtr,
                               SInt             aBuffLength = 0,
                               SInt           * aStringLength = NULL );

    static IDE_RC endPendingTran( sdiConnectInfo * aConnectInfo,
                                  ID_XID         * aXID,
                                  idBool           aIsCommit,
                                  idBool         * aIsLinkFailure = NULL );

    static IDE_RC checkNode( sdiConnectInfo * aConnectInfo,
                             idBool         * aIsLinkFailure );

    static IDE_RC commit( sdiConnectInfo * aConnectInfo,
                          sdiClientInfo  * aClientInfo,
                          idBool         * aIsLinkFailure = NULL );

    static IDE_RC rollback( sdiConnectInfo * aConnectInfo,
                            const SChar    * aSavepoint,
                            sdiClientInfo  * aClientInfo,
                            idBool         * aIsLinkFailure = NULL );

    static IDE_RC setSavepoint( sdiConnectInfo * aConnectInfo,
                                const SChar    * aSavepoint,
                                idBool         * aIsLinkFailure = NULL );

    static IDE_RC shardStmtPartialRollback( sdiConnectInfo * aConnectInfo,
                                            idBool         * aIsLinkFailure = NULL );

    static idBool checkDbcAlive( void   * aDbc );

    static IDE_RC getLinkInfo( sdiConnectInfo * aConnectInfo,
                               SChar          * aBuf,
                               UInt             aBufSize,
                               SInt             aKey );

    static IDE_RC isDataNode( sdiConnectInfo * aConnectInfo,
                              idBool         * aIsDataNode );

    static IDE_RC checkNeedFailover( sdiConnectInfo * aConnectInfo,
                                     idBool         * aNeedFailover );

    static idBool retryConnect( sdiConnectInfo * aConnectInfo,
                                SShort           aHandleType,
                                void           * aHandle );

    static void clearInternalConnectResult( sdiConnectInfo * aConnectInfo );

    static void setInternalConnectFailure( sdiConnectInfo * aConnectInfo );

    static IDE_RC internalConnect( void           * aDbc,
                                   sdiConnectInfo * aConnectInfo,
                                   UChar          * aConnectString,
                                   idBool           aIsShardClient );

    static idBool isInternalConnectFailoverLimited( sdiConnectInfo * aConnectInfo );

    static void setTransactionBrokenByInternalConnection( sdiConnectInfo * aConnectInfo );

    static void setTransactionBrokenByTransactionID( idBool     aIsUserAutoCommit,
                                                     void     * aDkiSession,
                                                     smiTrans * aTrans );

    static void appendResultInfoToErrorMessage( sdiConnectInfo    * aConnectInfo,
                                                qciStmtType         aStmtKind );

    /* PROJ-2728 Sharding LOB */
    static IDE_RC getLob( sdiConnectInfo * aConnectInfo,
                          sdlRemoteStmt  * aRemoteStmt,
                          SShort           aLocatorCType,
                          ULong            aLocator,
                          UInt             aStartOffset,
                          UInt             aSizeToGet,
                          SShort           aTargetCType,
                          void           * aBufferToStoreData,
                          UInt             aSizeBuffer,
                          UInt           * aSizeReadPtr,
                          idBool         * aIsLinkFailure );

    static IDE_RC putLob( sdiConnectInfo * aConnectInfo,
                          sdlRemoteStmt  * aRemoteStmt,
                          SShort           aLocatorCType,
                          ULong            aLocator,
                          SShort           aSourceCType,
                          void           * aValue,
                          SLong            aLength,
                          idBool         * aIsLinkFailure );

    static IDE_RC putEmptyLob( sdiConnectInfo * aConnectInfo,
                               sdlRemoteStmt  * aRemoteStmt,
                               SShort           aLocatorCType,
                               ULong            aLocator,
                               SShort           aSourceCType,
                               void           * aValue,
                               idBool         * aIsLinkFailure );

    static IDE_RC lobWrite( sdiConnectInfo * aConnectInfo,
                            sdlRemoteStmt  * aRemoteStmt,
                            SShort           aLocatorCType,
                            ULong            aLocator,
                            SShort           aTargetCType,
                            void           * aDataToPut,
                            UInt             aSizeDataToPut,
                            idBool         * aIsLinkFailure );

    static IDE_RC lobPrepare4Write( sdiConnectInfo * aConnectInfo,
                                    sdlRemoteStmt  * aRemoteStmt,
                                    SShort           aLocatorCType,
                                    ULong            aLocator,
                                    UInt             aStartOffset,
                                    UInt             aNewSize,
                                    idBool         * aIsLinkFailure );

    static IDE_RC lobFinishWrite( sdiConnectInfo * aConnectInfo,
                                  sdlRemoteStmt  * aRemoteStmt,
                                  SShort           aLocatorCType,
                                  ULong            aLocator,
                                  idBool         * aIsLinkFailure );

    static IDE_RC getLobLength( sdiConnectInfo * aConnectInfo,
                                sdlRemoteStmt  * aRemoteStmt,
                                SShort           aLocatorCType,
                                ULong            aLocator,
                                UInt           * aLobLengthPtr,
                                idBool         * aIsNullLob,
                                idBool         * aIsLinkFailure );

    static IDE_RC freeLob( sdiConnectInfo * aConnectInfo,
                           sdlRemoteStmt  * aRemoteStmt,
                           ULong            aLocator,
                           idBool         * aIsLinkFailure );

    static IDE_RC trimLob( sdiConnectInfo * aConnectInfo,
                           sdlRemoteStmt  * aRemoteStmt,
                           SShort           aLocatorCType,
                           ULong            aLocator,
                           UInt             aStartOffset,
                           idBool         * aIsLinkFailure );

    /* PROJ-2733-DistTxInfo */
    static IDE_RC getSCN( sdiConnectInfo * aConnectInfo, smSCN * aSCN );
    static IDE_RC setSCN( sdiConnectInfo * aConnectInfo, smSCN * aSCN );
    static IDE_RC setTxFirstStmtSCN( sdiConnectInfo * aConnectInfo, smSCN * aTxFirstStmtSCN );
    static IDE_RC setTxFirstStmtTime( sdiConnectInfo * aConnectInfo, SLong aTxFirstStmtTime );
    static IDE_RC setDistLevel( sdiConnectInfo * aConnectInfo, sdiDistLevel aDistLevel );

    static IDE_RC setTargetShardMetaNumber( sdiClientInfo * aClientInfo, sdiConnectInfo * aConnectInfo );

    /* TASK-7219 Non-shard DML */
    static IDE_RC setStmtExecSeq( sdiConnectInfo * aConnectInfo, UInt aStmtExecSequence );

private:
    static IDE_RC getConnectedLinkFullAddress( sdiConnectInfo * aConnectInfo );

    static void processError( SShort           aHandleType,
                              void           * aHandle,
                              SChar          * aCallFncName,
                              sdiConnectInfo * aConnectInfo,
                              idBool         * aIsLinkFailure = NULL );

    static void appendFootPrintToErrorMessage( sdiConnectInfo * aConnectInfo,
                                               UInt             aErrorCode, 
                                               SChar          * aErrorMsgPtr );

    static void makeIdeErrorBackup( sdlNestedErrorMgr * aMgr );

    static void mergeIdeErrorWithBackup( sdlNestedErrorMgr * aMgr );

    static void processErrorOnNested( SShort           aHandleType,
                                      void           * aHandle,
                                      SChar          * aCallFncName,
                                      sdiConnectInfo * aConnectInfo,
                                      idBool         * aIsLinkFailure = NULL );

    static IDE_RC loadLibrary();

    static idBool isFatalError( SShort           aHandleType,
                                void            *aHandle,
                                sdiConnectInfo  *aConnectInfo );

    /* PROJ-2728 Sharding LOB */
    static IDE_RC lobEnter( sdiConnectInfo * aConnectInfo,
                            sdlRemoteStmt  * aRemoteStmt,
                            const SChar    * aSdlFuncName,
                            SShort         * aHandleType,
                            SQLHANDLE      * aHandle );

    /* TASK-7218 Multi-Error Handling */
    static void addShardError( UInt             aErrorCode,
                               UInt             aNodeErrorCode,
                               SChar          * aErrorMessage,
                               SChar          * aCallFncName,
                               SChar          * aNodeName,
                               sdiConnectInfo * aConnectInfo );

private:
    static LIBRARY_HANDLE mOdbcLibHandle;
    static idBool         mInitialized;
};

#define IS_SUCCEEDED( aRET ) {\
    ( ( aRET == SQL_SUCCESS ) || ( aRET == SQL_SUCCESS_WITH_INFO ) || ( aRet == SQL_NO_DATA ) )

#define LOAD_LIBRARY(aName, aHandle)\
        {\
            aHandle = idlOS::dlopen( aName, RTLD_LAZY|RTLD_GLOBAL );\
            IDE_TEST_RAISE( aHandle == NULL, errInitOdbcLibrary );\
        }

#define ODBC_FUNC_DEF( aType )\
        {\
            SQL##aType = (ODBC##aType)idlOS::dlsym( sdl::mOdbcLibHandle, STR_SQL##aType );\
            IDE_TEST_RAISE( SQL##aType == NULL, errInitOdbcLibrary );\
        }

#define IDE_EXCEPTION_UNINIT_LIBRARY( aConnectionInfo, aFunctionName )\
        IDE_EXCEPTION( UnInitializedError )\
        {\
            IDE_SET( ideSetErrorCode( sdERR_ABORT_UNINITIALIZED_LIBRARY, \
                                      ( (aConnectionInfo == NULL) ? (SChar*)"" : \
                                                                    (SChar*)aConnectionInfo->mNodeName ), \
                                      aFunctionName ) );\
        }

#define IDE_EXCEPTION_NULL_DBC( aConnectionInfo, aFunctionName )\
        IDE_EXCEPTION( ErrorNullDbc )\
        {\
            IDE_SET( ideSetErrorCode( sdERR_ABORT_EXECUTE_NULL_DBC, \
                                      ( (aConnectionInfo == NULL) ? (SChar*)"" : \
                                                                    (SChar*)aConnectionInfo->mNodeName ), \
                                      aFunctionName ) );\
        }

#define IDE_EXCEPTION_NULL_STMT( aConnectionInfo, aFunctionName )\
        IDE_EXCEPTION( ErrorNullStmt )\
        {\
            IDE_SET( ideSetErrorCode( sdERR_ABORT_EXECUTE_NULL_STMT, \
                                      ( (aConnectionInfo == NULL) ? (SChar*)"" : \
                                                                    (SChar*)aConnectionInfo->mNodeName ), \
                                      aFunctionName ) );\
        }

#define IDE_EXCEPTION_NULL_SD_STMT( aConnectionInfo, aFunctionName )\
        IDE_EXCEPTION( ErrorNullSdStmt )\
        {\
            IDE_SET( ideSetErrorCode( sdERR_ABORT_EXECUTE_NULL_SD_STMT, \
                                      ( (aConnectionInfo == NULL) ? (SChar*)"" : \
                                                                    (SChar*)aConnectionInfo->mNodeName ), \
                                      aFunctionName ) );\
        }

#define SET_LINK_FAILURE_FLAG( aFlagPtr, aValue )\
        if ( aFlagPtr != NULL ){ *aFlagPtr = aValue; }\
        else { /* Do Nothing. */ }

inline idBool sdl::hasNodeAlternate( sdiNode * aNode )
{
    return ( ( ( aNode->mAlternateServerIP[0] != '\0' ) &&
               ( aNode->mAlternatePortNo > 0 ) ) ?
             ID_TRUE :
             ID_FALSE );
}

inline IDE_RC sdl::lobEnter( sdiConnectInfo * aConnectInfo,
                             sdlRemoteStmt  * aRemoteStmt,
                             const SChar    * aSdlFuncName,
                             SShort         * aHandleType,
                             SQLHANDLE      * aHandle )
{
    IDE_TEST_RAISE( mInitialized == ID_FALSE, UnInitializedError );

    if ( aRemoteStmt != NULL && aRemoteStmt->mStmt != NULL )
    {
        *aHandleType   = SQL_HANDLE_STMT;
        *aHandle = (SQLHANDLE) aRemoteStmt->mStmt;
    }
    else if ( aConnectInfo != NULL && aConnectInfo->mDbc != NULL )
    {
        *aHandleType   = SQL_HANDLE_DBC;
        *aHandle = (SQLHANDLE) aConnectInfo->mDbc;
    }
    else
    {
        IDE_TEST_RAISE( aRemoteStmt == NULL, ErrorNullSdStmt );
        IDE_TEST_RAISE( aConnectInfo == NULL, ErrorNullSdStmt );
        IDE_TEST_RAISE( aRemoteStmt->mStmt == NULL, ErrorNullStmt );
        IDE_TEST_RAISE( aConnectInfo->mDbc == NULL, ErrorNullDbc );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_UNINIT_LIBRARY( aConnectInfo, aSdlFuncName )
    IDE_EXCEPTION_NULL_SD_STMT( aConnectInfo, aSdlFuncName )
    IDE_EXCEPTION_NULL_DBC( aConnectInfo, aSdlFuncName )
    IDE_EXCEPTION_NULL_STMT( aConnectInfo, aSdlFuncName )
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#endif /* _O_SDL_H_ */

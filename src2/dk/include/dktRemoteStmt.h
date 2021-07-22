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

#ifndef _O_DKT_REMOTE_STMT_H_
#define _O_DKT_REMOTE_STMT_H_ 1

#include <idTypes.h>
#include <dkt.h>
#include <dkoLinkObjMgr.h>
#include <dkdDataMgr.h>
#include <dkdRemoteTableMgr.h>
#include <dkpBatchStatementMgr.h>

class dktRemoteStmt
{
private:
    SLong               mId;
    UInt                mStmtType;  /* passthrough func type */
    UInt                mResult;    /* success or failed */
    dkdDataMgr        * mDataMgr;   /* remote query */
    dkdRemoteTableMgr * mRemoteTableMgr; /* only remote_table */
    dktErrorInfo        mErrorInfo; /* from getErrorInfo */
    SChar             * mStmtStr;   /* remote statement string */
    dkoLink             mExecutedLinkObj; /* BUG-37077 */
    UInt                mBuffSize;   /* PROJ-2417 */

    IDE_RC              prepareProtocol( dksSession    *aSession,
                                         UInt           aSessionId,
                                         idBool         aIsTxBegin,
                                         ID_XID        *aXID,
                                         dkoLink       *aLinkObj,
                                         UShort        *aRemoteSessionId );
    
    IDE_RC              prepareBatchProtocol( dksSession *aSession, 
                                              UInt        aSessionId,
                                              idBool      aIsTxBegin,
                                              ID_XID     *aXID,
                                              dkoLink    *aLinkObj,
                                              UShort     *aRemoteSessionId );

    IDE_RC              freeProtocol( dksSession    *aSession, 
                                      UInt           aSessionId );

    IDE_RC              freeBatchProtocol( dksSession    *aSession, 
                                           UInt           aSessionId );

    IDE_RC              bindProtocol( dksSession    *aSession, 
                                      UInt           aSessionId, 
                                      UInt           aBindVarIdx, 
                                      UInt           aBindVarType, 
                                      UInt           aBindVarLen, 
                                      SChar         *aBindVar );

    IDE_RC              bindBatch( UInt           aSessionId, 
                                   UInt           aBindVarIdx, 
                                   UInt           aBindVarType, 
                                   UInt           aBindVarLen, 
                                   SChar         *aBindVar );

    IDE_RC prepareAddBatch( dksSession *aSession,
                            UInt aSessionId,
                            ULong aTimeoutSec );

    IDE_RC addBatches( dksSession *aSession,
                       UInt aSessionId,
                       ULong aTimeoutSec );
    
    IDE_RC executeBatch( dksSession *aSession,
                         UInt aSessionId,
                         ULong aTimeoutSec );

    dkpBatchStatementMgr * mBatchStatementMgr;
    SInt                 * mBatchExecutionResult;
    SInt                   mBatchExecutionResultSize;
    
public:
    UInt               mGlobalTxId;
    UInt                mLocalTxId;
    UInt                mRTxId;
    iduListNode         mNode;

    /* Initialize / Finalize / Clean */
    IDE_RC          initialize( UInt   aGlobalTxId,
                                UInt    aLocalTxId,
                                UInt    aRTxId,
                                SLong   aId, 
                                UInt    aStmtType,
                                SChar  *aStmtStr );

    /* BUG-37487 */
    void            finalize(); /* <-- 내부적으로 clean() 호출 */

    /* BUG-37487 */
    void            clean();

    /* Operations */
    IDE_RC          prepare( dksSession    *aSession,
                             UInt           aSessionId,
                             idBool         aIsTxBegin,
                             ID_XID        *aXID,
                             dkoLink       *aLinkObj,
                             UShort        *aRemoteSessionId );
    
    IDE_RC          requestResultSetMetaProtocol( dksSession   *aSession, 
                                                  UInt          aSessionId );

    IDE_RC          executeProtocol( dksSession    *aSession,
                                     UInt           aSessionId,
                                     idBool         aIsTxBegin,
                                     ID_XID        *aXID,
                                     dkoLink       *aLinkObj,
                                     SInt          *aResult );

    IDE_RC          addBatch( void );

    IDE_RC          executeBatchProtocol( dksSession    *aSession, 
                                          UInt           aSessionId, 
                                          SInt          *aResult );
    
    /* BUG-37663 */
    IDE_RC          abortProtocol( dksSession    *aSession, 
                                   UInt           aSessionId );

    IDE_RC          bind( dksSession    *aSession, 
                          UInt           aSessionId, 
                          UInt           aBindVarIdx, 
                          UInt           aBindVarType, 
                          UInt           aBindVarLen, 
                          SChar         *aBindVar );

    IDE_RC          fetchProtocol( dksSession    *aSession,
                                   UInt           aSessionId,
                                   void          *aQcStatement,
                                   idBool         aFetchAllRecord );

    IDE_RC          executeRemoteFetch( dksSession    *aSession, 
                                        UInt           aSessionId );

    IDE_RC          free( dksSession    *aSession, 
                          UInt           aSessionId );

    IDE_RC          getResultCountBatch( SInt *aResultCount );

    IDE_RC          getResultBatch( SInt           aIndex,
                                    SInt          *aResult );
    
    /* Data manager */
    IDE_RC          createDataManager();
    /* BUG-37487 */
    void            destroyDataManager();

    /* Remote table manager */
    IDE_RC          createRemoteTableManager();
    void            destroyRemoteTableManager();

    IDE_RC          activateRemoteDataManager( void *aQcStatement );

    /* PV: Get remote statement information for performance view */
    IDE_RC          getRemoteStmtInfo( dktRemoteStmtInfo    *aInfo );

    /* Type converter : protocol operation 을 통해 result set meta 를 
       얻은 후에 type converter 를 생성해야 한다. */
    IDE_RC          createTypeConverter( dkpColumn  *aColMetaArr,
                                         UInt         aColCount );

    IDE_RC          getTypeConverter( dkdTypeConverter ** aTypeConverter );

    IDE_RC          destroyTypeConverter();

    /* Error information */
    IDE_RC          getErrorInfoFromProtocol( dksSession    *aSession,
                                              UInt           aSessionId );

    /* Set remote statement string */
    IDE_RC          setStmtStr( SChar   *aStmtStr );

    /* Set/Get */
    inline dktErrorInfo *       getErrorInfo();
    
    inline dkdDataMgr *         getDataMgr();
    inline dkdRemoteTableMgr *  getRemoteTableMgr();

    inline void                 setStmtType( UInt   aStmtType );
    inline UInt                 getStmtType();

    inline void                 setStmtId( SLong    aStmtId );
    inline SLong                getStmtId();

    inline void                 setStmtResult( UInt aResult );
    inline UInt                 getStmtResult();

    inline dkoLink *            getExecutedLinkObj();
};

inline dkoLink *    dktRemoteStmt::getExecutedLinkObj()
{
    return &mExecutedLinkObj;
}

inline dktErrorInfo *  dktRemoteStmt::getErrorInfo()
{
    return &mErrorInfo;
}

inline dkdDataMgr *  dktRemoteStmt::getDataMgr()
{
    return mDataMgr;
}

inline dkdRemoteTableMgr *  dktRemoteStmt::getRemoteTableMgr()
{
    return mRemoteTableMgr;
}

inline void dktRemoteStmt::setStmtType( UInt   aStmtType )
{
    mStmtType = aStmtType;
}

inline UInt dktRemoteStmt::getStmtType()
{
    return mStmtType;
}

inline void dktRemoteStmt::setStmtId( SLong aStmtId )
{
    mId = aStmtId;
}

inline SLong dktRemoteStmt::getStmtId()
{
    return mId;
}

inline void dktRemoteStmt::setStmtResult( UInt aResult )
{
    mResult = aResult;
}

inline UInt dktRemoteStmt::getStmtResult()
{
    return mResult;
}


#endif /* _O_DKT_REMOTE_STMT_H_ */

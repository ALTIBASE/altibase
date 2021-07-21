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

#include <idvProfile.h>
#include <qci.h>
#include <mmErrorCode.h>
#include <mmcConv.h>
#include <mmcSession.h>
#include <mmcStatement.h>
#include <mmcStatementManager.h>
#include <mmtServiceThread.h>
#include <mmtThreadManager.h>

typedef struct mmtCmsBindContext
{
    cmtAny       *mSource;
    mmcSession   *mSession;
    qciStatement *mQciStmt;
    UChar        *mCollectionData;
    UInt          mCollectionSize;
    UInt          mCursor; // bug-27621: pointer to UInt
} mmtCmsBindContext;


/*
 * PROJ-1697
 * Array Execute result의 통신 전송량을 축소하기 위한 구조체
 */
typedef struct mmtBindDataListResult
{
    UShort mFromRowNumber;
    UInt   mResultSetCount;
    UInt   mAffectedRowCount;
    UShort mResultCount;
} mmtBindDataListResult;

#define INIT_BIND_DATA_LIST_RESULT(x, n, s, a)  \
{                                               \
    (x)->mFromRowNumber    = n;                 \
    (x)->mResultSetCount   = s;                 \
    (x)->mAffectedRowCount = a;                 \
    (x)->mResultCount      = 0;                 \
}

#define FLUSH_BIND_DATA_LIST_RESULT(x, p, g)                        \
{                                                                   \
    if( (x)->mResultCount > 0 )                                     \
    {                                                               \
        IDE_TEST( answerParamDataInListResult(                  \
                      (p),                                          \
                      (g),                                          \
                      (x)->mFromRowNumber,                          \
                      (x)->mFromRowNumber + (x)->mResultCount - 1,  \
                      (x)->mResultSetCount,                         \
                      (x)->mAffectedRowCount )                      \
                  != IDE_SUCCESS );                                 \
        (x)->mResultCount = 0;                                      \
    }                                                               \
}

#define DIFF_BIND_DATA_LIST_RESULT(x, s, a)                         \
( ((x)->mResultSetCount != s) || ((x)->mAffectedRowCount != a) )

#define ANSWER_BIND_DATA_LIST_RESULT(x, p, g, n, s, a)     \
{                                                          \
    if( (x)->mResultCount > 0 )                            \
    {                                                      \
        if( DIFF_BIND_DATA_LIST_RESULT( x, s, a ) )        \
        {                                                  \
            FLUSH_BIND_DATA_LIST_RESULT( x, p, g );        \
            INIT_BIND_DATA_LIST_RESULT( x, n, s, a );      \
        }                                                  \
    }                                                      \
    else                                                   \
    {                                                      \
        INIT_BIND_DATA_LIST_RESULT( x, n, s, a );          \
    }                                                      \
    (x)->mResultCount++;                                   \
}

static IDE_RC answerColumnInfoGetResult(cmiProtocolContext        *aProtocolContext,
                                            cmpArgDBColumnInfoGetA5 *aReqArg,
                                            qciBindColumn             *aBindColumn)
{
    cmiProtocol                      sProtocol;
    cmpArgDBColumnInfoGetResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, ColumnInfoGetResult);

    sArg->mStatementID  = aReqArg->mStatementID;
    sArg->mResultSetID  = aReqArg->mResultSetID;
    sArg->mColumnNumber = aReqArg->mColumnNumber;
    sArg->mDataType     = aBindColumn->mType;
    sArg->mLanguage     = aBindColumn->mLanguage;
    sArg->mArguments    = aBindColumn->mArguments;
    sArg->mPrecision    = aBindColumn->mPrecision;
    sArg->mScale        = aBindColumn->mScale;
    sArg->mNullableFlag = aBindColumn->mFlag;

    IDE_TEST(cmtVariableSetData(&sArg->mDisplayName,
                                (UChar *)aBindColumn->mDisplayName,
                                aBindColumn->mDisplayNameSize) != IDE_SUCCESS);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerColumnInfoGetListResult(cmiProtocolContext        *aProtocolContext,
                                                cmpArgDBColumnInfoGetA5 *aReqArg,
                                                UInt                       aColumnCount,
                                                UChar                     *aCollectionData,
                                                UInt                       aCollectionSize)
{
    cmiProtocol                          sProtocol;
    cmpArgDBColumnInfoGetListResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, ColumnInfoGetListResult);

    sArg->mStatementID  = aReqArg->mStatementID;
    sArg->mResultSetID  = aReqArg->mResultSetID;
    sArg->mColumnCount  = aColumnCount;

    if( cmiCheckInVariable( aProtocolContext, aCollectionSize ) == ID_TRUE )
    {
        cmtCollectionWriteInVariable( &sArg->mListData,
                                      aCollectionData,
                                      aCollectionSize );
    }
    else
    {
        cmtCollectionWriteVariable( &sArg->mListData,
                                    aCollectionData,
                                    aCollectionSize );
    }
    
    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerColumnInfoSetResult(cmiProtocolContext *aProtocolContext)
{
    cmiProtocol                      sProtocol;
    cmpArgDBColumnInfoSetResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, ColumnInfoSetResult);

    ACP_UNUSED(sArg);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerParamInfoGetResult(cmiProtocolContext       *aProtocolContext,
                                           cmpArgDBParamInfoGetA5 *aReqArg,
                                           qciBindParam             *aBindParam)
{
    cmiProtocol                     sProtocol;
    cmpArgDBParamInfoGetResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, ParamInfoGetResult);

    sArg->mStatementID  = aReqArg->mStatementID;
    sArg->mParamNumber  = aReqArg->mParamNumber;
    sArg->mDataType     = aBindParam->type;
    sArg->mLanguage     = aBindParam->language;
    sArg->mArguments    = aBindParam->arguments;
    sArg->mPrecision    = aBindParam->precision;
    sArg->mScale        = aBindParam->scale;
    sArg->mInOutType    = aBindParam->inoutType;
    sArg->mNullableFlag = 0; /* BUGBUG */

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerParamInfoSetResult(cmiProtocolContext *aProtocolContext)
{
    cmiProtocol                     sProtocol;
    cmpArgDBParamInfoSetResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, ParamInfoSetResult);

    ACP_UNUSED(sArg);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerParamInfoSetListResult(cmiProtocolContext *aProtocolContext)
{
    cmiProtocol                         sProtocol;
    cmpArgDBParamInfoSetListResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, ParamInfoSetListResult);

    ACP_UNUSED(sArg);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerParamDataInListResult(cmiProtocolContext          *aProtocolContext,
                                              cmpArgDBParamDataInListA5 *aDataInList,
                                              UShort                       aFromRowNumber,
                                              UShort                       aToRowNumber,
                                              UInt                         aResultSetCount,
                                              ULong                        aAffectedRowCount )
{
    cmiProtocol                        sProtocol;
    cmpArgDBParamDataInListResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, ParamDataInListResult);

    sArg->mStatementID      = aDataInList->mStatementID;
    sArg->mFromRowNumber    = aFromRowNumber;
    sArg->mToRowNumber      = aToRowNumber;
    sArg->mAffectedRowCount = aAffectedRowCount;
    sArg->mResultSetCount   = aResultSetCount;
    
    IDE_TEST(cmiWriteProtocol(aProtocolContext,
                              &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

// bug-28259: ipc needs paramDataInResult
// ipc인 경우 protocol에 대한 요청/응답 짝이 맞아야 한다.
// 그래서 paramDataIn에 대한 dummy 응답 protocol 추가.
static IDE_RC answerParamDataInResult(cmiProtocolContext *aProtocolContext)
{
    cmiProtocol                        sProtocol;
    cmpArgDBParamDataInResultA5       *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, ParamDataInResult);

    ACP_UNUSED(sArg);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC readBindParamInfoCallback( void         * aBindContext,
                                         qciBindParam * aBindParam )
{
    cmpCollectionDBParamInfoSetA5    sParamInfo;
    mmtCmsBindContext               *sBindContext;

    sBindContext = (mmtCmsBindContext*)aBindContext;
    
    // bug-27621: mCursor: UInt pointer -> UInt
    IDE_TEST( sBindContext->mCollectionSize <= sBindContext->mCursor);

    cmtCollectionReadParamInfoSetNext( sBindContext->mCollectionData,
                                       &(sBindContext->mCursor),
                                       (void*)&sParamInfo );

    aBindParam->id        = sParamInfo.mParamNumber - 1;
    aBindParam->type      = sParamInfo.mDataType;
    aBindParam->language  = sParamInfo.mLanguage;
    aBindParam->arguments = sParamInfo.mArguments;
    aBindParam->precision = sParamInfo.mPrecision;
    aBindParam->scale     = sParamInfo.mScale;
    aBindParam->inoutType = sParamInfo.mInOutType;

    if( aBindParam->language == 0 )
    {
        aBindParam->language = sBindContext->mSession->getLanguage()->id;
    }
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC setParamDataCallback(idvSQL * /* aStatistics*/,
                                   void   * aBindParam,
                                   void   * aTarget,
                                   UInt     aTargetSize,
                                   void   * aTemplate,
                                   void   * aBindContext)
{
    qciBindParam *sBindParam = (qciBindParam *) aBindParam;
    
    mmtCmsBindContext *sBindContext = (mmtCmsBindContext *)aBindContext;

    IDE_TEST(mmcConv::convertFromCM( sBindParam,
                                     aTarget,
                                     aTargetSize,
                                     sBindContext->mSource,
                                     aTemplate,
                                     sBindContext->mSession )
             != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC readBindParamDataCallback( void  * aBindContext )
{
    mmtCmsBindContext * sBindContext = (mmtCmsBindContext *) aBindContext;

    IDE_TEST( cmtAnyInitialize( sBindContext->mSource ) != IDE_SUCCESS );

    // bug-27621: mCursor: UInt pointer -> UInt
    IDE_TEST( cmtCollectionReadAnyNext( sBindContext->mCollectionData,
                                        &(sBindContext->mCursor),
                                        sBindContext->mSource )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC profVariableGetDataCallbackA5(cmtVariable * /* aVariable*/,
                                   UInt          /* aOffset*/,
                                   UInt         aSize,
                                   UChar       *aData,
                                   void        *aContext)
{
    mmtCmsBindProfContext *sContext = (mmtCmsBindProfContext *)aContext;

    if (sContext->mDescPos < (sContext->mMaxDesc - 1))
    {
        sContext->mDescInfo[sContext->mDescPos].mData   = aData;
        sContext->mDescInfo[sContext->mDescPos].mLength = aSize;
        sContext->mTotalSize                           += aSize;
        sContext->mDescPos++;
    }
    else
    {
        // prevent buffer overrun, just skip.
    }

    return IDE_SUCCESS;
}

UInt profWriteBindCallbackA5(void            *aBindData,
                           idvProfDescInfo *aInfo,
                           UInt             aInfoBegin,
                           UInt             aInfoCount,
                           UInt             aCurrSize)
{
    UInt           sEndDescNumber = aInfoBegin;
    UInt           sTotalSize     = aCurrSize;
    cmtAny        *sAny           = (cmtAny *)aBindData;
    cmtVariable   *sVariable      = NULL;
    cmtInVariable *sInVariable    = NULL;

    /* ------------------------------------------------
     *  Anyway, cmtAny should be logged.
     * ----------------------------------------------*/

    aInfo[sEndDescNumber].mData   = aBindData;
    aInfo[sEndDescNumber].mLength = ID_SIZEOF(cmtAny);
    sTotalSize                   += ID_SIZEOF(cmtAny);
    sEndDescNumber++;

    switch(cmtAnyGetType(sAny))
    {
        /* ------------------------------------------------
         *  Variable Size Data
         * ----------------------------------------------*/

        case CMT_ID_VARIABLE:
        case CMT_ID_BINARY:
            sVariable = (cmtVariable *)(&(sAny->mValue.mVariable));
            break;

        case CMT_ID_IN_VARIABLE:
        case CMT_ID_IN_BINARY:
            sInVariable = (cmtInVariable *)(&(sAny->mValue.mInVariable));
            break;

        case CMT_ID_BIT:
            sVariable = (cmtVariable *)(&(sAny->mValue.mBit.mData));
            break;

        case CMT_ID_IN_BIT:
            sInVariable = (cmtInVariable *)(&(sAny->mValue.mInBit.mData));
            break;

        case CMT_ID_NIBBLE:
            sVariable = (cmtVariable *)(&(sAny->mValue.mNibble.mData));
            break;

        case CMT_ID_IN_NIBBLE:
            sInVariable = (cmtInVariable *)(&(sAny->mValue.mInNibble.mData));
            break;

        /* ------------------------------------------------
         * fixed Size Data
         * ----------------------------------------------*/
        case CMT_ID_NULL:
        case CMT_ID_SINT8:
        case CMT_ID_UINT8:
        case CMT_ID_SINT16:
        case CMT_ID_UINT16:
        case CMT_ID_SINT32:
        case CMT_ID_UINT32:
        case CMT_ID_SINT64:
        case CMT_ID_UINT64:
        case CMT_ID_FLOAT32:
        case CMT_ID_FLOAT64:
        case CMT_ID_DATETIME:
        case CMT_ID_NUMERIC:
        case CMT_ID_LOBLOCATOR:
            break;

        /* ------------------------------------------------
         *  Invalid Data Type
         * ----------------------------------------------*/
        default:
            ideLog::log(IDE_SERVER_0,
                        "[WARNING] Invalid Bind"
                        " Data Type Profiling ==> %u\n",
                        (UInt)cmtAnyGetType(sAny));
            break;
    }

    if (sVariable != NULL)
    {
        mmtCmsBindProfContext sContext;

        sContext.mDescInfo  = aInfo;
        sContext.mDescPos   = sEndDescNumber;
        sContext.mTotalSize = sTotalSize;
        sContext.mMaxDesc   = aInfoCount;

        if (cmtVariableGetDataWithCallback(sVariable,
                                           profVariableGetDataCallbackA5,
                                           &sContext) == IDE_SUCCESS)
        {
            IDE_ASSERT(sContext.mDescPos < aInfoCount);

            sEndDescNumber   = sContext.mDescPos;
            sTotalSize       = sContext.mTotalSize;
        }
        else
        {
            // [ERROR ?? ] just header logging : and skip.
        }
    }

    if (sInVariable != NULL)
    {
        sTotalSize += sInVariable->mSize;
        aInfo[sEndDescNumber].mData = sInVariable->mData;
        aInfo[sEndDescNumber].mLength = sInVariable->mSize;
        sEndDescNumber++;
    }

    aInfo[sEndDescNumber].mData = NULL;

    return sTotalSize;
}


IDE_RC mmtServiceThread::columnInfoGetProtocolA5(cmiProtocolContext *aProtocolContext,
                                                   cmiProtocol        *aProtocol,
                                                   void               *aSessionOwner,
                                                   void               *aUserContext)
{
    cmpArgDBColumnInfoGetA5 *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ColumnInfoGet);
    mmcTask                   *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread          *sThread = (mmtServiceThread *)aUserContext;
    mmcSession                *sSession;
    mmcStatement              *sStatement;
    qciBindColumn              sBindColumn;
    UShort                     sColumnCount;
    UShort                     sResultSetID;
    mmcResultSet              * sResultSet;
    mmcStatement              * sResultSetStmt;
    UChar                     * sCollectionData;
    cmpCollectionDBColumnInfoGetResultA5 sResult;
    UInt                                   sCursor = 0;
    
    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sArg->mStatementID,
                           sThread) != IDE_SUCCESS);

    sResultSetID = sArg->mResultSetID;

    sResultSet = sStatement->getResultSet( sResultSetID );

    if( sResultSet == NULL )
    {
        IDE_TEST_RAISE(sResultSetID != MMC_RESULTSET_FIRST,
                       NoColumn);

        sResultSetStmt = sStatement;
    }
    else
    {
        // BUG-20756 sResultSet->mResultSetStmt 이 NULL이 될수 있다.
        if(sResultSet->mResultSetStmt == NULL)
        {
            sResultSetStmt = sStatement;
        }
        else
        {
            sResultSetStmt = (mmcStatement*)sResultSet->mResultSetStmt;
        }
    }
    
    sColumnCount = qci::getColumnCount(sResultSetStmt->getQciStmt());

    IDE_TEST_RAISE(sColumnCount == 0, NoColumn);

    // BUG-17544
    // Prepare 후 컬럼 정보를 보냈으면 다시 컬럼 정보를 보내지 않는다.
    IDE_TEST_RAISE(sStatement->getSendBindColumnInfo() == ID_TRUE,NoError);

    if (sArg->mColumnNumber == 0)
    {
        if( sSession->getHasClientListChannel() == ID_TRUE )
        {
            
            
            // cmiGetBindColumnInfoStructSize()에서 ALIAS 또는 컬럼 이름의
            // DISPLAY SIZE를 최대 50으로 가정한다.
            IDE_TEST( sSession->allocChunk( 
                            IDL_MAX( sColumnCount * cmiGetBindColumnInfoStructSize(),
                            (MMC_DEFAULT_COLLECTION_BUFFER_SIZE) ) )
                            != IDE_SUCCESS);

            sCollectionData = sSession->getChunk();
            sCursor = 0;
            
            for (sBindColumn.mId = 0;
                 sBindColumn.mId < sColumnCount;
                 sBindColumn.mId++)
            {
                IDE_TEST(cmtAnyInitialize( &sResult.mDisplayName )
                         != IDE_SUCCESS);
                IDE_TEST(qci::getBindColumnInfo(sResultSetStmt->getQciStmt(),
                                                &sBindColumn)
                         != IDE_SUCCESS);

                /* BUG-37755 compatibility test */
                sBindColumn.mFlag    &= QCI_BIND_FLAGS_NULLABLE_MASK;

                sResult.mDataType     = sBindColumn.mType;
                sResult.mLanguage     = sBindColumn.mLanguage;
                sResult.mArguments    = sBindColumn.mArguments;
                sResult.mPrecision    = sBindColumn.mPrecision;
                sResult.mScale        = sBindColumn.mScale;
                sResult.mNullableFlag = sBindColumn.mFlag;
                
                // BUG-24075 [MM] BIND할 때,
                //           DISPLAY SIZE를 최대 50으로 잡아야 합니다
                IDE_TEST( cmtAnyWriteInVariable( &sResult.mDisplayName,
                                                 (UChar *)sBindColumn.mDisplayName,
                                                 IDL_MIN(sBindColumn.mDisplayNameSize,
                                                         MMC_MAX_DISPLAY_NAME_SIZE) )
                          != IDE_SUCCESS );
                
                cmtCollectionWriteColumnInfoGetResult( sCollectionData,
                                                       & sCursor,
                                                       (void*)&sResult );
            }
            
            IDE_TEST( answerColumnInfoGetListResult( aProtocolContext,
                                                         sArg,
                                                         sColumnCount,
                                                         sCollectionData,
                                                         sCursor )
                      != IDE_SUCCESS );
        }
        else
        {
            for (sBindColumn.mId = 0;
                 sBindColumn.mId < sColumnCount;
                 sBindColumn.mId++)
            {
                IDE_TEST(qci::getBindColumnInfo(sResultSetStmt->getQciStmt(),
                                                &sBindColumn)
                         != IDE_SUCCESS);

                /* BUG-37755 compatibility test */
                sBindColumn.mFlag &= QCI_BIND_FLAGS_NULLABLE_MASK;

                sArg->mColumnNumber = sBindColumn.mId + 1;
                IDE_TEST(answerColumnInfoGetResult(aProtocolContext,
                                                       sArg,
                                                       &sBindColumn)
                         != IDE_SUCCESS);
            }
        }
    }
    else
    {
        IDE_TEST_RAISE(sArg->mColumnNumber > sColumnCount, InvalidColumn);

        sBindColumn.mId = sArg->mColumnNumber - 1;

        IDE_TEST(qci::getBindColumnInfo(sResultSetStmt->getQciStmt(),
                                        &sBindColumn)
                 != IDE_SUCCESS);

        /* BUG-37755 compatibility test */
        sBindColumn.mFlag &= QCI_BIND_FLAGS_NULLABLE_MASK;

        IDE_TEST(answerColumnInfoGetResult(aProtocolContext,
                                               sArg,
                                               &sBindColumn)
                 != IDE_SUCCESS);
    }
    
    // BUG-17544
    // Prepare 후 컬럼 정보를 보냈으면 Execute 후에는 컬럼 정보를 보내지 않도록 설정한다.
    // JDBC의 경우 Execute 갯수만큼 컬럼 정보를 요청할 수 있다.
    if (sStatement->getStmtState() == MMC_STMT_STATE_PREPARED)
    {
        sStatement->setSendBindColumnInfo(ID_TRUE);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(NoColumn);
    {
        IDE_SET(ideSetErrorCode(mmERR_IGNORE_NO_COLUMN));
    }
    IDE_EXCEPTION(InvalidColumn);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_BIND_COLUMN_NUMBER));
    }
    IDE_EXCEPTION(NoError);
    {
        IDE_SET(ideSetErrorCode(mmERR_IGNORE_NO_ERROR));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, ColumnInfoGet),
                                      0);
}

IDE_RC mmtServiceThread::columnInfoSetProtocolA5(cmiProtocolContext *aProtocolContext,
                                                   cmiProtocol        *aProtocol,
                                                   void               *aSessionOwner,
                                                   void               *aUserContext)
{
    cmpArgDBColumnInfoSetA5 *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ColumnInfoSet);
    mmcTask                   *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread          *sThread = (mmtServiceThread *)aUserContext;
    mmcSession                *sSession;
    mmcStatement              *sStatement;
    mmcStatement              *sResultSetStmt;
    qciBindColumn              sBindColumn;
    UShort                     sColumnCount;
    mmcResultSet              *sResultSet;
    
    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sArg->mStatementID,
                           sThread) != IDE_SUCCESS);

    // BUGBUG
    IDE_TEST_RAISE(checkStatementState(sStatement, MMC_STMT_STATE_EXECUTED) != IDE_SUCCESS,
                   NoCursor);

    sResultSet = sStatement->getResultSet( sArg->mResultSetID );
    // bug-26977: codesonar: resultset null ref
    IDE_TEST_RAISE( sResultSet == NULL, NoCursor );

    sResultSetStmt = (mmcStatement*)sResultSet->mResultSetStmt;
    // bug-26977: codesonar: resultset null ref
    // null이 가능한지는 모르겠지만, 방어코드추가.
    IDE_TEST_RAISE( sResultSetStmt == NULL, NoCursor );
    
    sColumnCount = qci::getColumnCount(sResultSetStmt->getQciStmt());

    IDE_TEST_RAISE(sArg->mColumnNumber > sColumnCount, NoColumn);
    IDE_TEST_RAISE(sArg->mColumnNumber == 0, InvalidColumn);

    sBindColumn.mId        = sArg->mColumnNumber - 1;
    sBindColumn.mType      = sArg->mDataType;
    sBindColumn.mLanguage  = sArg->mLanguage ? sArg->mLanguage : sSession->getLanguage()->id;
    sBindColumn.mArguments = sArg->mArguments;
    sBindColumn.mPrecision = sArg->mPrecision;
    sBindColumn.mScale     = sArg->mScale;
    sBindColumn.mFlag      = 0;

    IDE_TEST(qci::setBindColumnInfo(sResultSetStmt->getQciStmt(), &sBindColumn) != IDE_SUCCESS);

    return answerColumnInfoSetResult(aProtocolContext);

    IDE_EXCEPTION(NoCursor);
    {
        IDE_SET(ideSetErrorCode(mmERR_IGNORE_NO_CURSOR));
    }
    IDE_EXCEPTION(NoColumn);
    {
        IDE_SET(ideSetErrorCode(mmERR_IGNORE_NO_COLUMN));
    }
    IDE_EXCEPTION(InvalidColumn);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_BIND_COLUMN_NUMBER));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, ColumnInfoSet),
                                      0);
}

IDE_RC mmtServiceThread::paramInfoGetProtocolA5(cmiProtocolContext *aProtocolContext,
                                                  cmiProtocol        *aProtocol,
                                                  void               *aSessionOwner,
                                                  void               *aUserContext)
{
    cmpArgDBParamInfoGetA5 *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamInfoGet);
    mmcTask                  *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread         *sThread = (mmtServiceThread *)aUserContext;
    mmcSession               *sSession;
    mmcStatement             *sStatement;
    qciBindParam              sBindParam;
    UShort                    sParamCount;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sArg->mStatementID,
                           sThread) != IDE_SUCCESS);

    IDE_TEST(checkStatementState(sStatement, MMC_STMT_STATE_PREPARED) != IDE_SUCCESS);

    sParamCount = qci::getParameterCount(sStatement->getQciStmt());

    if (sArg->mParamNumber == 0)
    {
        IDE_TEST_RAISE(sParamCount == 0, NoParameter);

        for (sBindParam.id = 0; sBindParam.id < sParamCount; sBindParam.id++)
        {
            IDE_TEST(qci::getBindParamInfo(sStatement->getQciStmt(), &sBindParam) != IDE_SUCCESS);

            sArg->mParamNumber = sBindParam.id + 1;

            IDE_TEST(answerParamInfoGetResult(aProtocolContext,
                                                  sArg,
                                                  &sBindParam) != IDE_SUCCESS);
        }
    }
    else
    {
        IDE_TEST_RAISE(sArg->mParamNumber > sParamCount, InvalidParameter);

        sBindParam.id = sArg->mParamNumber - 1;

        IDE_TEST(qci::getBindParamInfo(sStatement->getQciStmt(), &sBindParam) != IDE_SUCCESS);

        IDE_TEST(answerParamInfoGetResult(aProtocolContext, sArg, &sBindParam) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(NoParameter);
    {
        IDE_SET(ideSetErrorCode(mmERR_IGNORE_NO_PARAMETER));
    }
    IDE_EXCEPTION(InvalidParameter);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_BIND_PARAMETER_NUMBER));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, ParamInfoGet),
                                      0);
}

IDE_RC mmtServiceThread::paramInfoSetProtocolA5(cmiProtocolContext *aProtocolContext,
                                                  cmiProtocol        *aProtocol,
                                                  void               *aSessionOwner,
                                                  void               *aUserContext)
{
    cmpArgDBParamInfoSetA5 *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamInfoSet);
    mmcTask                  *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread         *sThread = (mmtServiceThread *)aUserContext;
    mmcSession               *sSession;
    mmcStatement             *sStatement;
    qciBindParam              sBindParam;
    UShort                    sParamCount;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sArg->mStatementID,
                           sThread) != IDE_SUCCESS);

    IDE_TEST_RAISE(sStatement->getStmtState() != MMC_STMT_STATE_PREPARED,
                   InvalidStatementState);

    sParamCount = qci::getParameterCount(sStatement->getQciStmt());

    IDE_TEST_RAISE(sArg->mParamNumber > sParamCount, NoParameter);
    IDE_TEST_RAISE(sArg->mParamNumber == 0, InvalidParameter);

    if (sStatement->getBindState() > MMC_STMT_BIND_NONE)
    {
        IDE_TEST(qci::clearStatement(sStatement->getQciStmt(),
                                     NULL,
                                     QCI_STMT_STATE_PREPARED) != IDE_SUCCESS);

        sStatement->setBindState(MMC_STMT_BIND_NONE);
    }

    sBindParam.id        = sArg->mParamNumber - 1;
    sBindParam.type      = sArg->mDataType;
    sBindParam.language  = sArg->mLanguage ? sArg->mLanguage : sSession->getLanguage()->id;
    sBindParam.arguments = sArg->mArguments;
    sBindParam.precision = sArg->mPrecision;
    sBindParam.scale     = sArg->mScale;
    sBindParam.inoutType = sArg->mInOutType;

    IDE_TEST(qci::setBindParamInfo(sStatement->getQciStmt(), &sBindParam) != IDE_SUCCESS);

    if ( sArg->mParamNumber == sParamCount )
    {
        sStatement->setBindState(MMC_STMT_BIND_INFO);
    }
    else
    {
        // Nothing to do
    }

    return answerParamInfoSetResult(aProtocolContext);

    IDE_EXCEPTION(InvalidStatementState);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION(NoParameter);
    {
        IDE_SET(ideSetErrorCode(mmERR_IGNORE_NO_PARAMETER));
    }
    IDE_EXCEPTION(InvalidParameter);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_BIND_PARAMETER_NUMBER));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, ParamInfoSet),
                                      0);
}

/*
 * PROJ-1697 : bind parameter의 information list를 처리하는 프로토콜
 */
IDE_RC mmtServiceThread::paramInfoSetListProtocolA5(cmiProtocolContext *aProtocolContext,
                                                      cmiProtocol        *aProtocol,
                                                      void               *aSessionOwner,
                                                      void               *aUserContext)
{
    cmpArgDBParamInfoSetListA5 *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamInfoSetList);
    mmcTask                      *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread             *sThread = (mmtServiceThread *)aUserContext;
    mmcSession                   *sSession;
    mmcStatement                 *sStatement;
    UChar                        *sCollectionData;
    mmtCmsBindContext             sBindContext;
    
    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sArg->mStatementID,
                           sThread) != IDE_SUCCESS);

    IDE_TEST_RAISE(sStatement->getStmtState() != MMC_STMT_STATE_PREPARED,
                   InvalidStatementState);

    if (sStatement->getBindState() > MMC_STMT_BIND_NONE)
    {
        IDE_TEST(qci::clearStatement(sStatement->getQciStmt(),
                                     NULL,
                                     QCI_STMT_STATE_PREPARED) != IDE_SUCCESS);

        sStatement->setBindState(MMC_STMT_BIND_NONE);
    }
    
    if( cmtCollectionGetType( &sArg->mListData ) == CMT_ID_VARIABLE )
    {
        // CHUNK가 부족한 경우에만 할당됨.
        IDE_TEST( sSession->allocChunk( cmtCollectionGetSize(&sArg->mListData) )
                  != IDE_SUCCESS );

        sCollectionData = sSession->getChunk();
    
        IDE_TEST( cmtCollectionCopyData( &sArg->mListData, sCollectionData )
                  != IDE_SUCCESS );
    }
    else
    {
        sCollectionData = cmtCollectionGetData( &sArg->mListData );
    }

    sBindContext.mSession        = sSession;
    sBindContext.mCollectionData = sCollectionData;
    sBindContext.mCollectionSize = cmtCollectionGetSize(&sArg->mListData);
    // bug-27621: mCursor: UInt pointer -> UInt
    // 지역변수 sCursor 주소지정 제거.
    sBindContext.mCursor         = 0;

    IDE_TEST( qci::setBindParamInfoSet( sStatement->getQciStmt(),
                                       &sBindContext,
                                       readBindParamInfoCallback )
              != IDE_SUCCESS);

    sStatement->setBindState(MMC_STMT_BIND_INFO);

    return answerParamInfoSetListResult(aProtocolContext);

    IDE_EXCEPTION(InvalidStatementState);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, ParamInfoSetList),
                                      0);
}

IDE_RC mmtServiceThread::paramDataInProtocolA5(cmiProtocolContext *aProtocolContext,
                                                 cmiProtocol        *aProtocol,
                                                 void               *aSessionOwner,
                                                 void               *aUserContext)
{
    cmpArgDBParamDataInA5 *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataIn);
    mmcTask                 *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread        *sThread = (mmtServiceThread *)aUserContext;
    mmcSession              *sSession;
    mmcStatement            *sStatement;
    mmtCmsBindContext        sBindContext;
    UShort                   sParamCount;
    IDE_RC                   sRc = IDE_SUCCESS;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sArg->mStatementID,
                           sThread) != IDE_SUCCESS);

    IDE_TEST_RAISE( (sStatement->getAtomicExecSuccess() != ID_TRUE), SkipExecute);

    IDE_TEST_RAISE(sStatement->getStmtState() != MMC_STMT_STATE_PREPARED,
                   InvalidStatementState);

    sParamCount = qci::getParameterCount(sStatement->getQciStmt());

    IDE_TEST_RAISE(sArg->mParamNumber > sParamCount, NoParameter);
    IDE_TEST_RAISE(sArg->mParamNumber == 0, InvalidParameter);

    if (sStatement->getBindState() > MMC_STMT_BIND_INFO)
    {
        sStatement->setBindState(MMC_STMT_BIND_INFO);
    }
    else
    {
        // Nothing to do.
    }

    sBindContext.mSource  = &sArg->mData;
    sBindContext.mSession = sSession;
    sBindContext.mQciStmt = sStatement->getQciStmt();

    if(qci::setBindParamDataOld( sStatement->getQciStmt(),
                                 sArg->mParamNumber - 1,
                                 setParamDataCallback,
                                 &sBindContext ) != IDE_SUCCESS)
    {
        // BUG-22712 VC6 컴파일 오류 수정
        IDE_TEST(sStatement->isAtomic() != ID_TRUE);

        // BUG-21489
        sStatement->setAtomicLastErrorCode(ideGetErrorCode());
        sStatement->setAtomicExecSuccess(ID_FALSE); /* BUG-40245 */
        IDE_RAISE(SkipExecute);
    }

    // bug-25312: prepare 이후에 autocommit을 off에서 on으로 변경하고
    // bind 하면 stmt->mTrans 가 null이어서 segv.
    // autocommit off인 경우 beginSession에서 session에 mTrans가 할당되고
    // autocommit on 인 경우 prepare 할때, 또는 execute 시작시 
    // statement에 mTrans가 할당된다
    // 따라서 prepare 이후에 autocommit을 off에서 on으로 바꾸면
    // session에 대한 mTrans는 할당되어 있지만 statement에 대한 mTrans
    // 는 null이고, bind시에는 statement의 mTrans(null)를 사용하여 segv.
    // 변경: stmt->mTrans를 구하여 null이면 TransID로 0을 넘기도로 수정
    // (transID 0은 mTrans null과 동일한 의미이다)
    // ex) conn.setAutoCommit(false);
    //     conn.prepareStatement("insert..");
    //     conn.setAutoCommit(true);
    //     pstmt.setString(..);
    //     pstmt.execute(); => bindDataIn 할때 segv
    if ((idvProfile::getProfFlag() & IDV_PROF_TYPE_BIND_FLAG) == IDV_PROF_TYPE_BIND_FLAG)
    {
        mmcTransObj *sTrans = sSession->getTransPtr(sStatement);
        smTID sTransID = (sTrans != NULL) ? mmcTrans::getTransID(sTrans) : 0;

        idvProfile::writeBindA5( (void *)(sBindContext.mSource),
                sStatement->getSessionID(),
                sStatement->getStmtID(),
                sTransID, 
                profWriteBindCallbackA5 );
    }

    if ( qci::isLastParamData ( sStatement->getQciStmt(),
                                sArg->mParamNumber - 1 ) )
    {
        sStatement->setBindState(MMC_STMT_BIND_DATA);
    }
    else
    {
        // Nothing to do
    }

    IDE_EXCEPTION_CONT(SkipExecute);

    // bug-28259: ipc needs paramDataInResult
    // ipc인 경우 protocol에 대한 요청/응답 짝이 맞아야 한다.
    // client가 응답 확인 없이 요청만 송신하면 버퍼를 덮어쓰게 됨.
    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPC)
    {
        sRc = answerParamDataInResult(aProtocolContext);
    }
    return sRc;

    IDE_EXCEPTION(InvalidStatementState);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION(NoParameter);
    {
        IDE_SET(ideSetErrorCode(mmERR_IGNORE_NO_PARAMETER));
    }
    IDE_EXCEPTION(InvalidParameter);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_BIND_PARAMETER_NUMBER));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, ParamDataIn),
                                      0);
}

/*
 * PROJ-1697
 * bind parameter의 data list를 처리하는 프로토콜
 */
IDE_RC mmtServiceThread::paramDataInListProtocolA5(cmiProtocolContext *aProtocolContext,
                                                     cmiProtocol        *aProtocol,
                                                     void               *aSessionOwner,
                                                     void               *aUserContext)
{
    cmpArgDBParamDataInListA5 *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataInList);
    mmcTask                     *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread            *sThread = (mmtServiceThread *)aUserContext;
    mmcSession                  *sSession;
    mmcStatement                *sStatement;
    
    UShort                       sParamCount;
    mmtCmsBindContext            sBindContext;
    UInt                         i = 0; 
    UShort                       j;
    UChar                       *sCollectionData;
    cmtAny                       sAny;
    idBool                       sSuspended          = ID_FALSE;

    UInt                         sResultSetCount     = 0;
    ULong                        sAffectedRowCount   = 0;
    mmtBindDataListResult        sResult;

    mmtCmsBindContext            sProfBindContext;
    cmtAny                       sProfAny;
    qciStatement                *sQciStmt;
    qciBindParam                 sBindParam;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sArg->mStatementID,
                           sThread) != IDE_SUCCESS);

    IDE_TEST_RAISE(sStatement->getStmtState() != MMC_STMT_STATE_PREPARED,
                   InvalidStatementState);

    sParamCount = qci::getParameterCount(sStatement->getQciStmt());

    if( cmtCollectionGetType( &sArg->mListData ) == CMT_ID_VARIABLE )
    {
        // CHUNK가 부족한 경우에만 할당됨.
        IDE_TEST( sSession->allocChunk( cmtCollectionGetSize(&sArg->mListData) )
                  != IDE_SUCCESS );

        sCollectionData = sSession->getChunk();
        
        IDE_TEST( cmtCollectionCopyData( &sArg->mListData,
                                        sCollectionData )
                  != IDE_SUCCESS );
    }
    else
    {
        sCollectionData = cmtCollectionGetData( &sArg->mListData );
    }

    sBindContext.mSource  = &sAny;
    sBindContext.mSession = sSession;
    sBindContext.mQciStmt = sStatement->getQciStmt();
    sBindContext.mCollectionData = sCollectionData;
    // bug-27621: mCursor: UInt pointer -> UInt
    // 지역변수 sCursor 주소지정 제거.
    sBindContext.mCursor = 0;

    // fix BUG-22058
    sProfBindContext.mSource  = &sProfAny;
    sProfBindContext.mSession = sSession;
    sProfBindContext.mQciStmt = sStatement->getQciStmt();
    sProfBindContext.mCollectionData = sCollectionData;
    // bug-27621: mCursor: UInt pointer -> UInt
    // 지역변수 sProfCursor 주소지정 제거.
    sProfBindContext.mCursor = 0;

    INIT_BIND_DATA_LIST_RESULT( &sResult, sArg->mFromRowNumber, 0, 0 );


    // PROJ-1518
    IDE_TEST_RAISE(sThread->atomicCheck( sStatement, &(sArg->mExecuteOption) )  
                                        != IDE_SUCCESS, SkipExecute);
    
    //fix BUG-29824 A array binds should not be permitted with SELECT statement.
    IDE_TEST_RAISE(((sStatement->getStmtType() == QCI_STMT_SELECT) &&(sArg->mExecuteOption == CMP_DB_EXECUTE_ARRAY_EXECUTE)),
                   InvalidArrayBinds);
                   

    for( i = sArg->mFromRowNumber; i <= sArg->mToRowNumber; i++ )
    {
        if( qci::setBindParamDataSetOld( sStatement->getQciStmt(),
                                         &sBindContext,
                                         setParamDataCallback,
                                         readBindParamDataCallback )
            != IDE_SUCCESS )
        {
            // BUG-21489
            // BUG-23054 mmcStatement::setAtomicLastErrorCode() 에서 Valgrind BUG 발생합니다.
            if( sStatement->isAtomic() == ID_TRUE )
            {
                sStatement->setAtomicLastErrorCode(ideGetErrorCode());
                sStatement->setAtomicExecSuccess(ID_FALSE); /* BUG-40245 */
                IDE_RAISE( SkipExecute);
            }

            FLUSH_BIND_DATA_LIST_RESULT( &sResult,
                                         aProtocolContext,
                                         sArg );
            
            IDE_TEST( sThread->answerErrorResultA5(aProtocolContext,
                                                 CMI_PROTOCOL_OPERATION(DB, Execute),
                                                 i)
                      != IDE_SUCCESS );
            continue;
        }

        // fix BUG-22058
        if ((idvProfile::getProfFlag() & IDV_PROF_TYPE_BIND_FLAG) == IDV_PROF_TYPE_BIND_FLAG)
        {
            sQciStmt = sStatement->getQciStmt();
            // bug-25312: prepare 이후에 autocommit을 off에서 on으로 변경하고
            // bind 하면 stmt->mTrans 가 null이어서 segv.
            // 변경: stmt->mTrans를 구하여 null이면 TransID로 0을 넘기도로 수정
            mmcTransObj *sTrans = sSession->getTransPtr(sStatement);
            smTID sTransID = (sTrans != NULL) ? mmcTrans::getTransID(sTrans) : 0;

            for ( j = 0; j < sParamCount; j++)
            {
                sBindParam.id = j;

                IDE_TEST(qci::getBindParamInfo(sQciStmt, &sBindParam) != IDE_SUCCESS);

                // fix BUG-24877 paramDataInListProtocol에서 output param을 Profiling하려 합니다.
                // 클라이언트로부터 INPUT, INPUT_OUTPUT의 데이터만 전송되므로 OUTPUT PARAM은 SKIP한다.
                if ((sBindParam.inoutType == CMP_DB_PARAM_INPUT) ||
                    (sBindParam.inoutType == CMP_DB_PARAM_INPUT_OUTPUT))
                {
                    IDE_TEST(readBindParamDataCallback((void *)&sProfBindContext) != IDE_SUCCESS);

                    idvProfile::writeBindA5( (void *)(sProfBindContext.mSource),
                                           sStatement->getSessionID(),
                                           sStatement->getStmtID(),
                                           sTransID,
                                           profWriteBindCallbackA5 );
                }
            }
        }

        sStatement->setBindState(MMC_STMT_BIND_DATA);

        // 2. execute
        switch (sArg->mExecuteOption)
        {
            case CMP_DB_EXECUTE_NORMAL_EXECUTE:
                sStatement->setArray(ID_FALSE);
                sStatement->setRowNumber(i);

                if( sThread->executeA5(aProtocolContext,
                                     sStatement,
                                     ID_TRUE,
                                     &sSuspended,
                                     NULL, NULL) != IDE_SUCCESS)
                {
                    IDE_TEST( sThread->answerErrorResultA5(
                                  aProtocolContext,
                                  CMI_PROTOCOL_OPERATION(DB, Execute),
                                  i) != IDE_SUCCESS );
                }

                break;

            case CMP_DB_EXECUTE_ARRAY_EXECUTE:
                sStatement->setArray(ID_TRUE);
                sStatement->setRowNumber(i);

                if( sThread->executeA5( aProtocolContext,
                                        sStatement,
                                        ID_FALSE,
                                        &sSuspended,
                                        &sResultSetCount,
                                        &sAffectedRowCount ) != IDE_SUCCESS )
                {
                    FLUSH_BIND_DATA_LIST_RESULT( &sResult,
                                                 aProtocolContext,
                                                 sArg );
                    
                    IDE_TEST( sThread->answerErrorResultA5(
                                  aProtocolContext,
                                  CMI_PROTOCOL_OPERATION(DB, Execute),
                                  i) != IDE_SUCCESS );
                }
                else
                {
                    /* BUG-41437 A server occurs a fatal error while proc is executing */
                    IDE_TEST_RAISE( sResultSetCount > 0, InvalidProcedure );

                    ANSWER_BIND_DATA_LIST_RESULT( &sResult,
                                                  aProtocolContext,
                                                  sArg,
                                                  i,
                                                  sResultSetCount,
                                                  sAffectedRowCount );
                }                
                break;

            case CMP_DB_EXECUTE_ARRAY_BEGIN:
                sStatement->setArray(ID_TRUE);
                sStatement->setRowNumber(0);
                break;

            case CMP_DB_EXECUTE_ARRAY_END:
                sStatement->setArray(ID_FALSE);
                sStatement->setRowNumber(0);
                break;
            // PROJ-1518
            case CMP_DB_EXECUTE_ATOMIC_EXECUTE:
                sStatement->setRowNumber(i);
                // Rebuild Error 를 처리하기 위해서는 Bind가 끝난 시점에서
                // atomicBegin 을 호출해야 한다.
                if( i == 1)
                {
                    IDE_TEST_RAISE((sThread->atomicBegin(sStatement) != IDE_SUCCESS), SkipExecute);
                }

                IDE_TEST_RAISE(sThread->atomicExecuteA5(sStatement, aProtocolContext) != IDE_SUCCESS, SkipExecute);
                break;
            // 다른 옵션은 이곳에 올수 없다.
            case CMP_DB_EXECUTE_ATOMIC_BEGIN:
            case CMP_DB_EXECUTE_ATOMIC_END:
            default:
                IDE_RAISE(InvalidExecuteOption);
                break;
        }
    }
    
    FLUSH_BIND_DATA_LIST_RESULT( &sResult,
                                 aProtocolContext,
                                 sArg );

    IDE_EXCEPTION_CONT(SkipExecute);

    // fix BUG-30990
    // DEQUEUE 대기시 IDE_CM_STOP를 반환
    return (sSuspended == ID_TRUE) ? IDE_CM_STOP : IDE_SUCCESS;

    IDE_EXCEPTION(InvalidStatementState)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION(InvalidExecuteOption)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_EXECUTE_OPTION));
    }
    IDE_EXCEPTION(InvalidArrayBinds)
    {
        //fix BUG-29824 A array binds should not be permitted with SELECT statement.
        IDE_SET(ideSetErrorCode(mmERR_ABORT_InvalidArrayBinds));
    }
    IDE_EXCEPTION(InvalidProcedure)
    {
        /* BUG-41437 A server occurs a fatal error while proc is executing */
        IDE_SET(ideSetErrorCode(mmERR_ABORT_InvalidProcedure));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, ParamDataInList),
                                      0);
}

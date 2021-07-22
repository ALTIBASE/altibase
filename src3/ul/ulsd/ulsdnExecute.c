/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/**********************************************************************
 * $Id: ulsdnExecute.c 00000 2017-02-27 10:26:00Z swhors $
 **********************************************************************/

#include <uln.h>
#include <ulnPrivate.h>
#include <mtcc.h>

#include <ulsd.h>
#include <ulsdnTrans.h>
#include <ulsdnExecute.h>

/* PROJ-2638 shard native linker */
SQLRETURN ulsdExecuteForMtDataRows( ulnStmt      *aStmt,
                                    acp_char_t   *aOutBuf,
                                    acp_uint32_t  aOutBufLen,
                                    acp_uint32_t *aOffSets,
                                    acp_uint32_t *aMaxBytes,
                                    acp_uint16_t  aColumnCount )
{
    SQLRETURN sRet = SQL_ERROR;

    ACI_TEST(aStmt == NULL);
    ACI_TEST((aOutBuf    == NULL) ||
             (aOffSets   == NULL) ||
             (aMaxBytes  == NULL) ||
             (aOutBufLen <= 0));
    ACI_TEST((aColumnCount>ULN_COLUMN_ID_MAXIMUM) ||
             (aColumnCount<=0))

    sRet = ulnExecute(aStmt);

    /* PROJ-2638 shard native linker */
    if ( SQL_SUCCEEDED(sRet) )
    {
        aStmt->mShardStmtCxt.mIsMtDataFetch             = ACP_TRUE;
        aStmt->mShardStmtCxt.mRowDataBufLen             = aOutBufLen;
        aStmt->mShardStmtCxt.mRowDataBuffer             = (acp_uint8_t *)aOutBuf;
        aStmt->mShardStmtCxt.mColumnOffset.mColumnCnt   = aColumnCount;
        acpMemCpy(&aStmt->mShardStmtCxt.mColumnOffset.mOffSet[0],
                  aOffSets,
                  (ACI_SIZEOF(acp_uint32_t)*aColumnCount));
        acpMemCpy(&aStmt->mShardStmtCxt.mColumnOffset.mMaxByte[0],
                  aMaxBytes,
                  (ACI_SIZEOF(acp_uint32_t)*aColumnCount));
    }
    else
    {
        aStmt->mShardStmtCxt.mIsMtDataFetch             = ACP_FALSE;
        aStmt->mShardStmtCxt.mRowDataBufLen             = 0;
        aStmt->mShardStmtCxt.mRowDataBuffer             = NULL;
        aStmt->mShardStmtCxt.mColumnOffset.mColumnCnt   = 0;
    }

    return sRet;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdExecuteForMtData( ulnStmt *aStmt )
{
    aStmt->mShardStmtCxt.mIsMtDataFetch           = ACP_TRUE;
    aStmt->mShardStmtCxt.mRowDataBufLen           = 0;
    aStmt->mShardStmtCxt.mRowDataBuffer           = NULL;
    aStmt->mShardStmtCxt.mColumnOffset.mColumnCnt = 0;

    return ulnExecute(aStmt);
}

SQLRETURN ulsdExecuteForMtDataRowsAddCallback( acp_uint32_t       aIndex,
                                               ulnStmt           *aStmt,
                                               acp_char_t        *aOutBuf,
                                               acp_uint32_t       aOutBufLen,
                                               acp_uint32_t      *aOffSets,
                                               acp_uint32_t      *aMaxBytes,
                                               acp_uint16_t       aColumnCount,
                                               ulsdFuncCallback **aCallback )
{
    ulsdFuncCallback *sCallback = aStmt->mParentDbc->mShardDbcCxt.mFuncCallback;

    ACI_TEST(ulsdVerifyFuncCallback((ulnObject *)aStmt, sCallback) != ACI_SUCCESS);

    sCallback->mFuncType = ULSD_FUNC_EXECUTE_FOR_MT_DATA_ROWS;
    sCallback->mIndex    = aIndex;
    sCallback->mRet      = SQL_SUCCESS;

    sCallback->mStmt     = aStmt;
    sCallback->mDbc      = NULL;

    sCallback->mArg.mExecute.mOutBuf      = aOutBuf;
    sCallback->mArg.mExecute.mOutBufLen   = aOutBufLen;
    sCallback->mArg.mExecute.mOffSets     = aOffSets;
    sCallback->mArg.mExecute.mMaxBytes    = aMaxBytes;
    sCallback->mArg.mExecute.mColumnCount = aColumnCount;

    if (*aCallback != NULL)
    {
        sCallback->mCount = (*aCallback)->mCount + 1;
    }
    else
    {
        sCallback->mCount = 0;
    }

    /* add first */
    sCallback->mNext = *aCallback;

    *aCallback = sCallback;

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdExecuteForMtDataAddCallback( acp_uint32_t       aIndex,
                                           ulnStmt           *aStmt,
                                           ulsdFuncCallback **aCallback )
{
    ulsdFuncCallback *sCallback = aStmt->mParentDbc->mShardDbcCxt.mFuncCallback;

    ACI_TEST(ulsdVerifyFuncCallback((ulnObject *)aStmt, sCallback) != ACI_SUCCESS);

    sCallback->mFuncType = ULSD_FUNC_EXECUTE_FOR_MT_DATA;
    sCallback->mIndex    = aIndex;
    sCallback->mRet      = SQL_SUCCESS;

    sCallback->mStmt     = aStmt;
    sCallback->mDbc      = NULL;

    if (*aCallback != NULL)
    {
        sCallback->mCount = (*aCallback)->mCount + 1;
    }
    else
    {
        sCallback->mCount = 0;
    }

    /* add first */
    sCallback->mNext = *aCallback;

    *aCallback = sCallback;

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}
SQLRETURN ulsdExecDirectAddCallback( acp_uint32_t       aIndex,
                                     ulnStmt           *aStmt,
                                     acp_char_t        *aQuery,
                                     acp_sint32_t       aQueryLen,
                                     ulsdFuncCallback **aCallback )
{
    ulsdFuncCallback *sCallback = aStmt->mParentDbc->mShardDbcCxt.mFuncCallback;

    ACI_TEST(ulsdVerifyFuncCallback((ulnObject *)aStmt, sCallback) != ACI_SUCCESS);

    sCallback->mFuncType = ULSD_FUNC_EXECUTE_DIRECT;
    sCallback->mIndex    = aIndex;
    sCallback->mRet      = SQL_SUCCESS;

    sCallback->mStmt     = aStmt;
    sCallback->mDbc      = NULL;

    sCallback->mArg.mExecDirect.mQuery    = aQuery;
    sCallback->mArg.mExecDirect.mQueryLen = aQueryLen;

    if (*aCallback != NULL)
    {
        sCallback->mCount = (*aCallback)->mCount + 1;
    }
    else
    {
        sCallback->mCount = 0;
    }

    /* add first */
    sCallback->mNext = *aCallback;

    *aCallback = sCallback;

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdExecuteAddCallback( acp_uint32_t       aIndex,
                                  ulnStmt           *aStmt,
                                  ulsdFuncCallback **aCallback )
{
    ulsdFuncCallback *sCallback = aStmt->mParentDbc->mShardDbcCxt.mFuncCallback;

    ACI_TEST(ulsdVerifyFuncCallback((ulnObject *)aStmt, sCallback) != ACI_SUCCESS);

    sCallback->mFuncType = ULSD_FUNC_EXECUTE;
    sCallback->mIndex    = aIndex;
    sCallback->mRet      = SQL_SUCCESS;

    sCallback->mStmt     = aStmt;
    sCallback->mDbc      = NULL;

    if (*aCallback != NULL)
    {
        sCallback->mCount = (*aCallback)->mCount + 1;
    }
    else
    {
        sCallback->mCount = 0;
    }

    /* add first */
    sCallback->mNext = *aCallback;

    *aCallback = sCallback;

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdPrepareAddCallback( acp_uint32_t       aIndex,
                                  ulnStmt           *aStmt,
                                  acp_char_t        *aQuery,
                                  acp_sint32_t       aQueryLen,
                                  ulsdFuncCallback **aCallback )
{
    ulsdFuncCallback *sCallback = aStmt->mParentDbc->mShardDbcCxt.mFuncCallback;

    ACI_TEST(ulsdVerifyFuncCallback((ulnObject *)aStmt, sCallback) != ACI_SUCCESS);

    sCallback->mFuncType = ULSD_FUNC_PREPARE;
    sCallback->mIndex    = aIndex;
    sCallback->mRet      = SQL_SUCCESS;

    sCallback->mStmt     = aStmt;
    sCallback->mDbc      = NULL;

    sCallback->mArg.mPrepare.mQuery    = aQuery;
    sCallback->mArg.mPrepare.mQueryLen = aQueryLen;

    if (*aCallback != NULL)
    {
        sCallback->mCount = (*aCallback)->mCount + 1;
    }
    else
    {
        sCallback->mCount = 0;
    }

    /* add first */
    sCallback->mNext = *aCallback;

    *aCallback = sCallback;

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdPrepareTranAddCallback( acp_uint32_t       aIndex,
                                      ulnDbc            *aDbc,
                                      acp_uint32_t       aXIDSize,
                                      acp_uint8_t       *aXID,
                                      acp_uint8_t       *aReadOnly,
                                      ulsdFuncCallback **aCallback )
{
    ulsdFuncCallback *sCallback = aDbc->mShardDbcCxt.mFuncCallback;

    ACI_TEST(ulsdVerifyFuncCallback((ulnObject *)aDbc, sCallback) != ACI_SUCCESS);

    sCallback->mFuncType = ULSD_FUNC_PREPARE_TRAN;
    sCallback->mIndex    = aIndex;
    sCallback->mRet      = (SQLRETURN)(-1111);

    sCallback->mStmt     = NULL;
    sCallback->mDbc      = aDbc;

    sCallback->mArg.mPrepareTran.mXIDSize  = aXIDSize;
    sCallback->mArg.mPrepareTran.mXID      = aXID;
    sCallback->mArg.mPrepareTran.mReadOnly = aReadOnly;

    if (*aCallback != NULL)
    {
        sCallback->mCount = (*aCallback)->mCount + 1;
    }
    else
    {
        sCallback->mCount = 0;
    }

    /* add first */
    sCallback->mNext = *aCallback;

    *aCallback = sCallback;

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdEndPendingTranAddCallback( acp_uint32_t       aIndex,
                                         ulnDbc            *aDbc,
                                         acp_uint32_t       aXIDSize,
                                         acp_uint8_t       *aXID,
                                         acp_sint16_t       aCompletionType,
                                         ulsdFuncCallback **aCallback )
{
    ulsdFuncCallback *sCallback = aDbc->mShardDbcCxt.mFuncCallback;

    ACI_TEST(ulsdVerifyFuncCallback((ulnObject *)aDbc, sCallback) != ACI_SUCCESS);

    sCallback->mFuncType = ULSD_FUNC_END_PENDING_TRAN;
    sCallback->mIndex    = aIndex;
    sCallback->mRet      = (SQLRETURN)(-1111);

    sCallback->mStmt     = NULL;
    sCallback->mDbc      = aDbc;

    sCallback->mArg.mEndPendingTran.mXIDSize        = aXIDSize;
    sCallback->mArg.mEndPendingTran.mXID            = aXID;
    sCallback->mArg.mEndPendingTran.mCompletionType = aCompletionType;

    if (*aCallback != NULL)
    {
        sCallback->mCount = (*aCallback)->mCount + 1;
    }
    else
    {
        sCallback->mCount = 0;
    }

    /* add first */
    sCallback->mNext = *aCallback;

    *aCallback = sCallback;

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdEndTranAddCallback( acp_uint32_t       aIndex,
                                  ulnDbc            *aDbc,
                                  acp_sint16_t       aCompletionType,
                                  ulsdFuncCallback **aCallback )
{
    ulsdFuncCallback *sCallback = aDbc->mShardDbcCxt.mFuncCallback;

    ACI_TEST(ulsdVerifyFuncCallback((ulnObject *)aDbc, sCallback) != ACI_SUCCESS);

    sCallback->mFuncType = ULSD_FUNC_END_TRAN;
    sCallback->mIndex    = aIndex;
    sCallback->mRet      = (SQLRETURN)(-1111);

    sCallback->mStmt     = NULL;
    sCallback->mDbc      = aDbc;

    sCallback->mArg.mEndTran.mCompletionType = aCompletionType;

    if (*aCallback != NULL)
    {
        sCallback->mCount = (*aCallback)->mCount + 1;
    }
    else
    {
        sCallback->mCount = 0;
    }

    /* add first */
    sCallback->mNext = *aCallback;

    *aCallback = sCallback;

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

#ifdef COMPILE_SHARDCLI
SQLRETURN ulsdShardEndTranAddCallback( acp_uint32_t       aIndex,
                                       ulnDbc            *aDbc,
                                       acp_sint16_t       aCompletionType,
                                       ulsdFuncCallback **aCallback )
{
    ulsdFuncCallback *sCallback = aDbc->mShardDbcCxt.mFuncCallback;

    ACI_TEST(ulsdVerifyFuncCallback((ulnObject *)aDbc, sCallback) != ACI_SUCCESS);

    sCallback->mFuncType = ULSD_FUNC_SHARD_END_TRAN;
    sCallback->mIndex    = aIndex;
    sCallback->mRet      = (SQLRETURN)(-1111);

    sCallback->mStmt     = NULL;
    sCallback->mDbc      = aDbc;

    sCallback->mArg.mShardEndTran.mCompletionType = aCompletionType;

    if (*aCallback != NULL)
    {
        sCallback->mCount = (*aCallback)->mCount + 1;
    }
    else
    {
        sCallback->mCount = 0;
    }

    /* add first */
    sCallback->mNext = *aCallback;

    *aCallback = sCallback;

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}
#endif

SQLRETURN ulsdPutLobAddCallback( acp_uint32_t       aIndex,
                                 ulnStmt           *aStmt,
                                 acp_sint16_t       aLocatorCType,
                                 acp_uint64_t       aLocator,
                                 acp_uint32_t       aFromPosition,
                                 acp_uint32_t       aForLength,
                                 acp_sint16_t       aSourceCType,
                                 void              *aBuffer,
                                 acp_uint32_t       aBufferSize,
                                 ulsdFuncCallback **aCallback )
{
    ulsdFuncCallback *sCallback = aStmt->mParentDbc->mShardDbcCxt.mFuncCallback;

    ACI_TEST(ulsdVerifyFuncCallback((ulnObject *)aStmt, sCallback) != ACI_SUCCESS);

    sCallback->mFuncType = ULSD_FUNC_PUT_LOB;
    sCallback->mIndex    = aIndex;
    sCallback->mRet      = SQL_SUCCESS;

    sCallback->mStmt     = aStmt;
    sCallback->mDbc      = NULL;

    sCallback->mArg.mPutLob.mLocatorCType    = aLocatorCType;
    sCallback->mArg.mPutLob.mLocator         = aLocator;
    sCallback->mArg.mPutLob.mFromPosition    = aFromPosition;
    sCallback->mArg.mPutLob.mForLength       = aForLength;
    sCallback->mArg.mPutLob.mSourceCType     = aSourceCType;
    sCallback->mArg.mPutLob.mBuffer          = aBuffer;
    sCallback->mArg.mPutLob.mBufferSize      = aBufferSize;

    if (*aCallback != NULL)
    {
        sCallback->mCount = (*aCallback)->mCount + 1;
    }
    else
    {
        sCallback->mCount = 0;
    }

    /* add first */
    sCallback->mNext = *aCallback;

    *aCallback = sCallback;

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

void ulsdDoCallback( ulsdFuncCallback *aCallback )
{
    if ( aCallback != NULL )
    {
        if ( aCallback->mStmt != NULL )
        {
            aCallback->mStmt->mShardStmtCxt.mCallback = aCallback->mNext;
        }
        else
        {
            aCallback->mDbc->mShardDbcCxt.mCallback = aCallback->mNext;
        }

        /* call depth를 제한한다. */
        if ( aCallback->mCount % ULSD_CALLBACK_DEPTH_MAX == 0 )
        {
            if ( aCallback->mStmt != NULL )
            {
                aCallback->mStmt->mShardStmtCxt.mCallback = NULL;
            }
            else
            {
                aCallback->mDbc->mShardDbcCxt.mCallback = NULL;
            }
        }
        else
        {
            /* Nothing to do */
        }

        switch ( aCallback->mFuncType )
        {
            case ULSD_FUNC_PREPARE:
            {
                aCallback->mRet = ulnPrepare(aCallback->mStmt,
                                             aCallback->mArg.mPrepare.mQuery,
                                             aCallback->mArg.mPrepare.mQueryLen,
                                             NULL);
                break;
            }

            case ULSD_FUNC_EXECUTE_FOR_MT_DATA_ROWS:
            {
                aCallback->mRet =
                    ulsdExecuteForMtDataRows(aCallback->mStmt,
                                             aCallback->mArg.mExecute.mOutBuf,
                                             aCallback->mArg.mExecute.mOutBufLen,
                                             aCallback->mArg.mExecute.mOffSets,
                                             aCallback->mArg.mExecute.mMaxBytes,
                                             aCallback->mArg.mExecute.mColumnCount);
                break;
            }

            case ULSD_FUNC_EXECUTE_FOR_MT_DATA:
            {
                aCallback->mRet = ulsdExecuteForMtData(aCallback->mStmt);
                break;
            }

            case ULSD_FUNC_EXECUTE:
            {
                aCallback->mRet = ulnExecute(aCallback->mStmt);
                break;
            }
            case ULSD_FUNC_EXECUTE_DIRECT:
            {
                aCallback->mRet = ulnExecDirect(aCallback->mStmt,
                                                aCallback->mArg.mExecDirect.mQuery,
                                                aCallback->mArg.mExecDirect.mQueryLen);
                break;
            }
            case ULSD_FUNC_PREPARE_TRAN:
            {
                aCallback->mRet = ulsdShardPrepareTran(aCallback->mDbc,
                                                       aCallback->mArg.mPrepareTran.mXIDSize,
                                                       aCallback->mArg.mPrepareTran.mXID,
                                                       aCallback->mArg.mPrepareTran.mReadOnly);
                break;
            }

            case ULSD_FUNC_END_PENDING_TRAN:
            {
                aCallback->mRet = ulsdShardEndPendingTran(aCallback->mDbc,
                                                          aCallback->mArg.mEndPendingTran.mXIDSize,
                                                          aCallback->mArg.mEndPendingTran.mXID,
                                                          aCallback->mArg.mEndPendingTran.mCompletionType);
                break;
            }

            case ULSD_FUNC_END_TRAN:
            {
                aCallback->mRet = ulnEndTran(SQL_HANDLE_DBC,
                                             (ulnObject*)aCallback->mDbc,
                                             aCallback->mArg.mEndTran.mCompletionType);
                break;
            }

#ifdef COMPILE_SHARDCLI
            case ULSD_FUNC_SHARD_END_TRAN:
            {
                aCallback->mRet = ulsdnSimpleShardEndTranDbc( aCallback->mDbc,
                                                              aCallback->mArg.mShardEndTran.mCompletionType );
                break;
            }

            case ULSD_FUNC_PUT_LOB:
            {
                aCallback->mRet = ulnPutLob(SQL_HANDLE_STMT,
                                            (ulnObject *)aCallback->mStmt,
                                            aCallback->mArg.mPutLob.mLocatorCType,
                                            aCallback->mArg.mPutLob.mLocator,
                                            aCallback->mArg.mPutLob.mFromPosition,
                                            aCallback->mArg.mPutLob.mForLength,
                                            aCallback->mArg.mPutLob.mSourceCType,
                                            aCallback->mArg.mPutLob.mBuffer,
                                            aCallback->mArg.mPutLob.mBufferSize);
                break;
            }
#endif

            default:
                break;
        }

        if ( ( aCallback->mCount % ULSD_CALLBACK_DEPTH_MAX == 0 ) &&
             ( aCallback->mNext != NULL ) )
        {
            aCallback->mNext->mCount--;

            ulsdDoCallback( aCallback->mNext );
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

void ulsdReDoCallback( ulsdFuncCallback *aCallback )
{
    switch ( aCallback->mFuncType )
    {
        case ULSD_FUNC_PREPARE:
        {
            aCallback->mRet = ulnPrepare(aCallback->mStmt,
                                         aCallback->mArg.mPrepare.mQuery,
                                         aCallback->mArg.mPrepare.mQueryLen,
                                         NULL);
            break;
        }

        case ULSD_FUNC_EXECUTE_FOR_MT_DATA_ROWS:
        {
            aCallback->mRet =
                ulsdExecuteForMtDataRows(aCallback->mStmt,
                                         aCallback->mArg.mExecute.mOutBuf,
                                         aCallback->mArg.mExecute.mOutBufLen,
                                         aCallback->mArg.mExecute.mOffSets,
                                         aCallback->mArg.mExecute.mMaxBytes,
                                         aCallback->mArg.mExecute.mColumnCount);
            break;
        }

        case ULSD_FUNC_EXECUTE_FOR_MT_DATA:
        {
            aCallback->mRet = ulsdExecuteForMtData(aCallback->mStmt);
            break;
        }

        case ULSD_FUNC_EXECUTE:
        {
            aCallback->mRet = ulnExecute(aCallback->mStmt);
            break;
        }
        case ULSD_FUNC_EXECUTE_DIRECT:
        {
            aCallback->mRet = ulnExecDirect(aCallback->mStmt,
                                            aCallback->mArg.mExecDirect.mQuery,
                                            aCallback->mArg.mExecDirect.mQueryLen);
            break;
        }
        case ULSD_FUNC_PREPARE_TRAN:
        {
            aCallback->mRet = ulsdShardPrepareTran(aCallback->mDbc,
                                                   aCallback->mArg.mPrepareTran.mXIDSize,
                                                   aCallback->mArg.mPrepareTran.mXID,
                                                   aCallback->mArg.mPrepareTran.mReadOnly);
            break;
        }

        case ULSD_FUNC_END_PENDING_TRAN:
        {
            aCallback->mRet = ulsdShardEndPendingTran(aCallback->mDbc,
                                                      aCallback->mArg.mEndPendingTran.mXIDSize,
                                                      aCallback->mArg.mEndPendingTran.mXID,
                                                      aCallback->mArg.mEndPendingTran.mCompletionType);
            break;
        }

        case ULSD_FUNC_END_TRAN:
        {
            aCallback->mRet = ulnEndTran(SQL_HANDLE_DBC,
                                         (ulnObject*)aCallback->mDbc,
                                         aCallback->mArg.mEndTran.mCompletionType);
            break;
        }

#ifdef COMPILE_SHARDCLI
        case ULSD_FUNC_SHARD_END_TRAN:
        {
            aCallback->mRet = ulsdnSimpleShardEndTranDbc( aCallback->mDbc,
                                                          aCallback->mArg.mEndTran.mCompletionType );
            break;
        }

        case ULSD_FUNC_PUT_LOB:
        {
            aCallback->mRet = ulnPutLob(SQL_HANDLE_STMT,
                                        (ulnObject *)aCallback->mStmt,
                                        aCallback->mArg.mPutLob.mLocatorCType,
                                        aCallback->mArg.mPutLob.mLocator,
                                        aCallback->mArg.mPutLob.mFromPosition,
                                        aCallback->mArg.mPutLob.mForLength,
                                        aCallback->mArg.mPutLob.mSourceCType,
                                        aCallback->mArg.mPutLob.mBuffer,
                                        aCallback->mArg.mPutLob.mBufferSize);
            break;
        }
#endif

        default:
            break;
    }
}

SQLRETURN ulsdGetResultCallback( acp_uint32_t      aIndex,
                                 ulsdFuncCallback *aCallback,
                                 acp_uint8_t       aReCall )
{
    ulsdFuncCallback * sCallback = aCallback;
    SQLRETURN          sRet = SQL_NO_DATA;

    while ( sCallback != NULL )
    {
        if ( sCallback->mIndex == aIndex )
        {
            sRet = sCallback->mRet;

            if ( ( sRet == (SQLRETURN)(-1111) ) && ( aReCall == 1 ) )
            {
                /* 이전 callback 의 error 로 인해 수행하지 않은 경우 재시도 */
                ulsdReDoCallback( sCallback );
                sRet = sCallback->mRet;
            }
            else
            {
                /* Nothing to do. */
            }
            break;
        }
        else
        {
            sCallback = sCallback->mNext;
        }
    }

    return sRet;
}

void ulsdRemoveCallback( ulsdFuncCallback *aCallback )
{
    ulsdFuncCallback * sCallback = aCallback;
    ulsdFuncCallback * sNext     = NULL;

    while ( sCallback != NULL )
    {
        /* 초기화 */
        if ( sCallback->mStmt != NULL )
        {
            sCallback->mStmt->mShardStmtCxt.mCallback = NULL;
        }
        else if ( sCallback->mDbc != NULL )
        {
            sCallback->mDbc->mShardDbcCxt.mCallback = NULL;
        }
        else
        {
            /* Nothing to do */
        }

        sNext = sCallback->mNext;

        sCallback->mInUse = ACP_FALSE;  /* BUG-46814 */

        sCallback = sNext;
    }
}

void ulsdStmtCallback( ulnStmt *aStmt )
{
    if ( aStmt->mShardStmtCxt.mCallback != NULL )
    {
        ulsdDoCallback( aStmt->mShardStmtCxt.mCallback );
    }
    else
    {
        /* Nothing to do */
    }
}

void ulsdDbcCallback( ulnDbc *aDbc )
{
    if ( aDbc->mShardDbcCxt.mCallback != NULL )
    {
        ulsdDoCallback( aDbc->mShardDbcCxt.mCallback );
    }
    else
    {
        /* Nothing to do */
    }
}

/* BUG-46814 funcCallback이 사용중인지 체크 한다. */
ACI_RC ulsdVerifyFuncCallback( ulnObject *aObject, ulsdFuncCallback *aFuncCallback )
{
    ulnFnContext sFnContext;

    ACE_DASSERT( aFuncCallback != NULL );
    ACE_DASSERT( aFuncCallback->mInUse == ACP_FALSE );

    ACI_TEST_RAISE( aFuncCallback == NULL, LABEL_FUNC_CALLBACK_NULL );
    ACI_TEST_RAISE( aFuncCallback->mInUse == ACP_TRUE, LABEL_FUNC_CALLBACK_IN_USE );
    aFuncCallback->mInUse = ACP_TRUE;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_FUNC_CALLBACK_NULL)
    {
        ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_NONE, aObject, ULN_OBJ_GET_TYPE(aObject) );
        (void)ulnError( &sFnContext,
                        ulERR_ABORT_SHARD_ERROR,
                        "VerifyFuncCallback",
                        "Unexpected Error: The funcCallback is NULL" );
    }
    ACI_EXCEPTION(LABEL_FUNC_CALLBACK_IN_USE)
    {
        ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_NONE, aObject, ULN_OBJ_GET_TYPE(aObject) );
        (void)ulnError( &sFnContext,
                        ulERR_ABORT_SHARD_ERROR,
                        "VerifyFuncCallback",
                        "Unexpected Error: The funcCallback is already in use" );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

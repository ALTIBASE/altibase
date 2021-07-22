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

#include <cm.h>
#include <mmErrorCode.h>
#include <mmcConv.h>
#include <mmcConvFmMT.h>
#include <mmcStatement.h>
#include <mmcStatementManager.h>
#include <mmcTask.h>
#include <mmtServiceThread.h>
#include <mmtAuditManager.h>

static IDE_RC answerFetchMoveResult(cmiProtocolContext *aProtocolContext)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 1);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_FetchMoveResult);

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}

static IDE_RC answerFetchBeginResult(cmiProtocolContext *aProtocolContext, UInt aStatementID, UShort aResultSetID)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 7);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_FetchBeginResult);
    CMI_WR4(aProtocolContext, &aStatementID);
    CMI_WR2(aProtocolContext, &aResultSetID);

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}

static IDE_RC answerFetchEndResult(cmiProtocolContext *aProtocolContext, UInt aStatementID, UShort aResultSetID)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 7);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_FetchEndResult);
    CMI_WR4(aProtocolContext, &aStatementID);
    CMI_WR2(aProtocolContext, &aResultSetID);

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}

/*******************************************************************
 PROJ-2160 CM 타입제거
 Description : 1 row 씩 block에 기록한다.
               타입별로 데이타가 클때는 나누어서 기록한다.
********************************************************************/
static IDE_RC doFetch( cmiProtocolContext *aProtocolContext,
                       mmcSession         *aSession,
                       qciStatement       *aQciStmt,
                       UShort              aColumnFrom,
                       UShort              aColumnTo,
                       mmcBaseRow         *aBaseRow)
{
    cmiProtocolContext *sCtx = aProtocolContext;
    UInt                sRowSize = 0;
    UInt                sRemainSize = 0;
    UInt                sSendSize = 0;
    UInt                sColumnSize = 0;
    UInt                sLen32;
    UShort              sLen16;
    UChar               sLen8;
    UShort              sColumnIndex = 0;
    qciFetchColumnInfo  sFetchColumnInfo;

    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    UInt                sLobSize           = 0;
    ULong               sLobSize64         = 0;
    UChar               sHasData           = ID_FALSE;
    UInt                sLobCacheThreshold = 0;
    cmiWriteCheckState  sWriteCheckState   = CMI_WRITE_CHECK_DEACTIVATED;
    UShort              sOrgWriteCursor    = CMI_GET_CURSOR(aProtocolContext);
    idBool              sIsNullLob         = ID_FALSE;

    /* PROJ-2331 */
    qciBindColumn sBindColumn;

    sLobCacheThreshold = aSession->getLobCacheThreshold();

    sRowSize = (UInt)(qci::getRowActualSize(aQciStmt));

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    IDU_FIT_POINT("mmtCmsFetch::doFetch::CmiWriteCheck");
    CMI_WRITE_CHECK(sCtx, 5);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(sCtx, CMP_OP_DB_FetchResult);

    /* PROJ-2331 때문에 RowSize는 틀릴 수 있다.
     * 다만 Client 측에서 사용되지 않기 때문에 무시하도록 한다. */
    CMI_WR4(sCtx, &sRowSize);

    for( sColumnIndex =  aColumnFrom;
         sColumnIndex <= aColumnTo;
         sColumnIndex++ )
    {
        qci::getFetchColumnInfo(aQciStmt,
                                sColumnIndex,
                                &sFetchColumnInfo);

        switch(sFetchColumnInfo.dataTypeId)
        {
            case MTD_NULL_ID :
                sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                CMI_WRITE_CHECK(sCtx, ID_SIZEOF(UChar));
                sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                CMI_WR1(sCtx, *(sFetchColumnInfo.value));
                break;

            case MTD_BINARY_ID :
                sLen32 = ((mtdBinaryType*) sFetchColumnInfo.value)->mLength;
                sColumnSize = sLen32 + ID_SIZEOF(SDouble);

                sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
                CMI_WRITE_CHECK_WITH_IPCDA( sCtx, ID_SIZEOF(SDouble), sColumnSize);
                sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

                CMI_WR4(sCtx, &sLen32);
                CMI_WR4(sCtx, &sLen32);

                IDE_TEST_RAISE( cmiSplitWrite( sCtx,
                                               sLen32,
                                               ((mtdBinaryType*)sFetchColumnInfo.value)->mValue )
                                != IDE_SUCCESS, MarshallingError );
                break;

            // mtdCharType, mtdNcharType, mtdByteType은 구조가 모두 동일하다
            case MTD_CHAR_ID :
            case MTD_VARCHAR_ID :
            case MTD_NCHAR_ID :
            case MTD_NVARCHAR_ID :
                if(aBaseRow != NULL)
                {
                    sBindColumn.mId = sColumnIndex;
                    qci::getBindColumnInfo(aQciStmt, &sBindColumn);
                    mmcConvFromMT::compareMtChar(aBaseRow, &sBindColumn, (mtdCharType*)sFetchColumnInfo.value);

                    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                    CMI_WRITE_CHECK( sCtx, ID_SIZEOF(UChar));
                    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

                    if(aBaseRow->mIsRedundant == ID_TRUE)
                    {
                        aBaseRow->mCompressedSize4CurrentRow += ((mtdCharType*)sFetchColumnInfo.value)->length;
                        aBaseRow->mCompressedSize4CurrentRow += 2; // To add the size of UShort variable for including character length
                        aBaseRow->mCompressedSize4CurrentRow -= 1; // To subtract redundant flag size 1 byte
                        aBaseRow->mIsRedundant = ID_FALSE;
                        CMI_WR1(sCtx, 1);
                        break;
                    }
                    else
                    {
                        CMI_WR1(sCtx, 0);
                    }
                }

            	sLen16 = ((mtdCharType*)sFetchColumnInfo.value)->length;
				sColumnSize = sLen16 + ID_SIZEOF(UShort);

                sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
                CMI_WRITE_CHECK_WITH_IPCDA( sCtx, ID_SIZEOF(UShort), sColumnSize);
                sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

                CMI_WR2(sCtx, &sLen16);

                IDU_FIT_POINT_RAISE("mmtCmsFetch::doFetch::CHAR::MarshallingError", MarshallingError);

                IDE_TEST_RAISE( cmiSplitWrite( sCtx,
                                               sLen16,
                                               ((mtdCharType*)sFetchColumnInfo.value)->value)
                                != IDE_SUCCESS, MarshallingError );
                break;

            case MTD_BYTE_ID :
            case MTD_VARBYTE_ID :
                sLen16 = ((mtdByteType*)sFetchColumnInfo.value)->length;
                sColumnSize = sLen16 + ID_SIZEOF(UShort);

                sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
                CMI_WRITE_CHECK_WITH_IPCDA( sCtx, ID_SIZEOF(UShort), sColumnSize);
                sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

                CMI_WR2(sCtx, &sLen16);

                IDE_TEST_RAISE( cmiSplitWrite( sCtx,
                                               sLen16,
                                               ((mtdByteType*)sFetchColumnInfo.value)->value)
                                != IDE_SUCCESS, MarshallingError );
                break;

            case MTD_FLOAT_ID :
            case MTD_NUMERIC_ID :
            case MTD_NUMBER_ID :
                sLen8 = ((mtdNumericType*) sFetchColumnInfo.value)->length;
                sColumnSize = sLen8 + ID_SIZEOF(UChar);

                sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                CMI_WRITE_CHECK(sCtx, sColumnSize);
                sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                CMI_WR1(sCtx, sLen8);

                if( sLen8 != 0 )
                {
                    CMI_WR1(sCtx, ((mtdNumericType*) sFetchColumnInfo.value)->signExponent);
                    CMI_WCP(sCtx, ((mtdNumericType*) sFetchColumnInfo.value)->mantissa, sLen8 -ID_SIZEOF(UChar));
                }
                break;

            case MTD_SMALLINT_ID:
                sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                CMI_WRITE_CHECK(sCtx, ID_SIZEOF(mtdSmallintType));
                sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                CMI_WR2(sCtx, (UShort*)sFetchColumnInfo.value);
                break;

            case MTD_INTEGER_ID:
                sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                CMI_WRITE_CHECK(sCtx, ID_SIZEOF(mtdIntegerType));
                sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                CMI_WR4(sCtx, (UInt*)sFetchColumnInfo.value);
                break;

            case MTD_BIGINT_ID:
                sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                CMI_WRITE_CHECK(sCtx, ID_SIZEOF(mtdBigintType));
                sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                CMI_WR8(sCtx, (ULong*)sFetchColumnInfo.value);
                break;

            case MTD_REAL_ID :
                sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                CMI_WRITE_CHECK(sCtx, ID_SIZEOF(mtdRealType));
                sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                CMI_WR4(sCtx, (UInt*)sFetchColumnInfo.value);
                break;

            case MTD_DOUBLE_ID :
                sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                CMI_WRITE_CHECK(sCtx, ID_SIZEOF(mtdDoubleType));
                sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                CMI_WR8(sCtx, (ULong*)sFetchColumnInfo.value);
                break;

            case MTD_BLOB_LOCATOR_ID :
            case MTD_CLOB_LOCATOR_ID :
                /* PROJ-2616 IPCDA not support LOB DATA TYPE */
                IDE_TEST_RAISE( cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA,
                                NotSupportDataType );

                IDU_FIT_POINT_RAISE("mmtCmsFetch::doFetch::LOB::MarshallingError", MarshallingError);

                /* PROJ-2047 Strengthening LOB - LOBCACHE */
                IDE_TEST_RAISE(qciMisc::lobGetLength(aSession->getStatSQL(),
                                                     *((smLobLocator*)sFetchColumnInfo.value),
                                                     &sLobSize,
                                                     &sIsNullLob)
                               != IDE_SUCCESS, MarshallingError);  /* BUG-45898 */

                sColumnSize = ID_SIZEOF(smLobLocator) + ID_SIZEOF(ULong) + ID_SIZEOF(UChar);

                if (sLobCacheThreshold > 0 &&
                    sLobSize > 0 &&
                    sLobSize <= sLobCacheThreshold)
                {
                    sHasData = ID_TRUE;
                }
                else
                {
                    sHasData = ID_FALSE;
                }

                /* Locator, Size, HasData는 한번에 보내자. */
                sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                CMI_WRITE_CHECK(sCtx, sColumnSize);
                sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

                CMI_WR8(sCtx, (ULong *)sFetchColumnInfo.value);

                /* PROJ-2728 Sharding LOB 
                 *   ULONG_MAX 값을 NULL로 사용한다 */
                if ( ( aProtocolContext->mProtocol.mOpID == CMP_OP_DB_FetchV3 ) &&
                     ( sIsNullLob == ID_TRUE ) )
                {
                    sLobSize64 = ID_ULONG_MAX;
                }
                else
                {
                    sLobSize64 = sLobSize;
                }

                CMI_WR8(sCtx, &sLobSize64);
                CMI_WR1(sCtx, sHasData);

                if (sHasData == ID_TRUE)
                {
                    /* BUG-37642 Improve performance to fetch. */
                    sRemainSize = CMI_REMAIN_SPACE_IN_WRITE_BLOCK(aProtocolContext);
                    sLen32      = 0;

                    /*
                     * 복사 비용을 줄이기 위해 버퍼를 잡을 수 없기 때문에
                     * lobRead가 2번 호출될 수도 있다.
                     */
                    do
                    {
                        sSendSize = IDL_MIN( sLobSize, sRemainSize );

                        /* 통신 버퍼에 LOB Data를 직접 쓰기 위해 CMI_WCP를 사용하지 않았다. */
                        IDE_TEST_RAISE(qciMisc::lobRead(aSession->getStatSQL(),
                                                        *(smLobLocator *)sFetchColumnInfo.value,
                                                        sLen32,
                                                        sSendSize,
                                                        sCtx->mWriteBlock->mData + sCtx->mWriteBlock->mCursor),
                                       MarshallingError);
                        sCtx->mWriteBlock->mCursor += sSendSize;

                        sLobSize -= sSendSize;
                        if (sLobSize > 0)
                        {
                            IDE_TEST_RAISE( cmiSend( sCtx, ID_FALSE ) != IDE_SUCCESS, MarshallingError );

                            sRemainSize = CMI_REMAIN_SPACE_IN_WRITE_BLOCK(aProtocolContext);
                            sLen32 += sSendSize;
                        }
                        else
                        {
                            break;
                        }
                    } while (1);
                }
                else
                {
                    /* Nothing */
                }
                break;

            case MTD_DATE_ID:
                sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                CMI_WRITE_CHECK(sCtx, ID_SIZEOF(mtdDateType));
                sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                CMI_WR2(sCtx, (UShort*)&(((mtdDateType*)sFetchColumnInfo.value)->year)); 
                CMI_WR2(sCtx, &(((mtdDateType*)sFetchColumnInfo.value)->mon_day_hour));
                CMI_WR4(sCtx, &(((mtdDateType*)sFetchColumnInfo.value)->min_sec_mic));
                break;

            case MTD_INTERVAL_ID :
                sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                CMI_WRITE_CHECK(sCtx, ID_SIZEOF(mtdIntervalType));
                sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                CMI_WR8(sCtx, (ULong*)&(((mtdIntervalType*)sFetchColumnInfo.value)->second));
                CMI_WR8(sCtx, (ULong*)&(((mtdIntervalType*)sFetchColumnInfo.value)->microsecond));
                break;

            case MTD_BIT_ID :
            case MTD_VARBIT_ID :
                sLen32 = ((mtdBitType*) sFetchColumnInfo.value)->length;
                sColumnSize = BIT_TO_BYTE(sLen32) + ID_SIZEOF(UInt);

                sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                CMI_WRITE_CHECK_WITH_IPCDA( sCtx, ID_SIZEOF(UInt), sColumnSize);
                sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

                CMI_WR4(sCtx, &sLen32);

                IDE_TEST_RAISE( cmiSplitWrite( sCtx,
                                               BIT_TO_BYTE(sLen32),
                                               ((mtdBitType*)sFetchColumnInfo.value)->value )
                                != IDE_SUCCESS, MarshallingError );

                break;

            case MTD_NIBBLE_ID :
                sLen8 = ((mtdNibbleType*) sFetchColumnInfo.value)->length;

                if( sLen8 != MTD_NIBBLE_NULL_LENGTH )
                {
                    sColumnSize = ((sLen8 + 1) >> 1) + ID_SIZEOF(UChar);
                }
                else
                {
                    sColumnSize = ID_SIZEOF(UChar);
                }

                sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                CMI_WRITE_CHECK(sCtx, sColumnSize);
                sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                CMI_WR1(sCtx, sLen8);
                CMI_WCP(sCtx, ((mtdNibbleType*)sFetchColumnInfo.value)->value, sColumnSize - ID_SIZEOF(UChar));
                break;
            default :
                IDE_DASSERT(0);
                break;
        }
    }
    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(sCtx);

    return IDE_SUCCESS;

    IDE_EXCEPTION(NotSupportDataType);
    {
        ideLog::log(IDE_SERVER_1,
                    "doFetch: Not support Data Type ["
                    "column no: %d "
                    "type: %u]",
                    sColumnIndex, sFetchColumnInfo.dataTypeId);
        /* PROJ-2616 */
    }
    IDE_EXCEPTION(MarshallingError);
    {
        ideLog::log(IDE_SERVER_1,
                    "doFetch: marshal error ["
                    "column no: %d "
                    "type: %u]",
                    sColumnIndex, sFetchColumnInfo.dataTypeId);

        if (cmiGetLinkImpl(sCtx) != CMI_LINK_IMPL_IPCDA)
        {
            // 마샬링 에러가 발생한 경우 세션을 끊는 방법밖에 없다.
            // cf) 첫번째 패킷이 이미 전송된 후에 두번째 패킷에서
            // 마샬링 에러가 발생하면 ErrorResult 조차도 보내면 안된다.
            // client에서는 특정 프로토콜이 연이어 올것이라 믿기 때문에.
            sCtx->mSessionCloseNeeded = ID_TRUE;
        }
        else
        {
            /* BUG-45898 IPCDA는 패킷이 분할되지 않기에 접속을 끊을 필요는 없다. */
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
        }
    }
    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    CMI_SET_CURSOR(aProtocolContext, sOrgWriteCursor);

    return IDE_FAILURE;
}

static IDE_RC fetchEnd(cmiProtocolContext *aProtocolContext,
                       mmcStatement       *aStatement,
                       UShort              aResultSetID)
{
    IDE_RC         sRet;
    UShort         sEnableResultSetCount;
    mmcResultSet * sResultSet;
    mmcStatement * sResultSetStmt;

    /* BUG-45898 접속을 끊어야 할 때는 패킷을 더 전송할 필요가 없다. */
    IDE_TEST(aProtocolContext->mSessionCloseNeeded == ID_TRUE);

    sResultSet = aStatement->getResultSet( aResultSetID );
    // bug-26977: codesonar: resultset null ref
    // null이 가능한지는 모르겠지만, 방어코드임.
    IDE_TEST(sResultSet == NULL);

    sResultSetStmt = (mmcStatement*)sResultSet->mResultSetStmt;

    /* PROJ-1381, BUG-32902 FAC
     * Holdable, Keyset-driven은 CloseCursor 전에 임의로 Statement를 정리해서는 안된다. */
    if (aStatement->isClosableWhenFetchEnd() != ID_TRUE)
    {
        /* Fetch 완료 후, 제대로된 Plan을 주려면 여기서 만들어둬야한다. */
        if (aStatement->getSession()->getExplainPlan() != QCI_EXPLAIN_PLAN_OFF)
        {
            IDE_TEST(aStatement->clearPlanTreeText() != IDE_SUCCESS);
            IDE_TEST(aStatement->makePlanTreeText(ID_FALSE) != IDE_SUCCESS);
        }

        /* PROJ-1381, BUG-33121 FAC : ResultSet에 FetchEnd 상태 추가 */
        IDE_TEST(aStatement->setResultSetState(aResultSetID,
                                               MMC_RESULTSET_STATE_FETCH_END)
                 != IDE_SUCCESS);

        answerFetchEndResult(aProtocolContext, aStatement->getStmtID(), aResultSetID);
    }
    else
    {
        sEnableResultSetCount = aStatement->getEnableResultSetCount();

        if (aStatement->getResultSetState(aResultSetID) != MMC_RESULTSET_STATE_FETCH_CLOSE)
        {
            // bug-26977: codesonar: resultset null ref
            // null이 가능한지는 모르겠지만, 방어코드추가.
            IDE_TEST(sResultSetStmt == NULL);

            mmcStatement::makePlanTreeBeforeCloseCursor( aStatement,
                                                         sResultSetStmt );

            IDE_TEST_RAISE(qci::closeCursor(sResultSetStmt->getQciStmt(),
                                            sResultSetStmt->getSmiStmt())
                                            != IDE_SUCCESS, CloseCursorFailure);

            answerFetchEndResult(aProtocolContext, aStatement->getStmtID(), aResultSetID);

            // Fetch 가능한 Result Set을 하나 감소
            sEnableResultSetCount--;
            aStatement->setEnableResultSetCount(sEnableResultSetCount);
        }

        IDE_TEST(aStatement->endFetch(aResultSetID) != IDE_SUCCESS);

        if (sEnableResultSetCount <= 0)
        {
            /* PROJ-2223 Altibase Auditing */
            mmtAuditManager::auditByAccess( aStatement, MMC_EXECUTION_FLAG_SUCCESS );
            
            IDE_TEST_RAISE(qci::closeCursor(aStatement->getQciStmt(),
                                            aStatement->getSmiStmt())
                                            != IDE_SUCCESS, CloseCursorFailure);

            sRet = aStatement->clearStmt(MMC_STMT_BIND_NONE);

            IDE_TEST(aStatement->endStmt(MMC_EXECUTION_FLAG_SUCCESS)
                     != IDE_SUCCESS);

            aStatement->setExecuteFlag(ID_FALSE);

            IDE_TEST(sRet != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    /* 
     * BUG-30053 
     * Even though qci::closeCursor failed, 
     * The statement should be cleared.
     */
    IDE_EXCEPTION(CloseCursorFailure);
    {
        aStatement->clearStmt(MMC_STMT_BIND_INFO);
        aStatement->endStmt(MMC_EXECUTION_FLAG_FAILURE);
        aStatement->setExecuteFlag(ID_FALSE);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::fetchIPCDA(cmiProtocolContext *aProtocolContext,
                                    mmcSession         *aSession,
                                    mmcStatement       *aStatement,
                                    UShort              aResultSetID,
                                    UShort              aColumnFrom,
                                    UShort              aColumnTo,
                                    UInt                aRecordCount)
{
    mmcResultSet     *sResultSet;
    mmcStatement     *sResultSetStmt;
    qciStatement     *sQciStmt;
    ULong             sSendSize    = 0;
    UInt              sSize        = 0;
    UInt              sRemainSize  = 0;
    idBool            sRecordExist;
    mmcTransObj      *sShareTrans = NULL;

    sResultSet = aStatement->getResultSet( aResultSetID );
    // bug-26977: codesonar: resultset null ref
    // null이 가능한지는 모르겠지만, 방어코드임.
    IDE_TEST(sResultSet == NULL);

    sResultSetStmt = (mmcStatement*)sResultSet->mResultSetStmt;
    // bug-26977: codesonar: resultset null ref
    // null이 가능한지는 모르겠지만, 방어코드추가.
    IDE_TEST(sResultSetStmt == NULL);

    sQciStmt    = sResultSetStmt->getQciStmt();

    // first 1row
    sSendSize   = ((qci::getRowActualSize(sQciStmt)) + 5);

    IDE_TEST( qci::getRowSize( sQciStmt, &sSize )
                  != IDE_SUCCESS );

    /* BUG-44705 prefetch가 아닌경우에는 aRecordCount값을 UINT_MAX값으로 설정한다. */
    if ( aRecordCount == 0)
    {
        aRecordCount = ID_UINT_MAX;
    }

    sShareTrans = aStatement->getShareTransForSmiStmtLock(NULL);
    if ( sShareTrans != NULL )
    {
        aStatement->acquireShareTransSmiStmtLock(sShareTrans);
    }

    do
    {
        IDE_TEST_RAISE( doFetch( aProtocolContext,
                                 aSession,
                                 sQciStmt,
                                 aColumnFrom,
                                 aColumnTo,
                                 NULL ) != IDE_SUCCESS,
                        FetchError);

        if (aStatement->getStmtType() == QCI_STMT_DEQUEUE)
        {
            sRecordExist = ID_FALSE;
        }
        else
        {
            IDE_TEST_RAISE(qci::moveNextRecord(sQciStmt,
                                               sResultSetStmt->getSmiStmt(),
                                               &sRecordExist) != IDE_SUCCESS,
                           FetchError);
        }

        if (sRecordExist != ID_TRUE)
        {
            aStatement->setFetchFlag(MMC_FETCH_FLAG_CLOSE);
            break;
        }

        aRecordCount--;

        /* opcode(1) + rowsize(4bytes) + rowActualSize + CMI_IPCDA_REMAIN_PROTOCOL_SIZE */
        sSendSize = (1 + 4 + (qci::getRowActualSize(sQciStmt))) + CMI_IPCDA_REMAIN_PROTOCOL_SIZE;

        sRemainSize = CMI_REMAIN_SPACE_IN_WRITE_BLOCK(aProtocolContext);

        if( sSendSize > sRemainSize )
        {
            break;
        }
    } while (aRecordCount > 0);

    if (sRecordExist != ID_TRUE)
    {
        IDE_TEST(fetchEnd(aProtocolContext,
                          aStatement,
                          aResultSetID)
                 != IDE_SUCCESS);
    }

    if (sShareTrans != NULL)
    {
        aStatement->releaseShareTransSmiStmtLock(sShareTrans);
        sShareTrans = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(FetchError);
    {
        IDE_PUSH();

        fetchEnd(aProtocolContext, aStatement, aResultSetID);

        IDE_POP();
    }
    IDE_EXCEPTION_END;

    if (sShareTrans != NULL)
    {
        aStatement->releaseShareTransSmiStmtLock(sShareTrans);
        sShareTrans = NULL;
    }

    return IDE_FAILURE;
}

/* BUG-39463 Add new fetch protocol that can request over 65535 rows. */
IDE_RC mmtServiceThread::fetch(cmiProtocolContext *aProtocolContext,
                               mmcSession         *aSession,
                               mmcStatement       *aStatement,
                               UShort              aResultSetID,
                               UShort              aColumnFrom,
                               UShort              aColumnTo,
                               UInt                aRecordCount)
{
    mmcResultSet     *sResultSet;
    mmcStatement     *sResultSetStmt;
    qciStatement     *sQciStmt;
    ULong             sSendSize        = 0;
    UInt              sSize            = 0;
    UInt              sRemainSize      = 0;
    idBool            sRecordExist;
    mmcBaseRow       *sBaseRow         = NULL;    // PROJ-2331
    idBool            sIsFetchRowsMode = ID_FALSE;
    mmcTransObj      *sShareTrans = NULL;

    sResultSet = aStatement->getResultSet( aResultSetID );
    // bug-26977: codesonar: resultset null ref
    // null이 가능한지는 모르겠지만, 방어코드임.
    IDE_TEST(sResultSet == NULL);

    sResultSetStmt = (mmcStatement*)sResultSet->mResultSetStmt;
    // bug-26977: codesonar: resultset null ref
    // null이 가능한지는 모르겠지만, 방어코드추가.
    IDE_TEST(sResultSetStmt == NULL);

    sQciStmt    = sResultSetStmt->getQciStmt();
    sRemainSize = CMI_REMAIN_SPACE_IN_WRITE_BLOCK(aProtocolContext);

    // first 1row
    sSendSize   = ((qci::getRowActualSize(sQciStmt)) + 5);

    IDE_TEST( qci::getRowSize( sQciStmt, &sSize )
                  != IDE_SUCCESS );

    // PROJ-2331
    if ( aSession->getRemoveRedundantTransmission() == 1
         && sResultSetStmt->getStmtType() != QCI_STMT_DEQUEUE )
    {
        sBaseRow               = &( sResultSet->mBaseRow );
        aStatement->freeBaseRow( sBaseRow );
        aStatement->initBaseRow( sBaseRow );
        IDE_TEST( aStatement->allocBaseRow( sBaseRow, sSize )
                  != IDE_SUCCESS );
    }

    /* BUG-44705 [mm-cli] IPCDA 모드시 Fetch 동작 검토 */
    if ( aRecordCount != 0)
    {
        sIsFetchRowsMode = ID_TRUE;
    }
    else
    {
        aRecordCount = ID_UINT_MAX;
    }

    sShareTrans = aStatement->getShareTransForSmiStmtLock(NULL);
    if ( sShareTrans != NULL )
    {
        aStatement->acquireShareTransSmiStmtLock(sShareTrans);
    }

    do
    {
        IDE_TEST_RAISE( doFetch( aProtocolContext,
                                 aSession,
                                 sQciStmt,
                                 aColumnFrom,
                                 aColumnTo,
                                 sBaseRow ) != IDE_SUCCESS,
                        FetchError);

        if (aStatement->getStmtType() == QCI_STMT_DEQUEUE)
        {
            sRecordExist = ID_FALSE;
        }
        else
        {
            IDE_TEST_RAISE(qci::moveNextRecord(sQciStmt,
                                               sResultSetStmt->getSmiStmt(),
                                               &sRecordExist) != IDE_SUCCESS,
                           FetchError);

            // PROJ-2331
            if ( sBaseRow != NULL )
            {
                sBaseRow->mBaseColumnPos = 0;
                sBaseRow->mCompressedSize4CurrentRow = 0;

                if ( sBaseRow->mIsFirstRow == ID_TRUE )
                {
                    sBaseRow->mIsFirstRow = ID_FALSE;
                }
            }
        }

        if (sRecordExist != ID_TRUE)
        {
            aStatement->setFetchFlag(MMC_FETCH_FLAG_CLOSE);
            break;
        }
        else
        {
            /* do nothing */
        }

        if ( sIsFetchRowsMode == ID_TRUE )
        {
            aRecordCount--;
        }
        else
        {
            /* opcode(1) + rowsize(4bytes) + rowActualSize*/
            sSendSize = ((qci::getRowActualSize(sQciStmt)) + 1 + 4);

            if(sBaseRow != NULL)
            {
                sSendSize -= sBaseRow->mCompressedSize4CurrentRow;
                sBaseRow->mCompressedSize4CurrentRow = 0;
            }

            /*
             * BUG-37642 Improve performance to fetch.
             *
             * first row가 1 block을 넘어가는 경우 그 다음 block은
             * 꽉 채워서 보낸다.
             */
            sRemainSize = CMI_REMAIN_SPACE_IN_WRITE_BLOCK(aProtocolContext);

            if( sSendSize > sRemainSize )
            {
                break;
            }
        }

    } while (aRecordCount > 0);

    if (sRecordExist != ID_TRUE)
    {
        IDE_TEST(fetchEnd(aProtocolContext,
                          aStatement,
                          aResultSetID)
                 != IDE_SUCCESS);
    }
    else
    {
        /* do nothing. */
    }

    if (sShareTrans != NULL)
    {
        aStatement->releaseShareTransSmiStmtLock(sShareTrans);
        sShareTrans = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(FetchError);
    {
        IDE_PUSH();

        fetchEnd(aProtocolContext, aStatement, aResultSetID);

        IDE_POP();
    }
    IDE_EXCEPTION_END;

    if (sShareTrans != NULL)
    {
        aStatement->releaseShareTransSmiStmtLock(sShareTrans);
        sShareTrans = NULL;
    }

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::fetchMoveProtocol(cmiProtocolContext *aProtocolContext,
                                           cmiProtocol        *,
                                           void               *aSessionOwner,
                                           void               *aUserContext)
{
    mmcTask           *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread  *sThread = (mmtServiceThread *)aUserContext;
    mmcSession        *sSession;
    mmcStatement      *sStatement;
    idBool             sRecordExist = ID_FALSE;
    SLong              i;
    mmcResultSet     * sResultSet;
    mmcStatement     * sResultSetStmt;
    mmcTransObj      * sShareTrans = NULL;

    UInt        sStatementID;
    UShort      sResultSetID;
    UChar       sWhence;
    SLong       sOffset;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD4(aProtocolContext, &sStatementID);
    CMI_RD2(aProtocolContext, &sResultSetID);
    CMI_RD1(aProtocolContext, sWhence);
    CMI_RD8(aProtocolContext, (ULong*)&sOffset);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sStatementID,
                           sThread) != IDE_SUCCESS);
    //fix BUG-17689.
    IDE_TEST_RAISE(checkStatementState(sStatement, MMC_STMT_STATE_EXECUTED) != IDE_SUCCESS,
                   NoRows);

    IDE_TEST_RAISE(sWhence != CMP_DB_FETCHMOVE_CUR, UnsupportedFetchMove);
    IDE_TEST_RAISE(sOffset <= 0, UnsupportedFetchMove);

    sResultSet = sStatement->getResultSet(sResultSetID);
    // bug-26977: codesonar: resultset null ref
    // null이 가능한지는 모르겠지만, 방어코드임.
    IDE_TEST(sResultSet == NULL);

    sResultSetStmt = (mmcStatement*)sResultSet->mResultSetStmt;

    sStatement->setFetchEndTime(0); /* BUG-19456 */
    sStatement->setFetchStartTime(mmtSessionManager::getBaseTime());

    sShareTrans = sStatement->getShareTransForSmiStmtLock(NULL);
    if (sShareTrans != NULL)
    {
        sStatement->acquireShareTransSmiStmtLock(sShareTrans);
    }

    if (sStatement->getStmtType() == QCI_STMT_DEQUEUE)
    {
        if (sOffset > 1)
        {
            IDE_TEST(fetchEnd(aProtocolContext, sStatement, sResultSetID) != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST(answerFetchMoveResult(aProtocolContext) != IDE_SUCCESS);
        }
    }
    else
    {
        sOffset--;

        // bug-26977: codesonar: resultset null ref
        // null이 가능한지는 모르겠지만, 방어코드추가.
        IDE_TEST(sResultSetStmt == NULL);

        for (i = 0; i < sOffset; i++)
        {
            IDE_TEST(qci::moveNextRecord(sResultSetStmt->getQciStmt(),
                                         sResultSetStmt->getSmiStmt(),
                                         &sRecordExist) != IDE_SUCCESS);

            if (sRecordExist != ID_TRUE)
            {
                sStatement->setFetchFlag(MMC_FETCH_FLAG_CLOSE);
                IDE_TEST(fetchEnd(aProtocolContext, sStatement, sResultSetID) != IDE_SUCCESS);

                break;
            }
        }

        if (sRecordExist == ID_TRUE)
        {
            IDE_TEST(answerFetchMoveResult(aProtocolContext) != IDE_SUCCESS);
        }
    }

    if (sShareTrans != NULL)
    {
        sStatement->releaseShareTransSmiStmtLock(sShareTrans);
        sShareTrans = NULL;
    }

    /* BUG-19456 */
    sStatement->setFetchEndTime(mmtSessionManager::getBaseTime());

    return IDE_SUCCESS;

    IDE_EXCEPTION(NoRows);
    {
        //fix BUG-17689.
        answerFetchEndResult(aProtocolContext,
                             sStatementID,
                             sResultSetID);
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(UnsupportedFetchMove);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_UNSUPPORTED_FETCHMOVE));
    }
    IDE_EXCEPTION_END;

    if (sShareTrans != NULL)
    {
        sStatement->releaseShareTransSmiStmtLock(sShareTrans);
        sShareTrans = NULL;
    }

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, FetchMove),
                                      0);
}

IDE_RC mmtServiceThread::fetchProtocol(cmiProtocolContext *aProtocolContext,
                                       cmiProtocol        *aProtocol,
                                       void               *aSessionOwner,
                                       void               *aUserContext)
{
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    mmcStatement     *sStatement;
    UShort            sColumnCount;
    mmcResultSet    * sResultSet;
    mmcStatement    * sResultSetStmt;

    UInt        sStatementID;
    UShort      sResultSetID;
    UInt        sRecordCountV2;
    UShort      sRecordCount;
    UShort      sColumnFrom;
    UShort      sColumnTo;
    ULong       sReserved;
    IDE_RC      sRC = IDE_FAILURE;

    /* BUG-39463 Add new fetch protocol that can request over 65535 rows. */
    switch (aProtocol->mOpID)
    {
        case CMP_OP_DB_FetchV3:
        case CMP_OP_DB_FetchV2:
            CMI_RD4(aProtocolContext, &sStatementID);
            CMI_RD2(aProtocolContext, &sResultSetID);
            CMI_RD4(aProtocolContext, &sRecordCountV2);
            CMI_RD2(aProtocolContext, &sColumnFrom);
            CMI_RD2(aProtocolContext, &sColumnTo);
            CMI_RD8(aProtocolContext, &sReserved);
            break;

        case CMP_OP_DB_Fetch:
        default:
            /* PROJ-2160 CM 타입제거
            모두 읽은 다음에 프로토콜을 처리해야 한다. */
            CMI_RD4(aProtocolContext, &sStatementID);
            CMI_RD2(aProtocolContext, &sResultSetID);
            CMI_RD2(aProtocolContext, &sRecordCount);
            CMI_RD2(aProtocolContext, &sColumnFrom);
            CMI_RD2(aProtocolContext, &sColumnTo);
            sRecordCountV2 = sRecordCount;
            break;
    }

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sStatementID,
                           sThread) != IDE_SUCCESS);
    /*PROJ-2616*/
    if (sStatement->isSimpleQuerySelectExecuted() == ID_TRUE)
    {
        /**********************
         * A) fetch_begin
         **********************/
        /* for new simple-query IPCDA
         * IPCDA의 심플쿼리에서는 데이터가 Execute에서 SHM으로 복사가
         * 이루어 진다. 따라서, 실제 fetch에서는 아무것도 하지 않는다.
         */
    }
    else
    {
        /* PROJ-1381 FAC : Fetch validation */
        IDE_TEST_RAISE(sStatement->getFetchFlag() == MMC_FETCH_FLAG_INVALIDATED,
                       FetchOutOfSeqError);

        //fix BUG-17689.
        IDE_TEST_RAISE(checkStatementState(sStatement, MMC_STMT_STATE_EXECUTED)
                       != IDE_SUCCESS, NoRows);

        sResultSet = sStatement->getResultSet( sResultSetID );
        // bug-26977: codesonar: resultset null ref
        // null이 가능한지는 모르겠지만, 방어코드임.
        IDE_TEST(sResultSet == NULL);

        sResultSetStmt = (mmcStatement*)sResultSet->mResultSetStmt;

        switch (sStatement->getResultSetState(sResultSetID))
        {
            case MMC_RESULTSET_STATE_INITIALIZE:

                // Result Set의 moveNextReocrd()는 Execute시에 호출 되었다.
                // Result Set이 여러개인 경우 처음 execute시에 record가 없으면
                // MMC_RESULTSET_STATE_INITIALIZE상태이며, 이는 바로 fetchEnd
                sStatement->setFetchFlag(MMC_FETCH_FLAG_CLOSE);

                /* PROJ-1381, BUG-33121 FAC : ResultSet에 FetchEnd 상태 추가 */
            case MMC_RESULTSET_STATE_FETCH_END:
                IDE_RAISE(FetchEnd);
                break;

            case MMC_RESULTSET_STATE_FETCH_READY:
                sStatement->setFetchFlag(MMC_FETCH_FLAG_PROCEED);
                IDE_TEST(answerFetchBeginResult(aProtocolContext, sStatementID, sResultSetID) != IDE_SUCCESS);

                IDE_TEST(sStatement->setResultSetState(sResultSetID, MMC_RESULTSET_STATE_FETCH_PROCEED) != IDE_SUCCESS);
                break;

            case MMC_RESULTSET_STATE_FETCH_PROCEED:
                break;

            default:
                IDE_RAISE(NoCursor);
                break;
        }

        sStatement->setFetchEndTime(0); /* BUG-19456 */
        sStatement->setFetchStartTime(mmtSessionManager::getBaseTime());

        // bug-26977: codesonar: resultset null ref
        // null이 가능한지는 모르겠지만, 방어코드추가.
        IDE_TEST(sResultSetStmt == NULL);
        sColumnCount = qci::getColumnCount(sResultSetStmt->getQciStmt());

        if (sColumnFrom == 0)
        {
            sColumnFrom = 1;
        }

        if (sColumnTo == 0)
        {
            sColumnTo = sColumnCount;
        }
        else
        {
            if (sColumnTo > sColumnCount)
            {
                sColumnTo = sColumnCount;
            }
        }

        if ( cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA )
        {
            IDE_TEST(sThread->fetchIPCDA(aProtocolContext,
                                    sSession,
                                    sStatement,
                                    sResultSetID,
                                    sColumnFrom - 1,
                                    sColumnTo - 1,
                                    sRecordCountV2) != IDE_SUCCESS);

        }
        else
        {
            IDE_TEST(sThread->fetch(aProtocolContext,
                                    sSession,
                                    sStatement,
                                    sResultSetID,
                                    sColumnFrom - 1,
                                    sColumnTo - 1,
                                    sRecordCountV2) != IDE_SUCCESS);
        }

        /* BUG-19456 */
        sStatement->setFetchEndTime(mmtSessionManager::getBaseTime());
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(FetchOutOfSeqError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_FETCH_OUT_OF_SEQ));
    }
    IDE_EXCEPTION(NoRows);
    {
        //fix BUG-17689.
        answerFetchEndResult(aProtocolContext,
                             sStatementID,
                             sResultSetID);
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(FetchEnd);
    {
        IDE_PUSH();

        fetchEnd(aProtocolContext,
                 sStatement,
                 sResultSetID);

        IDE_POP();

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(NoCursor);
    {
        IDE_SET(ideSetErrorCode(mmERR_IGNORE_NO_CURSOR));
    }
    IDE_EXCEPTION_END;

    /* PROJ-2733-Protocol 하위 호환성으로 인해 아래 코드가 추가되었다.
                          CMP_OP_DB_FetchV?가 추가되어도 여기는 신경쓸 필요가 없다. */
    if ((aProtocol->mOpID == CMP_OP_DB_FetchV2) ||
        (aProtocol->mOpID == CMP_OP_DB_Fetch))
    {
        sRC = sThread->answerErrorResult(aProtocolContext,
                                         CMI_PROTOCOL_OPERATION(DB, Fetch),
                                         0);
    }
    else
    {
        sRC = sThread->answerErrorResult(aProtocolContext,
                                         aProtocol->mOpID,
                                         0,
                                         sSession);
    }

    return sRC;
}

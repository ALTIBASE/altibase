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
#include <mmcTask.h>
#include <mtl.h>

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
    UInt   mFromRowNumber;
    UInt   mResultSetCount;
    SLong  mAffectedRowCount;
    SLong  mFetchedRowCount;
    UShort mResultCount;
} mmtBindDataListResult;

#define FLUSH_BIND_DATA_LIST_RESULT(x, p, s, g)                        \
{                                                                      \
    if( (x)->mResultCount > 0 )                                        \
    {                                                                  \
        IDE_TEST( answerParamDataInListResult(                         \
                      (p),                                             \
                      (s),                                             \
                      (g),                                             \
                      (x)->mFromRowNumber,                             \
                      (x)->mFromRowNumber + (x)->mResultCount - 1,     \
                      (x)->mResultSetCount,                            \
                      (x)->mAffectedRowCount,                          \
                      (x)->mFetchedRowCount )                          \
                  != IDE_SUCCESS );                                    \
        (x)->mResultCount = 0;                                         \
    }                                                                  \
}



static IDE_RC answerColumnInfoGetResult(cmiProtocolContext    *aProtocolContext,
                                        UInt                   aStatementID,
                                        UShort                 aResultSetID,
                                        qciStatement          *aQciStmt,
                                        UShort                 aColumnNumber)
{
    qciBindColumn      sBindColumn;
    UInt               sDisplayNameLen;
    UInt               sTotalNameLen;
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sBindColumn.mId = aColumnNumber;

    IDE_TEST(qci::getBindColumnInfo(aQciStmt, &sBindColumn) != IDE_SUCCESS);

    if( sBindColumn.mDisplayNameSize >  MMC_MAX_DISPLAY_NAME_SIZE )
    {
        sDisplayNameLen = mtl::getSafeCutSize( 
              (UChar*)sBindColumn.mDisplayName, 
              MMC_MAX_DISPLAY_NAME_SIZE,
              (UChar*)sBindColumn.mDisplayName + sBindColumn.mDisplayNameSize,
              (mtlModule *)mtl::mDBCharSet );
    }
    else
    {
        sDisplayNameLen = sBindColumn.mDisplayNameSize;
    }


    sTotalNameLen = sDisplayNameLen +
                    sBindColumn.mColumnNameSize +
                    sBindColumn.mBaseColumnNameSize +
                    sBindColumn.mTableNameSize +
                    sBindColumn.mBaseTableNameSize +
                    sBindColumn.mSchemaNameSize +
                    sBindColumn.mCatalogNameSize;

    /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
    // 1+4+2+2+4+4+1+4+4+1+7+sTotalNameLen
    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK_WITH_IPCDA(aProtocolContext, 34 + sTotalNameLen, 34 + sTotalNameLen + 4);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_ColumnInfoGetResult);
    CMI_WR4(aProtocolContext, &aStatementID);
    CMI_WR2(aProtocolContext, &aResultSetID);
    CMI_WR2(aProtocolContext, &aColumnNumber);
    CMI_WR4(aProtocolContext, &sBindColumn.mType);
    CMI_WR4(aProtocolContext, &sBindColumn.mLanguage);
    CMI_WR1(aProtocolContext, sBindColumn.mArguments); // int to char
    CMI_WR4(aProtocolContext, (UInt*)&sBindColumn.mPrecision);
    CMI_WR4(aProtocolContext, (UInt*)&sBindColumn.mScale);
    CMI_WR1(aProtocolContext, sBindColumn.mFlag); // int to char

    CMI_WR1(aProtocolContext, sDisplayNameLen);
    CMI_WCP(aProtocolContext, sBindColumn.mDisplayName, sDisplayNameLen);
    CMI_WR1(aProtocolContext, sBindColumn.mColumnNameSize);
    CMI_WCP(aProtocolContext, sBindColumn.mColumnName, sBindColumn.mColumnNameSize);
    CMI_WR1(aProtocolContext, sBindColumn.mBaseColumnNameSize);
    CMI_WCP(aProtocolContext, sBindColumn.mBaseColumnName, sBindColumn.mBaseColumnNameSize);
    CMI_WR1(aProtocolContext, sBindColumn.mTableNameSize);
    CMI_WCP(aProtocolContext, sBindColumn.mTableName, sBindColumn.mTableNameSize);
    CMI_WR1(aProtocolContext, sBindColumn.mBaseTableNameSize);
    CMI_WCP(aProtocolContext, sBindColumn.mBaseTableName, sBindColumn.mBaseTableNameSize);
    CMI_WR1(aProtocolContext, sBindColumn.mSchemaNameSize);
    CMI_WCP(aProtocolContext, sBindColumn.mSchemaName, sBindColumn.mSchemaNameSize);
    CMI_WR1(aProtocolContext, sBindColumn.mCatalogNameSize);
    CMI_WCP(aProtocolContext, sBindColumn.mCatalogName, sBindColumn.mCatalogNameSize);

    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
    {
        /* PROJ-2616 */
        /* Set max-byte-size in qciBindColumn in IPCDA. */
        CMI_WR4(aProtocolContext, (UInt*)&sBindColumn.mMaxByteSize);
    }
    else
    {
        /* Do nothing. */
    }

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

static IDE_RC answerColumnInfoGetListResult(cmiProtocolContext    *aProtocolContext,
                                            UInt                   aStatementID,
                                            UShort                 aResultSetID,
                                            qciStatement          *aQciStmt,
                                            UShort                 aColumnCount)
{
    qciBindColumn      sBindColumn;
    UInt               sDisplayNameLen;
    UInt               sTotalNameLen;
    UShort             sOrgWriteCursor  = CMI_GET_CURSOR(aProtocolContext);
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 9);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_ColumnInfoGetListResult);
    CMI_WR4(aProtocolContext, &aStatementID);
    CMI_WR2(aProtocolContext, &aResultSetID);
    CMI_WR2(aProtocolContext, &aColumnCount);

    for (sBindColumn.mId = 0; sBindColumn.mId < aColumnCount; sBindColumn.mId++)
    {
        IDE_ASSERT( qci::getBindColumnInfo(aQciStmt, &sBindColumn) == IDE_SUCCESS );

        if( sBindColumn.mDisplayNameSize >  MMC_MAX_DISPLAY_NAME_SIZE )
        {
            sDisplayNameLen = mtl::getSafeCutSize( 
               (UChar*)sBindColumn.mDisplayName, 
               MMC_MAX_DISPLAY_NAME_SIZE,
               (UChar*)sBindColumn.mDisplayName + sBindColumn.mDisplayNameSize,
               (mtlModule *)mtl::mDBCharSet );
        }
        else
        {
            sDisplayNameLen = sBindColumn.mDisplayNameSize;
        }

        sTotalNameLen = sDisplayNameLen +
                        sBindColumn.mColumnNameSize +
                        sBindColumn.mBaseColumnNameSize +
                        sBindColumn.mTableNameSize +
                        sBindColumn.mBaseTableNameSize +
                        sBindColumn.mSchemaNameSize +
                        sBindColumn.mCatalogNameSize;

        /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
        sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
        CMI_WRITE_CHECK_WITH_IPCDA(aProtocolContext, 25 + sTotalNameLen, 25 + sTotalNameLen + 4);
        sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

        CMI_WR4(aProtocolContext, &sBindColumn.mType);
        CMI_WR4(aProtocolContext, &sBindColumn.mLanguage);
        CMI_WR1(aProtocolContext, sBindColumn.mArguments); // int to char
        CMI_WR4(aProtocolContext, (UInt*)&sBindColumn.mPrecision);
        CMI_WR4(aProtocolContext, (UInt*)&sBindColumn.mScale);
        CMI_WR1(aProtocolContext, sBindColumn.mFlag); // int to char

        CMI_WR1(aProtocolContext, sDisplayNameLen);
        CMI_WCP(aProtocolContext, sBindColumn.mDisplayName, sDisplayNameLen);
        CMI_WR1(aProtocolContext, sBindColumn.mColumnNameSize);
        CMI_WCP(aProtocolContext, sBindColumn.mColumnName, sBindColumn.mColumnNameSize);
        CMI_WR1(aProtocolContext, sBindColumn.mBaseColumnNameSize);
        CMI_WCP(aProtocolContext, sBindColumn.mBaseColumnName, sBindColumn.mBaseColumnNameSize);
        CMI_WR1(aProtocolContext, sBindColumn.mTableNameSize);
        CMI_WCP(aProtocolContext, sBindColumn.mTableName, sBindColumn.mTableNameSize);
        CMI_WR1(aProtocolContext, sBindColumn.mBaseTableNameSize);
        CMI_WCP(aProtocolContext, sBindColumn.mBaseTableName, sBindColumn.mBaseTableNameSize);
        CMI_WR1(aProtocolContext, sBindColumn.mSchemaNameSize);
        CMI_WCP(aProtocolContext, sBindColumn.mSchemaName, sBindColumn.mSchemaNameSize);
        CMI_WR1(aProtocolContext, sBindColumn.mCatalogNameSize);
        CMI_WCP(aProtocolContext, sBindColumn.mCatalogName, sBindColumn.mCatalogNameSize);

        if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
        {
            /* PROJ-2616 */
            /* Set max-byte-size in qciBindColumn. */
            CMI_WR4(aProtocolContext, (UInt*)&sBindColumn.mMaxByteSize);
        }
        else
        {
            /* Do nothing. */
        }
    }

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    CMI_SET_CURSOR(aProtocolContext, sOrgWriteCursor);

    return IDE_FAILURE;
}

static IDE_RC answerParamInfoGetResult(cmiProtocolContext   *aProtocolContext,
                                       UInt                  aStatementID,
                                       UShort                aParamNumber,
                                       qciBindParam         *aBindParam )
{
    UChar              sByte;
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    // 1+4+2+4+4+1+4+4+1+1
    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 26);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_ParamInfoGetResult);
    CMI_WR4(aProtocolContext, &aStatementID);
    CMI_WR2(aProtocolContext, &aParamNumber);
    CMI_WR4(aProtocolContext, &(aBindParam->type));
    CMI_WR4(aProtocolContext, &(aBindParam->language));

    sByte = aBindParam->arguments;
    CMI_WR1(aProtocolContext, sByte);

    CMI_WR4(aProtocolContext, (UInt*)&(aBindParam->precision));
    CMI_WR4(aProtocolContext, (UInt*)&(aBindParam->scale));

    sByte = aBindParam->inoutType;
    CMI_WR1(aProtocolContext, sByte);

    sByte = 0;
    CMI_WR1(aProtocolContext, sByte);

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

static IDE_RC answerParamInfoSetListResult(cmiProtocolContext *aProtocolContext)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 1);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_ParamInfoSetListResult);

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

static IDE_RC answerParamDataInListResult(cmiProtocolContext *aProtocolContext,
                                          mmcSession         *aSession,
                                          UInt                aStatementID,
                                          UInt                aFromRowNumber,
                                          UInt                aToRowNumber,
                                          UShort              aResultSetCount,
                                          SLong               aAffectedRowCount,
                                          SLong               aFetchedRowCount )
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
    smSCN sSCN;

    SM_INIT_SCN(&sSCN);

    /* PROJ-2733-DistTxInfo */
    if ( aSession->isGCTx() == ID_TRUE )
    {
        aSession->getLastSystemSCN( aProtocolContext->mProtocol.mOpID, &sSCN );
    }

    #if defined(DEBUG)
    ideLog::log(IDE_SD_18, "= [%s] answerParamDataInListResult, SCN : %"ID_UINT64_FMT,
                aSession->getSessionTypeString(),
                sSCN);
    #endif

    switch (aProtocolContext->mProtocol.mOpID)
    {
        case CMP_OP_DB_ParamDataInListV3:  /* PROJ-2733-Protocol */
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 39);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, CMP_OP_DB_ParamDataInListV3Result);
            CMI_WR4(aProtocolContext, &aStatementID);
            CMI_WR4(aProtocolContext, &aFromRowNumber);
            CMI_WR4(aProtocolContext, &aToRowNumber);
            CMI_WR2(aProtocolContext, &aResultSetCount);
            CMI_WR8(aProtocolContext, (ULong *)&aAffectedRowCount);
            CMI_WR8(aProtocolContext, (ULong *)&aFetchedRowCount);
            CMI_WR8(aProtocolContext, &sSCN);
            break;

        case CMP_OP_DB_ParamDataInListV2:  /* BUG-44572 */
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 31);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, CMP_OP_DB_ParamDataInListV2Result);
            CMI_WR4(aProtocolContext, &aStatementID);
            CMI_WR4(aProtocolContext, &aFromRowNumber);
            CMI_WR4(aProtocolContext, &aToRowNumber);
            CMI_WR2(aProtocolContext, &aResultSetCount);
            CMI_WR8(aProtocolContext, (ULong *)&aAffectedRowCount);
            CMI_WR8(aProtocolContext, (ULong *)&aFetchedRowCount);
            break;

        case CMP_OP_DB_ParamDataInList:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 23);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, CMP_OP_DB_ParamDataInListResult);
            CMI_WR4(aProtocolContext, &aStatementID);
            CMI_WR4(aProtocolContext, &aFromRowNumber);
            CMI_WR4(aProtocolContext, &aToRowNumber);
            CMI_WR2(aProtocolContext, &aResultSetCount);
            CMI_WR8(aProtocolContext, (ULong *)&aAffectedRowCount);
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

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

// bug-28259: ipc needs paramDataInResult
// ipc인 경우 protocol에 대한 요청/응답 짝이 맞아야 한다.
// 그래서 paramDataIn에 대한 dummy 응답 protocol 추가.
static IDE_RC answerParamDataInResult(cmiProtocolContext *aProtocolContext)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 1);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_ParamDataInResult);

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
 Description : cm 데이타를 QP 의 메모리로 복사해주는 함수
               A7의 경우 동일한 데이타를 사용하므로 별도의 컨버젼이 필요없다.
********************************************************************/
IDE_RC mmtServiceThread::setParamData4RebuildCallback(idvSQL * /* aStatistics */,
                                                      void   * aBindParam,
                                                      void   * aTarget,
                                                      UInt     aTargetSize,
                                                      void   * aTemplate,
                                                      void   * aSession4Rebuild,
                                                      void   * aBindData)
{
    mmcSession         *sSession   = (mmcSession *)aSession4Rebuild;
    cmiProtocolContext *sCtx       = sSession->getTask()->getProtocolContext();
    cmtAny              sAny;
    UInt                sCursor    = 0;
    qciBindParam       *sBindParam = (qciBindParam *) aBindParam;

    if( cmiGetPacketType(sCtx) == CMP_PACKET_TYPE_A5 )
    {
        IDE_TEST( cmtAnyInitialize( &sAny ) != IDE_SUCCESS );

        IDE_TEST( cmtCollectionReadAnyNext( (UChar*) aBindData,
                                            &sCursor,
                                            &sAny )
                != IDE_SUCCESS );

        IDE_TEST(mmcConv::convertFromCM( sBindParam,
                                         aTarget,
                                         aTargetSize,
                                         &sAny,
                                         aTemplate,
                                         (mmcSession *) aSession4Rebuild )
                 != IDE_SUCCESS);
    }
    else
    {
        idlOS::memcpy( aTarget, aBindData, aTargetSize);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//=========================================================
// proj_2160 cm_type removal
// write mt-typed data instead of deprecated cmtAny-typed data.
UInt profWriteBindCallback(idvProfBind     *aBindInfo,
                           idvProfDescInfo *aInfo,
                           UInt             aInfoBegin,
                           UInt             aCurrSize)
{
    UInt           sDescIdx   = aInfoBegin;
    UInt           sTotalSize = aCurrSize;
    UChar         *sSrcData   = (UChar*)(aBindInfo->mData);
    UInt           sDescLen   = 0;

    UChar          sLen8;
    UShort         sLen16;
    UInt           sLen32;
 
    // parameter 번호 (1-base), mtdType 기록
    aInfo[sDescIdx].mData   = aBindInfo; // structure copy
    aInfo[sDescIdx].mLength = 8;         // mParamNo(4) + mType(4)
    sTotalSize += 8;
    sDescIdx++;

    // data 기록
    switch (aBindInfo->mType)
    {
        case MTD_NULL_ID :
            sDescLen = ID_SIZEOF(UChar);
            break;

        case MTD_BINARY_ID  :
            // 원본 qp 데이터는 align이 안되어 있다
            ID_4_BYTE_ASSIGN(&sLen32, sSrcData);
            sDescLen = ID_SIZEOF(SDouble) + sLen32;
            break;

        case MTD_CHAR_ID    :
        case MTD_VARCHAR_ID :
        case MTD_NCHAR_ID   :
        case MTD_NVARCHAR_ID:
        case MTD_BYTE_ID    :
        case MTD_VARBYTE_ID    :
            ID_2_BYTE_ASSIGN(&sLen16, sSrcData);
            sDescLen = ID_SIZEOF(UShort) + sLen16;
            break;

        case MTD_FLOAT_ID  :
        case MTD_NUMERIC_ID:
        case MTD_NUMBER_ID :
            ID_1_BYTE_ASSIGN(&sLen8, sSrcData);
            sDescLen = ID_SIZEOF(UChar) + sLen8;
            break;

        case MTD_SMALLINT_ID:
            sDescLen = ID_SIZEOF(mtdSmallintType);
            break;

        case MTD_INTEGER_ID:
            sDescLen = ID_SIZEOF(mtdIntegerType);
            break;

        case MTD_BIGINT_ID :
            sDescLen = ID_SIZEOF(mtdBigintType);
            break;

        case MTD_REAL_ID :
            sDescLen = ID_SIZEOF(mtdRealType);
            break;

        case MTD_DOUBLE_ID :
            sDescLen = ID_SIZEOF(mtdDoubleType);
            break;

        case MTD_BLOB_LOCATOR_ID :
        case MTD_CLOB_LOCATOR_ID :
            sDescLen = ID_SIZEOF(smLobLocator) + ID_SIZEOF(UInt);
            break;

        case MTD_DATE_ID:
            sDescLen = ID_SIZEOF(mtdDateType);
            break;

        case MTD_INTERVAL_ID:
            sDescLen = ID_SIZEOF(mtdIntervalType);
            break;

        case MTD_BIT_ID :
        case MTD_VARBIT_ID :
            ID_4_BYTE_ASSIGN(&sLen32, sSrcData);
            sLen32 = BIT_TO_BYTE(sLen32);
            sDescLen = ID_SIZEOF(UInt) + sLen32;
            break;

        case MTD_NIBBLE_ID :
            ID_1_BYTE_ASSIGN(&sLen8, sSrcData);
            if (sLen8 != MTD_NIBBLE_NULL_LENGTH)
            {
                sLen8 = ((sLen8 + 1) >> 1);
            }
            else
            {
                sLen8 = 0;
            }
            sDescLen = ID_SIZEOF(UChar) + sLen8;
            break;

        /* ------------------------------------------------
         *  Invalid Data Type
         * ----------------------------------------------*/
        default:
            ideLog::log(IDE_SERVER_0,
                        "[WARNING] Invalid Bind"
                        " Data Type Profiling ==> %u\n",
                        aBindInfo->mType);
            break;
    }

    aInfo[sDescIdx].mData   = sSrcData;
    aInfo[sDescIdx].mLength = sDescLen;
    // this is used to save the total size of 2 descriptors
    sTotalSize += sDescLen;

    sDescIdx++;
    aInfo[sDescIdx].mData = NULL;

    return sTotalSize; // ipvProfHeader.mSize
}

IDE_RC mmtServiceThread::columnInfoGetProtocol(cmiProtocolContext *aProtocolContext,
                                               cmiProtocol        *,
                                               void               *aSessionOwner,
                                               void               *aUserContext)
{
    mmcTask                   *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread          *sThread = (mmtServiceThread *)aUserContext;
    mmcSession                *sSession;
    mmcStatement              *sStatement;
    qciStatement              *sQciStmt;
    mmcResultSet              *sResultSet;
    mmcStatement              *sResultSetStmt;
    UShort                     sColumnCount;

    UInt          sStatementID;
    UShort        sResultSetID;
    UShort        sColumnNumber;

    CMI_RD4(aProtocolContext, &sStatementID);
    CMI_RD2(aProtocolContext, &sResultSetID);
    CMI_RD2(aProtocolContext, &sColumnNumber);
    // protocol read complete

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sStatementID,
                           sThread) != IDE_SUCCESS);

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

    sQciStmt     = sResultSetStmt->getQciStmt();
    sColumnCount = qci::getColumnCount(sQciStmt);

    IDE_TEST_RAISE(sColumnCount == 0, NoColumn);

    // BUG-17544
    // Prepare 후 컬럼 정보를 보냈으면 다시 컬럼 정보를 보내지 않는다.
    IDE_TEST_RAISE(sStatement->getSendBindColumnInfo() == ID_TRUE, NoError);

    if (sColumnNumber == 0)
    {
        IDE_TEST( answerColumnInfoGetListResult( aProtocolContext,
                                                 sStatementID,
                                                 sResultSetID,
                                                 sQciStmt,
                                                 sColumnCount )
                    != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST_RAISE(sColumnNumber > sColumnCount, InvalidColumn);

        IDE_TEST(answerColumnInfoGetResult(aProtocolContext,
                                           sStatementID,
                                           sResultSetID,
                                           sQciStmt,
                                           sColumnNumber - 1)
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

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, ColumnInfoGet),
                                      0);
}

IDE_RC mmtServiceThread::paramInfoGetProtocol(cmiProtocolContext *aProtocolContext,
                                              cmiProtocol        *,
                                              void               *aSessionOwner,
                                              void               *aUserContext)
{
    mmcTask                  *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread         *sThread = (mmtServiceThread *)aUserContext;
    mmcSession               *sSession;
    mmcStatement             *sStatement;
    qciBindParam              sBindParam;
    UShort                    sParamCount;

    UInt        sStatementID;
    UShort      sParamNumber;

    CMI_RD4(aProtocolContext, &sStatementID);
    CMI_RD2(aProtocolContext, &sParamNumber);
    // protocol read complete

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sStatementID,
                           sThread) != IDE_SUCCESS);

    IDE_TEST(checkStatementState(sStatement, MMC_STMT_STATE_PREPARED) != IDE_SUCCESS);

    sParamCount = qci::getParameterCount(sStatement->getQciStmt());

    if (sParamNumber == 0)
    {
        IDE_TEST_RAISE(sParamCount == 0, NoParameter);

        for (sBindParam.id = 0; sBindParam.id < sParamCount; sBindParam.id++)
        {
            IDE_TEST(qci::getBindParamInfo(sStatement->getQciStmt(), &sBindParam) != IDE_SUCCESS);

            IDE_TEST(answerParamInfoGetResult(aProtocolContext,
                                              sStatementID,
                                              sBindParam.id + 1,
                                              &sBindParam) != IDE_SUCCESS);
        }
    }
    else
    {
        IDE_TEST_RAISE(sParamNumber > sParamCount, InvalidParameter);

        sBindParam.id = sParamNumber - 1;

        IDE_TEST(qci::getBindParamInfo(sStatement->getQciStmt(), &sBindParam) != IDE_SUCCESS);

        IDE_TEST(answerParamInfoGetResult(aProtocolContext,
                                          sStatementID,
                                          sParamNumber,
                                          &sBindParam) != IDE_SUCCESS);
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

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, ParamInfoGet),
                                      0);
}

/*
 * PROJ-1697 : bind parameter의 information list를 처리하는 프로토콜
 */
IDE_RC mmtServiceThread::paramInfoSetListProtocol(cmiProtocolContext *aProtocolContext,
                                                  cmiProtocol        *,
                                                  void               *aSessionOwner,
                                                  void               *aUserContext)
{
    mmcTask             *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    mmcStatement        *sStatement;
    UShort               sBindId;
    qciBindParam         sBindParam;

    UInt                 sStatementID;
    UShort               sParamCount;
    UChar                sByte;
    UShort               sOrgCursor;

    qciStatement        *sQciStmt;
    UShort               sRealParamCount;

    CMI_RD4(aProtocolContext, &sStatementID);
    CMI_RD2(aProtocolContext, &sParamCount);

    sOrgCursor = aProtocolContext->mReadBlock->mCursor;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sStatementID,
                           sThread) != IDE_SUCCESS);

    IDE_TEST_RAISE(sStatement->getStmtState() != MMC_STMT_STATE_PREPARED,
                   InvalidStatementState);

    sQciStmt        = sStatement->getQciStmt();
    sRealParamCount = qci::getParameterCount(sQciStmt);

    IDE_TEST_RAISE( sParamCount < sRealParamCount, column_count_mismatch );

    if (sStatement->getBindState() > MMC_STMT_BIND_NONE)
    {
        IDE_TEST(qci::clearStatement(sQciStmt,
                                     NULL,
                                     QCI_STMT_STATE_PREPARED) != IDE_SUCCESS);

        sStatement->setBindState(MMC_STMT_BIND_NONE);
    }

    for ( sBindId = 0; sBindId < sRealParamCount; sBindId++)
    {
        CMI_RD2(aProtocolContext, &(sBindParam.id));
        sBindParam.id -= 1;

        CMI_RD4(aProtocolContext, &(sBindParam.type));
        CMI_RD4(aProtocolContext, &(sBindParam.language));
        if( sBindParam.language == 0 )
        {
            sBindParam.language = sSession->getLanguage()->id;
        }

        CMI_RD1(aProtocolContext, sByte);
        sBindParam.arguments = (UInt) sByte; // char to int

        CMI_RD4(aProtocolContext, (UInt*)&(sBindParam.precision));
        CMI_RD4(aProtocolContext, (UInt*)&(sBindParam.scale));

        CMI_RD1(aProtocolContext, sByte);
        sBindParam.inoutType = (UInt) sByte; // char to int

        /* PROJ-2616 */
        if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
        {
            /* 1) CTYPE for SimpleQuery */
            CMI_RD2(aProtocolContext, (UShort*)&sBindParam.ctype);
        }

        IDE_TEST( qci::setBindParamInfo(sQciStmt, &sBindParam) != IDE_SUCCESS);
    }

    /* PROJ-2616 */
    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
    {
        /* IPCDA & SimpleQuery인 경우에 C_TYPE이 추가 된다. 그래서, 아래의
         * mCursor의 위치 변경에서 추가 된 C_TYPE 만큼 길이를 더해 줘야 한다.
         */
        aProtocolContext->mReadBlock->mCursor = sOrgCursor + (sParamCount * (20 + 2));
    }
    else
    {
        aProtocolContext->mReadBlock->mCursor = sOrgCursor + (sParamCount * 20);
    }

    if( sRealParamCount == 0 )
    {
        // IDE_TEST(qci::bindParamInfo(sQciStmt) != IDE_SUCCESS);
    }

    sStatement->setBindState(MMC_STMT_BIND_INFO);

    return answerParamInfoSetListResult(aProtocolContext);

    IDE_EXCEPTION(InvalidStatementState);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION(column_count_mismatch);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_BIND_COLUMN_COUNT_MISMATCH));
    }
    IDE_EXCEPTION_END;

    aProtocolContext->mReadBlock->mCursor = sOrgCursor + (sParamCount * 20);

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, ParamInfoSetList),
                                      0);
}

static IDE_RC mmtCopyBindParamDataCallback(qciBindParam *aBindParam, UChar *aSource, UInt *aStructSize)
{
    UInt             sLen32    = 0;
    UShort           sLen16    = 0;
    UChar            sLen8     = 0;
    mtdDateType     *sDate;
    mtdIntervalType *sInterval;
    UInt             sStructSize;

    /* BUG-43294 The server raises an assertion becauseof bindings to NULL columns */
    IDU_FIT_POINT_RAISE( "mmtCopyBindParamDataCallback::aBindParam::type", InvalidType );

    // endian을 고려해서 읽어야한다.
    // aSource 메모리 는 align이 안되어 있다.
    switch (aBindParam->type)
    {
        case MTD_BINARY_ID :
            CM_ENDIAN_ASSIGN4(&sLen32, aSource);
            sStructSize = MTD_BINARY_TYPE_STRUCT_SIZE(sLen32);
            IDE_TEST_RAISE(sStructSize > aBindParam->dataSize, InvalidSize);
            ((mtdBinaryType*)aBindParam->data)->mLength = sLen32;

            idlOS::memcpy( ((mtdBinaryType*)aBindParam->data)->mValue,
                           aSource + ID_SIZEOF(SDouble),
                           sLen32 );
            break;

        case MTD_CHAR_ID:
        case MTD_VARCHAR_ID:
            CM_ENDIAN_ASSIGN2(&sLen16, aSource);
            // 복사하기 전에 버퍼크기를 넘는지 확인해야 한다
            sStructSize = MTD_CHAR_TYPE_STRUCT_SIZE(sLen16);
            IDE_TEST_RAISE(sStructSize > aBindParam->dataSize, InvalidSize);

            ((mtdCharType*)aBindParam->data)->length = sLen16;

            idlOS::memcpy( ((mtdCharType*)aBindParam->data)->value,
                           aSource + ID_SIZEOF(UShort),
                           sLen16 );
            break;

        case MTD_NCHAR_ID :
        case MTD_NVARCHAR_ID :
            CM_ENDIAN_ASSIGN2(&sLen16, aSource);
            sStructSize = MTD_CHAR_TYPE_STRUCT_SIZE(sLen16);
            IDE_TEST_RAISE( sStructSize > aBindParam->dataSize, InvalidSize );

            ((mtdNcharType*)aBindParam->data)->length = sLen16;

            idlOS::memcpy( ((mtdNcharType*)aBindParam->data)->value,
                           aSource + ID_SIZEOF(UShort),
                           sLen16 );
            break;

        case MTD_FLOAT_ID :
        case MTD_NUMERIC_ID :
        case MTD_NUMBER_ID :
            CM_ENDIAN_ASSIGN1(sLen8, *(aSource));
            sStructSize = ID_SIZEOF(UChar) + sLen8;
            IDE_TEST_RAISE(sStructSize > aBindParam->dataSize, InvalidSize);

            idlOS::memcpy( aBindParam->data,
                           aSource,
                           sLen8 + ID_SIZEOF(UChar));
            break;

        case MTD_SMALLINT_ID:
            CM_ENDIAN_ASSIGN2(aBindParam->data, aSource);
            sStructSize = ID_SIZEOF(UShort);
            break;

        case MTD_INTEGER_ID:
            CM_ENDIAN_ASSIGN4(aBindParam->data, aSource);
            sStructSize = ID_SIZEOF(UInt);
            break;

        case MTD_BIGINT_ID :
            CM_ENDIAN_ASSIGN8(aBindParam->data, aSource);
            sStructSize = ID_SIZEOF(ULong);
            break;

        case MTD_REAL_ID :
            CM_ENDIAN_ASSIGN4(aBindParam->data, aSource);
            sStructSize = ID_SIZEOF(SFloat);
            break;

        case MTD_DOUBLE_ID :
            CM_ENDIAN_ASSIGN8(aBindParam->data, aSource);
            sStructSize = ID_SIZEOF(SDouble);
            break;

        case MTD_BLOB_LOCATOR_ID :
        case MTD_CLOB_LOCATOR_ID :
            CM_ENDIAN_ASSIGN8(aBindParam->data, aSource);
            sStructSize = ID_SIZEOF(ULong);
            break;

        case MTD_DATE_ID:
            sDate = (mtdDateType*)(aBindParam->data);
            CM_ENDIAN_ASSIGN2(&(sDate->year), aSource);
            CM_ENDIAN_ASSIGN2(&(sDate->mon_day_hour), aSource+ID_SIZEOF(UShort));
            CM_ENDIAN_ASSIGN4(&(sDate->min_sec_mic ), aSource+ID_SIZEOF(UInt));
            sStructSize = ID_SIZEOF(mtdDateType);
            break;

        case MTD_INTERVAL_ID:
            sInterval = (mtdIntervalType*)(aBindParam->data);
            CM_ENDIAN_ASSIGN8(&(sInterval->second), aSource);
            CM_ENDIAN_ASSIGN8(&(sInterval->microsecond), aSource+ID_SIZEOF(ULong));
            sStructSize = ID_SIZEOF(mtdIntervalType);
            break;

        case MTD_BIT_ID :
        case MTD_VARBIT_ID :
            CM_ENDIAN_ASSIGN4(&sLen32, aSource);

            if( sLen32 != 0 )
            {
                sStructSize = MTD_BIT_TYPE_STRUCT_SIZE(sLen32);
            }
            else
            {
                sStructSize = ID_SIZEOF(UInt);
            }

            IDE_TEST_RAISE(sStructSize > aBindParam->dataSize, InvalidSize);
            IDE_TEST_RAISE(sLen32 > (UInt)(aBindParam->precision),
                           InvalidSize);

            idlOS::memcpy(aBindParam->data,
                          aSource,
                          sStructSize);

            ((mtdBitType*)aBindParam->data)->length = sLen32;

            break;

        case MTD_NIBBLE_ID :
            CM_ENDIAN_ASSIGN1(sLen8, *(aSource));

            if (sLen8 != MTD_NIBBLE_NULL_LENGTH)
            {
                sStructSize = MTD_NIBBLE_TYPE_STRUCT_SIZE(sLen8);
            }
            else
            {
                sStructSize = ID_SIZEOF(UChar);
            }

            IDE_TEST_RAISE(sStructSize > aBindParam->dataSize, InvalidSize);

            idlOS::memcpy(aBindParam->data,
                          aSource,
                          sStructSize);

            ((mtdNibbleType*)aBindParam->data)->length = sLen8;
            break;

        case MTD_BYTE_ID :
        case MTD_VARBYTE_ID :
            CM_ENDIAN_ASSIGN2(&sLen16, aSource);
            sStructSize = MTD_BYTE_TYPE_STRUCT_SIZE(sLen16);
            IDE_TEST_RAISE(sStructSize > aBindParam->dataSize, InvalidSize);

            ((mtdByteType*)aBindParam->data)->length = sLen16;

            idlOS::memcpy( ((mtdByteType*)aBindParam->data)->value,
                           aSource + ID_SIZEOF(UShort),
                           sLen16 );
            break;

        default:
            IDE_RAISE(InvalidType);
            break;
    }

    if (aStructSize != NULL)
    {
        *aStructSize = sStructSize;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE,
                                aBindParam->id, getMtTypeName(aBindParam->type),
                                sStructSize, aBindParam->dataSize));
    }
    /* BUG-43294 The server raises an assertion becauseof bindings to NULL columns */
    IDE_EXCEPTION( InvalidType )
    {
        ideLog::log(IDE_SERVER_0,
                    MM_TRC_INVALID_DATA_TYPE,
                    aBindParam->id,
                    aBindParam->type,
                    aBindParam->language,
                    aBindParam->arguments,
                    aBindParam->precision,
                    aBindParam->scale,
                    aBindParam->inoutType,
                    aBindParam->dataSize);

        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_TYPE,
                                aBindParam->id,
                                aBindParam->type));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::paramDataInProtocol(cmiProtocolContext *aProtocolContext,
                                             cmiProtocol        *,
                                             void               *aSessionOwner,
                                             void               *aUserContext)
{
    mmcTask           *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread  *sThread = (mmtServiceThread *)aUserContext;
    mmcSession        *sSession;
    mmcStatement      *sStatement;

    UChar             *sRow    = NULL;
    UShort             sParamCount;
    IDE_RC             sRc = IDE_SUCCESS;

    UInt               sStatementID;
    UShort             sParamNumber;
    UInt               sRowSize;

    qciStatement      *sQciStmt;
    qciBindParam       sBindParam;
    idvProfBind        sProfBind;

    CMI_RD4(aProtocolContext, &sStatementID);
    CMI_RD2(aProtocolContext, &sParamNumber);
    CMI_RD4(aProtocolContext, &sRowSize);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sStatementID,
                           sThread) != IDE_SUCCESS);

    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
    {
        IDE_TEST_RAISE( cmiSplitReadIPCDA( aProtocolContext,
                                           sRowSize,
                                           &sRow,
                                           NULL)
                        != IDE_SUCCESS, cm_error );
    }
    else
    {
        IDE_TEST( sSession->allocChunk( sRowSize ) != IDE_SUCCESS );

        sRow = sSession->getChunk();

        IDE_TEST_RAISE( cmiSplitRead( aProtocolContext,
                                      sRowSize,
                                      sRow,
                                      NULL )
                        != IDE_SUCCESS, cm_error );
    }
    sRowSize = 0;

    IDE_TEST_RAISE( (sStatement->getAtomicExecSuccess() != ID_TRUE), SkipExecute);

    IDE_TEST_RAISE(sStatement->getStmtState() != MMC_STMT_STATE_PREPARED,
                   InvalidStatementState);

    sQciStmt = sStatement->getQciStmt();
    sParamCount = qci::getParameterCount(sQciStmt);

    IDE_TEST_RAISE(sParamNumber > sParamCount, NoParameter);
    IDE_TEST_RAISE(sParamNumber == 0, InvalidParameter);

    if (sStatement->getBindState() > MMC_STMT_BIND_INFO)
    {
        sStatement->setBindState(MMC_STMT_BIND_INFO);
    }
    else
    {
        // Nothing to do.
    }

    if(qci::setBindParamData(sQciStmt, sParamNumber - 1, sRow, mmtCopyBindParamDataCallback) != IDE_SUCCESS)
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
    // 변경: stmt->mTrans를 구하여 null이면 TransID로 0을 넘기도로 수정
    if ((idvProfile::getProfFlag() & IDV_PROF_TYPE_BIND_FLAG) == IDV_PROF_TYPE_BIND_FLAG)
    {
        mmcTransObj *sTrans = sSession->getTransPtr(sStatement);
        smTID sTransID = (sTrans != NULL) ? mmcTrans::getTransID(sTrans) : 0;

        sBindParam.id = sParamNumber - 1;
        IDE_TEST(qci::getBindParamInfo(sQciStmt, &sBindParam) != IDE_SUCCESS);
        sProfBind.mParamNo = sParamNumber;
        sProfBind.mType = sBindParam.type;
        sProfBind.mData = sBindParam.data;
        idvProfile::writeBind(&sProfBind,
                              sStatement->getSessionID(),
                              sStatement->getStmtID(),
                              sTransID, 
                              profWriteBindCallback);
    }

    if ( qci::isLastParamData ( sQciStmt,
                                sParamNumber - 1 ) == ID_TRUE )
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
    if ((cmiGetLinkImpl( aProtocolContext ) == CMI_LINK_IMPL_IPC) ||
        (cmiGetLinkImpl( aProtocolContext ) == CMI_LINK_IMPL_IPCDA))
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
    IDE_EXCEPTION(cm_error);
    {
        return IDE_FAILURE;
    }
    IDE_EXCEPTION_END;

    if( sRowSize != 0 )
    {
        IDE_TEST_RAISE( cmiSplitSkipRead( aProtocolContext,
                                          sRowSize,
                                          NULL )
                        != IDE_SUCCESS, cm_error );
    }

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, ParamDataIn),
                                      0);
}

/*
 * PROJ-1697
 * bind parameter의 data list를 처리하는 프로토콜
 */
IDE_RC mmtServiceThread::paramDataInListProtocol(cmiProtocolContext *aProtocolContext,
                                                 cmiProtocol        *aProtocol,
                                                 void               *aSessionOwner,
                                                 void               *aUserContext)
{
    mmcTask                     *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread            *sThread = (mmtServiceThread *)aUserContext;
    mmcSession                  *sSession;
    mmcStatement                *sStatement;

    UChar                       *sRow    = NULL;
    UInt                         sOffset  = 0;
    ULong                        sRowSize = 0;
    UShort                       sParamCount;
    idBool                       sSuspended          = ID_FALSE;

    UInt                         sResultSetCount     = 0;
    SLong                        sAffectedRowCount   = 0;
    SLong                        sFetchedRowCount    = 0;
    mmtBindDataListResult        sResult;

    qciStatement                *sQciStmt;
    qciBindParam                 sBindParam;

    UShort        sBindColNum    = 0;    /*PROJ-2616*/
    UShort       *sBindColInfo   = NULL; /*PROJ-2616*/

    UInt          sStatementID;
    UInt          sRowNumber = 0;
    UInt          sFromRowNumber;
    UInt          sToRowNumber;
    UChar         sExecuteOption = CMP_DB_EXECUTE_NONE;
    idvProfBind   sProfBind;

    /* PROJ-2733-DistTxInfo */
    smSCN         sRequestSCN;
    smSCN         sTxFirstStmtSCN;
    SLong         sTxFirstStmtTime = 0;
    UChar         sDistLevel   = 0;

    UInt          sClientTouchNodeArr[SDI_NODE_MAX_COUNT] = {0, };
    UShort        sClientTouchNodeCount = 0;
    UInt          i = 0;

    /* TASK-7219 Non-shard DML */
    UInt           sStmtExecSeqForShardTx = 0;

    SM_INIT_SCN(&sRequestSCN);
    SM_INIT_SCN(&sTxFirstStmtSCN);

    /* PROJ-2733-Protocol */
    switch (aProtocol->mOpID)
    {
        case CMP_OP_DB_ParamDataInListV3:
            CMI_RD4(aProtocolContext, &sStatementID);
            CMI_RD4(aProtocolContext, &sFromRowNumber);
            CMI_RD4(aProtocolContext, &sToRowNumber);
            CMI_RD1(aProtocolContext, sExecuteOption);
            /* PROJ-2733-DistTxInfo */
            CMI_RD8(aProtocolContext, &sRequestSCN);
            CMI_RD8(aProtocolContext, &sTxFirstStmtSCN);
            CMI_RD8(aProtocolContext, (ULong *)&sTxFirstStmtTime);
            CMI_RD1(aProtocolContext, sDistLevel);
            /* TASK-7219 Non-shard DML */
            CMI_RD4(aProtocolContext, &sStmtExecSeqForShardTx);
            /* BUG-48489 */
            CMI_RD2(aProtocolContext, &sClientTouchNodeCount);
            IDE_DASSERT(sClientTouchNodeCount <= SDI_NODE_MAX_COUNT);
            for (i = 0; i < sClientTouchNodeCount; i++)
            {
                CMI_RD4(aProtocolContext, &(sClientTouchNodeArr[i]));
            }
            CMI_RD8(aProtocolContext, &sRowSize);
            break;

        case CMP_OP_DB_ParamDataInListV2:
        case CMP_OP_DB_ParamDataInList:
            CMI_RD4(aProtocolContext, &sStatementID);
            CMI_RD4(aProtocolContext, &sFromRowNumber);
            CMI_RD4(aProtocolContext, &sToRowNumber);
            CMI_RD1(aProtocolContext, sExecuteOption);
            CMI_RD8(aProtocolContext, &sRowSize);
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sStatementID,
                           sThread) != IDE_SUCCESS);

    IDE_TEST_RAISE(sStatement->getStmtState() != MMC_STMT_STATE_PREPARED,
                   InvalidStatementState);

    sQciStmt    = sStatement->getQciStmt();

    sResult.mFromRowNumber    = sFromRowNumber;
    sResult.mResultSetCount   = 0;
    sResult.mAffectedRowCount = -1;
    sResult.mFetchedRowCount  = -1;
    sResult.mResultCount      = 0;

    #if defined(DEBUG)
    ideLog::log(IDE_SD_18, "= [%s] mmtServiceThread::paramDataInListProtocol"
                              ", RequestSCN : %"ID_UINT64_FMT
                              ", TxFirstStmtSCN : %"ID_UINT64_FMT
                              ", TxFirstStmtTime : %"ID_INT64_FMT
                              ", DistLevel : %"ID_UINT32_FMT,
                sSession->getSessionTypeString(),
                sRequestSCN,
                sTxFirstStmtSCN,
                sTxFirstStmtTime,
                sDistLevel);
    #endif

    /* PROJ-2733-DistTxInfo */
    sStatement->setRequestSCN(&sRequestSCN);
    sStatement->setTxFirstStmtSCN(&sTxFirstStmtSCN);
    sStatement->setTxFirstStmtTime(sTxFirstStmtTime);
    sStatement->setDistLevel((sdiDistLevel)sDistLevel);

    /* TASK-7219 Non-shard DML */
    if ( sSession->getShardSessionType() != SDI_SESSION_TYPE_USER )
    {
        sSession->setStmtExecSeqForShardTx(sStmtExecSeqForShardTx);
    }

    // PROJ-1518
    if( sThread->atomicCheck( sStatement, &(sExecuteOption) ) != IDE_SUCCESS )
    {
        /* PROJ-2160 CM 타입제거
           모두 읽은 다음에 프로토콜을 처리해야 한다. */
        IDE_TEST_RAISE( cmiSplitSkipRead( aProtocolContext,
                                          sRowSize,
                                          NULL )
                        != IDE_SUCCESS, cm_error );
       sRowSize = 0;
       IDE_RAISE( SkipExecute);
    }

    //fix BUG-29824 A array binds should not be permitted with SELECT statement.
    IDE_TEST_RAISE( ((sStatement->getStmtType() == QCI_STMT_SELECT) &&
                    (sExecuteOption == CMP_DB_EXECUTE_ARRAY_EXECUTE)) ,
                    InvalidArrayBinds);

    sSession->setClientTouchNodeCount(sClientTouchNodeCount);  /* BUG-48384 */

    /* BUG-48384 ShardCLI, User|LibSession에서만 TouchNode를 보낸다. */
    if (sClientTouchNodeCount > 0)
    {
        IDE_DASSERT(sSession->getShardSessionType() != SDI_SESSION_TYPE_COORD);
        IDE_DASSERT(sSession->isShardClient() == SDI_SHARD_CLIENT_TRUE);

        /* UserSession에서만 터치를 한다. */
        if ((sSession->getShardSessionType() == SDI_SESSION_TYPE_USER) &&
            (sSession->isShardClient() == SDI_SHARD_CLIENT_TRUE))
        {
            for (i = 0; i < sClientTouchNodeCount; i++)
            {
                IDE_TEST(sSession->touchShardNode(sClientTouchNodeArr[i]) != IDE_SUCCESS);
            }
        }
    }

    if ( (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) &&
         (sStatement->isSimpleQuery() == ID_TRUE))
    {
        sStatement->setBindState(MMC_STMT_BIND_DATA);

        /* 이 정보는 SimpleQuery에서 사용되기 때문에, CMP_OP_DB_Execute나
         * CMP_OP_DB_ParamDataInList의 패킷에 같이 서버로 보내게 된다.
         * 그런데, Prepare를 응답을 받지 않고 바로 Execute나 ParamDataInList를
         * 보내는 경우에는 SimpleQuery인지를 알 수 없다.
         * 그래서, IPCDA인 경우에는 무조건 이 정보를 전송하도록 하였다.
         */
        sBindColInfo = (UShort *)(aProtocolContext->mReadBlock->mData +
                                  aProtocolContext->mReadBlock->mCursor);
        sBindColNum = *sBindColInfo;

        /***********************************************
         *  Packet's Constitute =
         *   column_count(short) +
         *   column_id(short) * column_count
         ***********************************************/
        /* move cursor of cmi's readblock.*/
        CMI_SKIP_READ_BLOCK(aProtocolContext, 2 + (sBindColNum * 2));

        /* 2. execute */
        IDE_TEST(sExecuteOption != CMP_DB_EXECUTE_NORMAL_EXECUTE);
        sStatement->setArray(ID_FALSE);
        sStatement->setRowNumber(sRowNumber);

        if( sThread->executeIPCDASimpleQuery(aProtocolContext,
                                             sStatement,
                                             sBindColInfo,
                                             ID_TRUE,
                                             &sSuspended,
                                             NULL,
                                             NULL,
                                             NULL,
                                             (UChar *)(aProtocolContext->mReadBlock->mData + aProtocolContext->mReadBlock->mCursor))
            != IDE_SUCCESS)
        {
            aProtocolContext->mReadBlock->mCursor += sRowSize;
            IDE_TEST( sThread->answerErrorResult(aProtocolContext,
                                                 aProtocol->mOpID,
                                                 sRowNumber,
                                                 sSession,
                                                 sExecuteOption)
                      != IDE_SUCCESS );
        }
        else
        {
            aProtocolContext->mReadBlock->mCursor += sRowSize;
        }
    }
    else
    {
        /* PROJ-2160 CM 타입제거
           모두 읽은 다음에 프로토콜을 처리해야 한다. */
        if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
        {
            /* 이 정보는 SimpleQuery에서 사용되기 때문에, CMP_OP_DB_Execute나
             * CMP_OP_DB_ParamDataInList의 패킷에 같이 서버로 보내게 된다.
             * 그런데, Prepare를 응답을 받지 않고 바로 Execute나 ParamDataInList를
             * 보내는 경우에는 SimpleQuery인지를 알 수 없다.
             * 그래서, IPCDA인 경우에는 무조건 이 정보를 전송하도록 하였다.
             */
            sBindColNum = *((UShort *)(aProtocolContext->mReadBlock->mData +
                                       aProtocolContext->mReadBlock->mCursor));
            /***********************************************
             *  Packet's Constitute =
             *   column_count(short) +
             *   column_id(short) * column_count
             ***********************************************/
            /* move cursor of cmi's readblock.*/
            CMI_SKIP_READ_BLOCK(aProtocolContext, 2 + (sBindColNum * 2));

            IDE_TEST_RAISE( cmiSplitReadIPCDA( aProtocolContext,
                                               sRowSize,
                                               &sRow,
                                               NULL)
                            != IDE_SUCCESS, cm_error );
        }
        else
        {
            IDE_TEST( sSession->allocChunk( sRowSize ) != IDE_SUCCESS );

            sRow        = sSession->getChunk();
            IDE_TEST_RAISE( cmiSplitRead( aProtocolContext,
                                          sRowSize,
                                          sRow,
                                          NULL )
                            != IDE_SUCCESS, cm_error );
        }
        sRowSize = 0;

        sParamCount = qci::getParameterCount(sQciStmt);

        // BUG-41452 Save totalRowNumber
        sStatement->setTotalRowNumber(sToRowNumber);

        for( sRowNumber = sFromRowNumber; sRowNumber <= sToRowNumber; ++sRowNumber )
        {
            if( qci::setBindParamDataSet(sQciStmt, sRow + sOffset, &sOffset, mmtCopyBindParamDataCallback)
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
                                             sSession,
                                             sStatementID );

                IDE_TEST( sThread->answerErrorResult(aProtocolContext,
                                                     aProtocol->mOpID,
                                                     sRowNumber,
                                                     sSession,
                                                     sExecuteOption)
                          != IDE_SUCCESS );

                continue;
            }

            // fix BUG-22058
            if ((idvProfile::getProfFlag() & IDV_PROF_TYPE_BIND_FLAG) == IDV_PROF_TYPE_BIND_FLAG)
            {
                // bug-25312: prepare 이후에 autocommit을 off에서 on으로 변경하고
                // bind 하면 stmt->mTrans 가 null이어서 segv.
                // 변경: stmt->mTrans를 구하여 null이면 TransID로 0을 넘기도로 수정
                mmcTransObj *sTrans = sSession->getTransPtr(sStatement);
                smTID sTransID = (sTrans != NULL) ? mmcTrans::getTransID(sTrans) : 0;

                for ( i = 0; i < sParamCount; i++)
                {
                    sBindParam.id = i;

                    IDE_TEST(qci::getBindParamInfo(sQciStmt, &sBindParam) != IDE_SUCCESS);

                    // fix BUG-24877 paramDataInListProtocol에서
                    // output param을 Profiling하려 합니다.
                    // 클라이언트로부터 INPUT, INPUT_OUTPUT의
                    // 데이터만 전송되므로 OUTPUT PARAM은 SKIP한다.
                    if ((sBindParam.inoutType == CMP_DB_PARAM_INPUT) ||
                        (sBindParam.inoutType == CMP_DB_PARAM_INPUT_OUTPUT))
                    {
                        // proj_2160 cm_type removal
                        // cmblock 데이터가 아닌 qp 저장 데이터를 사용.
                        // why? out-binding 데이터는 큰 패킷의 경우
                        // cmblock 데이터를 사용하기 힘들기 때문.
                        // altiProfile에서 매 파라미터마다 고정버퍼
                        // 위치에 적재하므로 align 문제는 없음.
                        sProfBind.mParamNo = i + 1;
                        sProfBind.mType = sBindParam.type;
                        sProfBind.mData = sBindParam.data;
                        idvProfile::writeBind(&sProfBind,
                                              sStatement->getSessionID(),
                                              sStatement->getStmtID(),
                                              sTransID,
                                              profWriteBindCallback );
                    }
                }
            }

            sStatement->setBindState(MMC_STMT_BIND_DATA);

            // 2. execute
            switch (sExecuteOption)
            {
                case CMP_DB_EXECUTE_NORMAL_EXECUTE:
                    sStatement->setArray(ID_FALSE);
                    sStatement->setRowNumber(sRowNumber);

                    if( sThread->execute(aProtocolContext,
                                         sStatement,
                                         ID_TRUE,
                                         &sSuspended,
                                         NULL, NULL, NULL) != IDE_SUCCESS)
                    {
                        IDE_TEST( sThread->answerErrorResult(aProtocolContext,
                                                             aProtocol->mOpID,
                                                             sRowNumber,
                                                             sSession,
                                                             sExecuteOption)
                                  != IDE_SUCCESS );
                    }

                    break;

                case CMP_DB_EXECUTE_ARRAY_EXECUTE:
                    sStatement->setArray(ID_TRUE);
                    sStatement->setRowNumber(sRowNumber);

                    if( sThread->execute( aProtocolContext,
                                          sStatement,
                                          ID_FALSE,
                                          &sSuspended,
                                          &sResultSetCount,
                                          &sAffectedRowCount,
                                          &sFetchedRowCount ) != IDE_SUCCESS )
                    {
                        FLUSH_BIND_DATA_LIST_RESULT( &sResult,
                                                     aProtocolContext,
                                                     sSession,
                                                     sStatementID );

                        IDE_TEST( sThread->answerErrorResult(aProtocolContext,
                                                             aProtocol->mOpID,
                                                             sRowNumber,
                                                             sSession,
                                                             sExecuteOption)
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* BUG-41437 A server occurs a fatal error while proc is executing */
                        IDE_TEST_RAISE( sResultSetCount > 0, InvalidProcedure );

                        if( sResult.mResultCount > 0 )
                        {
                            if ( (sResult.mResultSetCount != sResultSetCount) ||
                                 (sResult.mAffectedRowCount != sAffectedRowCount) )
                            {
                                FLUSH_BIND_DATA_LIST_RESULT(&sResult,
                                                            aProtocolContext,
                                                            sSession,
                                                            sStatementID);

                                sResult.mFromRowNumber    = sRowNumber;
                                sResult.mResultSetCount   = sResultSetCount;
                                sResult.mAffectedRowCount = sAffectedRowCount;
                                sResult.mFetchedRowCount  = sFetchedRowCount;
                                sResult.mResultCount      = 0;
                            }
                        }
                        else
                        {
                            sResult.mFromRowNumber    = sRowNumber;
                            sResult.mResultSetCount   = sResultSetCount;
                            sResult.mAffectedRowCount = sAffectedRowCount;
                            sResult.mFetchedRowCount  = sFetchedRowCount;
                            sResult.mResultCount      = 0;
                        }
                        sResult.mResultCount++;
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
                    sStatement->setRowNumber(sRowNumber);
                    // Rebuild Error 를 처리하기 위해서는 Bind가 끝난 시점에서
                    // atomicBegin 을 호출해야 한다.
                    if( sRowNumber == 1 )
                    {
                        IDE_TEST_RAISE((sThread->atomicBegin(sStatement) != IDE_SUCCESS), SkipExecute);
                    }

                    IDE_TEST_RAISE(sThread->atomicExecute(sStatement, aProtocolContext) != IDE_SUCCESS, SkipExecute);
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
                                     sSession,
                                     sStatementID );

        IDE_EXCEPTION_CONT(SkipExecute);
    }

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
    IDE_EXCEPTION(cm_error)
    {
        return IDE_FAILURE;
    }
    IDE_EXCEPTION_END;

    // proj_2160 cm_type removal
    // 처리도중 에러가 발생한 경우
    // 다음 프로토콜을 정상적으로 처리하기 위해
    // 남은 데이터를 skip 한다.
    // sRowSize가 0인 경우는, 프로토콜 데이터를 이미 다 읽은 경우임.
    // ex) prepare 프로토콜시 table not found 에러
    if( sRowSize > 0 )
    {
        IDE_TEST_RAISE( cmiSplitSkipRead( aProtocolContext,
                                          sRowSize,
                                          NULL )
                        != IDE_SUCCESS, cm_error );
    }

    return sThread->answerErrorResult(aProtocolContext,
                                      aProtocol->mOpID,
                                      0,
                                      sSession,
                                      sExecuteOption);
}

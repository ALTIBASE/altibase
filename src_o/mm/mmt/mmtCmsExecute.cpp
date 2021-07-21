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

#include <mmErrorCode.h>
#include <mmcConv.h>
#include <mmcSession.h>
#include <mmcStatement.h>
#include <mmcStatementManager.h>
#include <mmcTask.h>
#include <mmtServiceThread.h>
#include <mmqManager.h>
#include <mmdXa.h>
#include <mmtAuditManager.h>

typedef struct mmtCmsExecuteContext
{
    cmiProtocolContext *mProtocolContext;
    mmcStatement       *mStatement;
    SLong               mAffectedRowCount;
    SLong               mFetchedRowCount;
    idBool              mSuspended;
    UChar              *mCollectionData;
    UInt                mCursor; // bug-27621: pointer to UInt
} mmtCmsExecuteContext;

static IDE_RC answerExecuteResult(mmtCmsExecuteContext *aExecuteContext,
                                  mmcSession           *aSession)
{
    cmiProtocolContext    *sCtx   = aExecuteContext->mProtocolContext;
    mmcStatement          *sStmt  = aExecuteContext->mStatement;

    UInt               sStatementID      = aExecuteContext->mStatement->getStmtID();
    UInt               sRowNumber        = aExecuteContext->mStatement->getRowNumber();
    UShort             sResultSetCount   = aExecuteContext->mStatement->getResultSetCount();
    ULong              sAffectedRowCount = (ULong)aExecuteContext->mAffectedRowCount;
    ULong              sFetchedRowCount  = (ULong)aExecuteContext->mFetchedRowCount;
    cmiWriteCheckState sWriteCheckState  = CMI_WRITE_CHECK_DEACTIVATED;

    smSCN              sSCN;

    UShort             sSessionPropID       = CMP_DB_PROPERTY_MAX;
    SChar            * sSessionPropValueStr = NULL;
    UInt               sSessionPropValueLen = 0;

    idBool             sGetSessionPropertyInfo = ID_FALSE;

    SM_INIT_SCN(&sSCN);

    switch (sCtx->mProtocol.mOpID)
    {
        case CMP_OP_DB_ExecuteV3:
        case CMP_OP_DB_ParamDataInListV3:
            sGetSessionPropertyInfo = ID_TRUE;
            break;

        case CMP_OP_DB_ExecuteV2:
        case CMP_OP_DB_ParamDataInListV2:
            break;

        case CMP_OP_DB_Execute:
        case CMP_OP_DB_ParamDataInList:
            break;

        default:
            IDE_DASSERT(0);
            break;
    }
    
    if ( ( sGetSessionPropertyInfo == ID_TRUE ) &&
         ( sStmt->getStmtType() == QCI_STMT_SET_SESSION_PROPERTY ) )
    {
        // query 1 : 1 execute alter session의 값은 항상 1개
        aSession->getSessionPropertyInfo( &sSessionPropID,
                                          &sSessionPropValueStr,
                                          &sSessionPropValueLen );
    }

    if ( aSession->isGCTx() == ID_TRUE )
    {
        aSession->getLastSystemSCN( sCtx->mProtocol.mOpID, &sSCN );
    }

    #if defined(DEBUG)
    ideLog::log(IDE_SD_18, "= [%s] answerExecuteResult, SCN : %"ID_UINT64_FMT,
                aSession->getSessionTypeString(),
                sSCN);
    #endif
    
    switch (sCtx->mProtocol.mOpID)
    {
        case CMP_OP_DB_ExecuteV3:  /* PROJ-2733-Protocol */
        case CMP_OP_DB_ParamDataInListV3:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK_WITH_IPCDA(sCtx,
                                       41 + sSessionPropValueLen,
                                       41 + sSessionPropValueLen + 1);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(sCtx, CMP_OP_DB_ExecuteV3Result);
            CMI_WR4(sCtx, &sStatementID);
            CMI_WR4(sCtx, &sRowNumber);
            CMI_WR2(sCtx, &sResultSetCount);
            CMI_WR8(sCtx, &sAffectedRowCount);
            CMI_WR8(sCtx, &sFetchedRowCount);
            CMI_WR8(sCtx, &sSCN);  /* PROJ-2733-DistTxInfo */
            // PROJ-2727
            CMI_WR2(sCtx, &sSessionPropID);
            CMI_WR4(sCtx, &sSessionPropValueLen);
            CMI_WCP(sCtx, sSessionPropValueStr, sSessionPropValueLen);
            break;

        case CMP_OP_DB_ExecuteV2:  /* BUG-44572 */
        case CMP_OP_DB_ParamDataInListV2:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK_WITH_IPCDA(sCtx, 27, 27 + 1);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(sCtx, CMP_OP_DB_ExecuteV2Result);
            CMI_WR4(sCtx, &sStatementID);
            CMI_WR4(sCtx, &sRowNumber);
            CMI_WR2(sCtx, &sResultSetCount);
            CMI_WR8(sCtx, &sAffectedRowCount);
            CMI_WR8(sCtx, &sFetchedRowCount);
            break;

        case CMP_OP_DB_Execute:
        case CMP_OP_DB_ParamDataInList:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
            CMI_WRITE_CHECK_WITH_IPCDA(sCtx, 19, 19 + 1);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(sCtx, CMP_OP_DB_ExecuteResult);
            CMI_WR4(sCtx, &sStatementID);
            CMI_WR4(sCtx, &sRowNumber);
            CMI_WR2(sCtx, &sResultSetCount);
            CMI_WR8(sCtx, &sAffectedRowCount);
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    /* PROJ-2616 */
    if (cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA)
    {
        CMI_WR1(sCtx, sStmt->isSimpleQuerySelectExecuted() == ID_TRUE ? 1 : 0);
    }

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(sCtx);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}

static IDE_RC answerParamDataOutList(mmtCmsExecuteContext *aExecuteContext,
                                     UShort                aParamCount)
{
    cmiProtocolContext   *sCtx = aExecuteContext->mProtocolContext;
    mmcStatement         *sStatement       = aExecuteContext->mStatement;
    UChar                *sData            = aExecuteContext->mCollectionData;
    mmcSession           *sSession         = sStatement->getSession();
    UInt                  sStatementID     = sStatement->getStmtID();
    UInt                  sRowNumber       = sStatement->getRowNumber();

    UShort                sParamIndex = 0;
    UInt                  sInOutType = 0;
    UInt                  sType = 0;

    UInt                  sColumnSize = 0;
    UInt                  sLen32 = 0;
    UShort                sLen16 = 0;
    UChar                 sLen8;
    UChar                 sTemp;

    mmcTransObj          *sTrans = NULL;
    smTID                 sTransID;
    idvProfBind           sProfBind;
    cmiWriteCheckState    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
    UShort                sOrgWriteCursor    = CMI_GET_CURSOR(sCtx);

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(sCtx, 13);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(sCtx, CMP_OP_DB_ParamDataOutList);
    CMI_WR4(sCtx, &sStatementID);
    CMI_WR4(sCtx, &sRowNumber);
    // 파라미터데이터의 정확한 총 크기가 mCursor에 저장되어 있다
    // cf) qci::getBindParamData()
    CMI_WR4(sCtx, &(aExecuteContext->mCursor));

    if ((idvProfile::getProfFlag() & IDV_PROF_TYPE_BIND_FLAG) == IDV_PROF_TYPE_BIND_FLAG)
    {
        sTrans = sSession->getTransPtr(sStatement);
        sTransID = (sTrans != NULL) ? mmcTrans::getTransID(sTrans) : 0;
    }

    /* PROJ-2160 CM 타입제거
       sData는 Align 이 안된 데이타 이다. */
    for( sParamIndex = 0; sParamIndex < aParamCount; sParamIndex++ )
    {
        sInOutType = qci::getBindParamInOutType(sStatement->getQciStmt(), sParamIndex);
        sType      = qci::getBindParamType(sStatement->getQciStmt(), sParamIndex);

        if( sInOutType != CMP_DB_PARAM_INPUT )
        {
            /* sColumnSize 값을 제대로 유지해야 한다. */
            switch(sType)
            {
                case MTD_NULL_ID :

                    sColumnSize = ID_SIZEOF(UChar);
                    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                    CMI_WRITE_CHECK(sCtx, ID_SIZEOF(UChar));
                    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

                    CMI_WR1(sCtx, *(sData));
                    break;
                case MTD_BINARY_ID :

                    ID_4_BYTE_ASSIGN( &sLen32, sData );
                    sColumnSize = sLen32 + ID_SIZEOF(SDouble);

                    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                    /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
                    CMI_WRITE_CHECK_WITH_IPCDA( sCtx, ID_SIZEOF(SDouble), ID_SIZEOF(SDouble) + sLen32);
                    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

                    CMI_WR4(sCtx, &sLen32);
                    CMI_WR4(sCtx, &sLen32);

                    IDE_TEST( cmiSplitWrite( sCtx, sLen32, sData + ID_SIZEOF(SDouble))
                              != IDE_SUCCESS );
                    break;

                // mtdCharType, mtdNcharType, mtdByteType은 구조가 모두 동일하다
                case MTD_CHAR_ID    :
                case MTD_VARCHAR_ID :
                case MTD_NCHAR_ID   :
                case MTD_NVARCHAR_ID:
                case MTD_BYTE_ID    :
                case MTD_VARBYTE_ID :

                    ID_2_BYTE_ASSIGN( &sLen16, sData );
                    sColumnSize = sLen16 + ID_SIZEOF(UShort);

                    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                    /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
                    CMI_WRITE_CHECK_WITH_IPCDA( sCtx, ID_SIZEOF(UShort), ID_SIZEOF(UShort) + sLen16);
                    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

                    CMI_WR2(sCtx, &sLen16);

                    IDE_TEST( cmiSplitWrite( sCtx, sLen16, sData + ID_SIZEOF(UShort))
                              != IDE_SUCCESS );
                    break;

                case MTD_FLOAT_ID :
                case MTD_NUMERIC_ID :
                case MTD_NUMBER_ID :

                    ID_1_BYTE_ASSIGN( &sLen8, sData );
                    sColumnSize = sLen8 + ID_SIZEOF(UChar);
                    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                    CMI_WRITE_CHECK(sCtx, sColumnSize);
                    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                    CMI_WR1(sCtx, sLen8);

                    if( sLen8 != 0 )
                    {
                        ID_1_BYTE_ASSIGN( &sTemp, sData + ID_SIZEOF(UChar) );

                        CMI_WR1(sCtx, sTemp);
                        CMI_WCP(sCtx, sData + 2, sLen8 -ID_SIZEOF(UChar));
                    }
                    break;

                case MTD_SMALLINT_ID:
                    sColumnSize = ID_SIZEOF(mtdSmallintType);
                    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                    CMI_WRITE_CHECK(sCtx, ID_SIZEOF(mtdSmallintType));
                    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                    CMI_WR2(sCtx, (UShort*)sData);
                    break;

                case MTD_INTEGER_ID:
                    sColumnSize = ID_SIZEOF(mtdIntegerType);
                    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                    CMI_WRITE_CHECK(sCtx, ID_SIZEOF(mtdIntegerType));
                    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                    CMI_WR4(sCtx, (UInt*)sData);
                    break;

                case MTD_BIGINT_ID:
                    sColumnSize = ID_SIZEOF(mtdBigintType);
                    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                    CMI_WRITE_CHECK(sCtx, ID_SIZEOF(mtdBigintType));
                    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                    CMI_WR8(sCtx, (ULong*)sData);
                    break;

                case MTD_REAL_ID :
                    sColumnSize = ID_SIZEOF(mtdRealType);
                    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                    CMI_WRITE_CHECK(sCtx, ID_SIZEOF(mtdRealType));
                    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                    CMI_WR4(sCtx, (UInt*)sData);
                    break;

                case MTD_DOUBLE_ID :
                    sColumnSize = ID_SIZEOF(mtdDoubleType);
                    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                    CMI_WRITE_CHECK(sCtx, ID_SIZEOF(mtdDoubleType));
                    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                    CMI_WR8(sCtx, (ULong*)sData);
                    break;

                case MTD_BLOB_LOCATOR_ID :
                case MTD_CLOB_LOCATOR_ID :
                    /* PROJ-2047 Strengthening LOB - LOBCACHE */
                    sColumnSize = ID_SIZEOF(smLobLocator) +
                                  ID_SIZEOF(ULong) +
                                  ID_SIZEOF(UChar);

                    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                    CMI_WRITE_CHECK(sCtx, sColumnSize);
                    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                    CMI_WR8(sCtx, (ULong *)sData);

                    sLen32 = ID_SIZEOF(smLobLocator);
                    CMI_WR8(sCtx, (ULong *)(sData + sLen32));

                    sLen32 += ID_SIZEOF(ULong);
                    CMI_WR1(sCtx, *(sData + sLen32));
                    break;

                case MTD_DATE_ID:
                    sColumnSize = ID_SIZEOF(mtdDateType);
                    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                    CMI_WRITE_CHECK(sCtx, sColumnSize);
                    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                    CMI_WR2(sCtx, (UShort*)sData);
                    CMI_WR2(sCtx, (UShort*)(sData + 2));
                    CMI_WR4(sCtx, (UInt*)(sData + 4));
                    break;

                case MTD_INTERVAL_ID:
                    sColumnSize = ID_SIZEOF(mtdIntervalType);
                    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                    CMI_WRITE_CHECK(sCtx, sColumnSize);
                    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
                    CMI_WR8(sCtx, (ULong*)sData);
                    CMI_WR8(sCtx, (ULong*)(sData + 8));
                    break;

                case MTD_BIT_ID :
                case MTD_VARBIT_ID :

                    ID_4_BYTE_ASSIGN( &sLen32, sData );
                    sColumnSize = BIT_TO_BYTE(sLen32) + ID_SIZEOF(UInt);

                    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
                    /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
                    CMI_WRITE_CHECK_WITH_IPCDA( sCtx, ID_SIZEOF(UInt), sColumnSize);
                    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

                    CMI_WR4(sCtx, &sLen32);

                    IDE_TEST( cmiSplitWrite( sCtx,
                                             BIT_TO_BYTE(sLen32),
                                             sData + ID_SIZEOF(UInt))
                              != IDE_SUCCESS );
                    break;

                case MTD_NIBBLE_ID :

                    ID_1_BYTE_ASSIGN( &sLen8, sData );

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
                    CMI_WCP(sCtx, sData + ID_SIZEOF(UChar), sColumnSize - ID_SIZEOF(UChar));
                    break;

                default :
                sColumnSize = 0;
                IDE_DASSERT(0);
                    break;
            } // end switch

            // fix BUG-24881 Output Parameter도 Profiling 해야
            if ((idvProfile::getProfFlag() & IDV_PROF_TYPE_BIND_FLAG) ==
                IDV_PROF_TYPE_BIND_FLAG)
            {
                sProfBind.mParamNo = sParamIndex + 1;
                sProfBind.mType = sType;
                sProfBind.mData = sData;
                idvProfile::writeBind(&sProfBind,
                                      sStatement->getSessionID(),
                                      sStatementID,
                                      sTransID,
                                      profWriteBindCallback );
            }
            sData += sColumnSize;
        }
    }

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(sCtx);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log(IDE_SERVER_1,
                "answerParamDataOutList: marshal error ["
                "total size: %u "
                "param no: %d "
                "type: %u]",
                aExecuteContext->mCursor, sParamIndex, sType);

    if(cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA)
    {
        /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
        if( sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED)
        {
            IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
        }
    }
    else
    {
        // 마샬링 에러가 발생한 경우 세션을 끊는 방법밖에 없다.
        // cf) 첫번째 패킷이 이미 전송된 후에 두번째 패킷에서
        // 마샬링 에러가 발생하면 ErrorResult 조차도 보내면 안된다.
        // client에서는 특정 프로토콜이 연이어 올것이라 믿기 때문에.
        sCtx->mSessionCloseNeeded = ID_TRUE;
    }
    
    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return IDE_FAILURE;
}

/*******************************************************************
 PROJ-2160 CM 타입제거
 Description : 엔디안 변환없이 데이타를 임시버퍼로 복사한다.
               실제 전송은 answerParamDataOutList 함수에서 한다.
********************************************************************/
static IDE_RC getBindParamListCallback(idvSQL       *aStatistics,
                                       qciBindParam *aBindParam,
                                       void         *aData,
                                       void         *aExecuteContext)
{
    mmtCmsExecuteContext *sExecuteContext  = (mmtCmsExecuteContext *)aExecuteContext;
    UChar                *sData            = (UChar*)aData;
    UChar                *sDest            = sExecuteContext->mCollectionData + sExecuteContext->mCursor;
    UInt                  sColumnSize      = 0;
    UInt                  sLobSize         = 0;

    /* PROJ-2047 Strengthening LOB - LOBCACHE */ 
    ULong                 sLobSize64       = 0;

    switch(aBindParam->type)
    {
        case MTD_NULL_ID :
            sColumnSize = 1;

            idlOS::memcpy(sDest, sData, sColumnSize);
            break;
        case MTD_BINARY_ID :
            /* BUG-45962 */
            sColumnSize = ((mtdBinaryType*) sData)->mLength + ID_SIZEOF(ULong);

            idlOS::memcpy(sDest, sData, sColumnSize);
            break;

        case MTD_CHAR_ID :
        case MTD_VARCHAR_ID :
            sColumnSize = ((mtdCharType*)sData)->length +ID_SIZEOF(UShort);

            idlOS::memcpy(sDest, sData, sColumnSize);
            break;

        case MTD_NCHAR_ID :
        case MTD_NVARCHAR_ID :
            sColumnSize = ((mtdNcharType*)sData)->length +ID_SIZEOF(UShort);

            idlOS::memcpy(sDest, sData, sColumnSize);
            break;

        case MTD_FLOAT_ID :
        case MTD_NUMERIC_ID :
        case MTD_NUMBER_ID :
            sColumnSize = ((mtdNumericType*) sData)->length + ID_SIZEOF(UChar);

            idlOS::memcpy(sDest, sData, sColumnSize);
            break;

        case MTD_SMALLINT_ID:
            sColumnSize = ID_SIZEOF(mtdSmallintType);

            idlOS::memcpy(sDest, sData, sColumnSize);
            break;

        case MTD_INTEGER_ID:
            sColumnSize = ID_SIZEOF(mtdIntegerType);

            idlOS::memcpy(sDest, sData, sColumnSize);
            break;

        case MTD_BIGINT_ID:
            sColumnSize = ID_SIZEOF(mtdBigintType);

            idlOS::memcpy(sDest, sData, sColumnSize);
            break;

        case MTD_REAL_ID :
            sColumnSize = ID_SIZEOF(mtdRealType);

            idlOS::memcpy(sDest, sData, sColumnSize);
            break;

        case MTD_DOUBLE_ID :
            sColumnSize = ID_SIZEOF(mtdDoubleType);

            idlOS::memcpy(sDest, sData, sColumnSize);
            break;

        case MTD_BLOB_LOCATOR_ID :
        case MTD_CLOB_LOCATOR_ID :
            sColumnSize = ID_SIZEOF(smLobLocator);
            idlOS::memcpy(sDest, sData, sColumnSize);

            /* PROJ-2047 Strengthening LOB - LOBCACHE */
            (void)qciMisc::lobGetLength( aStatistics, *((smLobLocator*)sData), &sLobSize );
            sLobSize64 = sLobSize;
            idlOS::memcpy(sDest + sColumnSize, &sLobSize64, ID_SIZEOF(ULong));
            sColumnSize += ID_SIZEOF(ULong);

            *(sDest + sColumnSize) = ID_FALSE;
            sColumnSize += ID_SIZEOF(UChar);
            break;

        case MTD_DATE_ID:
            sColumnSize = ID_SIZEOF(mtdDateType);

            idlOS::memcpy(sDest, sData, sColumnSize);
            break;

        case MTD_INTERVAL_ID:
            sColumnSize = ID_SIZEOF(mtdIntervalType);

            idlOS::memcpy(sDest, sData, sColumnSize);
            break;

        case MTD_BIT_ID :
        case MTD_VARBIT_ID :
            sColumnSize = BIT_TO_BYTE(((mtdBitType*)sData)->length) + ID_SIZEOF(UInt);

            idlOS::memcpy(sDest, sData, sColumnSize);
            break;

        case MTD_NIBBLE_ID :

            if( (((mtdNibbleType*)sData)->length) != MTD_NIBBLE_NULL_LENGTH )
            {
                sColumnSize = (((((mtdNibbleType*)sData)->length) + 1) >> 1) + ID_SIZEOF(UChar);
            }
            else
            {
                sColumnSize = ID_SIZEOF(UChar);
            }
            idlOS::memcpy(sDest, sData, sColumnSize);
            break;

        case MTD_BYTE_ID :
        case MTD_VARBYTE_ID :
            sColumnSize = ((mtdByteType*) sData)->length + ID_SIZEOF(UShort);

            idlOS::memcpy(sDest, sData, sColumnSize);
            break;

        default :
        IDE_DASSERT(0);
    }

    sExecuteContext->mCursor += sColumnSize;

    return IDE_SUCCESS;
}

static IDE_RC sendBindOut(mmcSession           *aSession,
                          mmtCmsExecuteContext *aExecuteContext)
{
    qciStatement *sQciStmt;
    UInt          sInOutType;
    UShort        sParamCount;
    UInt          sOutParamSize;
    UInt          sOutParamCount;
    UShort        sParamIndex;
    UInt          sAllocSize;
    qciStmtType   sStmtType;


    sQciStmt    = aExecuteContext->mStatement->getQciStmt();
    sParamCount = qci::getParameterCount(sQciStmt);

    IDE_TEST( qci::getOutBindParamSize( sQciStmt,
                                        &sOutParamSize,
                                        &sOutParamCount )
              != IDE_SUCCESS );

    IDE_TEST_CONT( sOutParamCount == 0, SKIP_SEND_BIND_OUT );

    /* BUG-48877 */
    if ( ( aSession->isShardCoordinatorSession() == ID_TRUE ) ||
         ( aSession->isShardLibrarySession() == ID_TRUE ) )
    {
        sStmtType = aExecuteContext->mStatement->getStmtType();

        switch (sStmtType)
        {
            case QCI_STMT_INSERT:
            case QCI_STMT_UPDATE:
            case QCI_STMT_DELETE:
                IDE_TEST_CONT( aExecuteContext->mAffectedRowCount == 0,
                               SKIP_SEND_BIND_OUT );
                break;
            default:
                break;
        }
    }
    else
    {
        /* Nothing to do */
    }

    // lob insert인 경우 precision이 -4로 설정되어 있어
    // size가 0으로 계산된다.
    sAllocSize = IDL_MAX(sOutParamSize, MMC_DEFAULT_COLLECTION_BUFFER_SIZE);
    IDE_TEST( aSession->allocOutChunk(sAllocSize) != IDE_SUCCESS );

    aExecuteContext->mCollectionData   = aSession->getOutChunk();
    aExecuteContext->mCursor = 0;

    for( sParamIndex = 0; sParamIndex < sParamCount; sParamIndex++ )
    {
        sInOutType = qci::getBindParamInOutType(sQciStmt, sParamIndex);

        if ((sInOutType == CMP_DB_PARAM_INPUT_OUTPUT) ||
            (sInOutType == CMP_DB_PARAM_OUTPUT))
        {
            IDE_TEST(qci::getBindParamData(sQciStmt,
                                           sParamIndex,
                                           getBindParamListCallback,
                                           aExecuteContext)
                        != IDE_SUCCESS);
        }
    }

    IDE_TEST( answerParamDataOutList(aExecuteContext,
                                     sParamCount) != IDE_SUCCESS);

    IDE_EXCEPTION_CONT( SKIP_SEND_BIND_OUT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC doRetry(mmcStatement *aStmt)
{
    IDE_TEST(qci::retry(aStmt->getQciStmt(), aStmt->getSmiStmt()) != IDE_SUCCESS);

    IDE_TEST(aStmt->endStmt(MMC_EXECUTION_FLAG_RETRY) != IDE_SUCCESS);

    IDE_TEST(aStmt->beginStmt() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC doRebuild(mmcStatement *aStmt)
{
    IDE_TEST(qci::closeCursor(aStmt->getQciStmt(), aStmt->getSmiStmt()) != IDE_SUCCESS);

    IDE_TEST(aStmt->endStmt(MMC_EXECUTION_FLAG_REBUILD) != IDE_SUCCESS);

    // BUG_12177
    aStmt->setCursorFlag(SMI_STATEMENT_ALL_CURSOR);

#if defined(DEBUG)
    if ( aStmt->getSession()->isShardUserSession() == ID_FALSE )
    {
        IDE_DASSERT( aStmt->getSession()->detectShardMetaChange() == ID_FALSE );
    }
#endif

    IDE_TEST(aStmt->beginStmt() != IDE_SUCCESS);

    IDE_TEST(aStmt->rebuild() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// fix BUG-30891
static IDE_RC doEnd(mmtCmsExecuteContext *aExecuteContext, mmcExecutionFlag aExecutionFlag)
{
    mmcStatement *sStmt = aExecuteContext->mStatement;

    /* PROJ-2223 Altibase Auditing */
    mmtAuditManager::auditByAccess( sStmt, aExecutionFlag );
    
    // fix BUG-30990
    // QUEUE가 EMPTY일 경우에만 BIND_DATA 상태로 설정한다.
    switch(aExecutionFlag)
    {
        case MMC_EXECUTION_FLAG_SUCCESS :
            IDE_TEST(sStmt->clearStmt(MMC_STMT_BIND_NONE) != IDE_SUCCESS);
            break;
        case MMC_EXECUTION_FLAG_QUEUE_EMPTY :
            IDE_TEST(sStmt->clearStmt(MMC_STMT_BIND_DATA) != IDE_SUCCESS);
            break;
        default:
            IDE_TEST(sStmt->clearStmt(MMC_STMT_BIND_INFO) != IDE_SUCCESS);
            break;
    }

    IDE_TEST(sStmt->endStmt(aExecutionFlag) != IDE_SUCCESS);

    sStmt->setExecuteFlag(ID_FALSE);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC doExecute(mmtCmsExecuteContext *aExecuteContext)
{
    mmcStatement   * sStatement = aExecuteContext->mStatement;
    mmcSession     * sSession   = sStatement->getSession();
    qciStatement   * sQciStmt   = sStatement->getQciStmt();
    smiStatement   * sSmiStmt   = NULL; 
    idBool           sRetry;
    idBool           sRecordExist;
    UShort           sResultSetCount;
    mmcResultSet   * sResultSet = NULL;
    mmcExecutionFlag sExecutionFlag = MMC_EXECUTION_FLAG_FAILURE;
    idBool           sBeginFetchTime = ID_FALSE;
    UInt             sOldResultSetHWM = 0;
    UInt             sStmtRetryMax = sSession->getShardStatementRetry();
    UInt             sRebuildRetryMax = SDI_SHARD_RETRY_LOOP_MAX;

    /* PROJ-2177 User Interface - Cancel */
    sStatement->setExecuting(ID_TRUE);

    // PROJ-2163
    IDE_TEST(sStatement->reprepare() != IDE_SUCCESS);

    IDE_TEST(sStatement->beginStmt() != IDE_SUCCESS);

    IDE_TEST( sStatement->shardRetryRebuild() != IDE_SUCCESS );

    // PROJ-2446 BUG-40481
    // mmcStatement.mSmiStmtPtr is changed at mmcStatement::beginSP()
    sSmiStmt = sStatement->getSmiStmt();

    do
    {
        do
        {
            sRetry = ID_FALSE;

            /* PROJ-2201 */
            sSession->setExecutingStatement(sStatement);
            if (sStatement->execute(&aExecuteContext->mAffectedRowCount,
                                    &aExecuteContext->mFetchedRowCount) != IDE_SUCCESS)
            {
                IDE_TEST_RAISE( sSession->processShardRetryError( sStatement,
                                                                  &sStmtRetryMax,
                                                                  &sRebuildRetryMax )
                                != IDE_SUCCESS,
                                ExecuteFailAbort );

                switch (ideGetErrorCode() & E_ACTION_MASK)
                {
                    case E_ACTION_IGNORE:
                        IDE_RAISE(ExecuteFailIgnore);
                        break;

                    case E_ACTION_RETRY:
                        IDE_TEST(doRetry(sStatement) != IDE_SUCCESS);

                        sRetry = ID_TRUE;
                        break;

                    case E_ACTION_REBUILD:
                        IDE_TEST(doRebuild(sStatement) != IDE_SUCCESS);

                        sRetry = ID_TRUE;
                        break;

                    case E_ACTION_ABORT:
                        IDE_RAISE(ExecuteFailAbort);
                        break;

                    case E_ACTION_FATAL:
                        IDE_CALLBACK_FATAL("fatal error returned from mmcStatement::execute()");
                        break;

                    default:
                        IDE_CALLBACK_FATAL("invalid error action returned from mmcStatement::execute()");
                        break;
                }
            }
            /* PROJ-2201 */
            sSession->setExecutingStatement(NULL);
        } while (sRetry == ID_TRUE);

        sStatement->setStmtState(MMC_STMT_STATE_EXECUTED);

        IDE_TEST( sendBindOut( sSession,
                               aExecuteContext )
                  != IDE_SUCCESS );

        // fix BUG-30913
        // execute의 성공 시점은 execute 및 sendBindOut 이후이다.
        sExecutionFlag = MMC_EXECUTION_FLAG_SUCCESS;

        /* 
         * BUG-40611 If realloc of initializeResultSet fails, signal 11 occurs.
         */
        sOldResultSetHWM = sStatement->getResultSetHWM();
        sResultSetCount = qci::getResultSetCount(sStatement->getQciStmt());
        sStatement->setResultSetCount(sResultSetCount);
        sStatement->setEnableResultSetCount(sResultSetCount);
        //fix BUG-27198 Code-Sonar  return value ignore하여, 결국 mResultSet가
        // null pointer일때 이를 de-reference를 할수 있습니다.
        IDE_TEST_RAISE(sStatement->initializeResultSet(sResultSetCount) != IDE_SUCCESS,
                       RestoreResultSetValues);


        if (sResultSetCount > 0)
        {
            sStatement->setFetchFlag(MMC_FETCH_FLAG_PROCEED);

            sResultSet = sStatement->getResultSet( MMC_RESULTSET_FIRST );
            // bug-26977: codesonar: resultset null ref
            // null이 가능한지는 모르겠지만, 방어코드임.
            IDE_TEST(sResultSet == NULL);

            if( sResultSet->mInterResultSet == ID_TRUE )
            {
                sRecordExist = sResultSet->mRecordExist;

                if (sRecordExist == ID_TRUE)
                {
                    // fix BUG-31195
                    sStatement->setFetchStartTime(mmtSessionManager::getBaseTime());
                    /* BUG-19456 */
                    sStatement->setFetchEndTime(0);

                    IDV_SQL_OPTIME_BEGIN( sStatement->getStatistics(),
                                          IDV_OPTM_INDEX_QUERY_FETCH);

                    sBeginFetchTime = ID_TRUE;

                    IDE_TEST(sStatement->beginFetch(MMC_RESULTSET_FIRST) != IDE_SUCCESS);
                }
                else
                {
                    sStatement->setFetchFlag(MMC_FETCH_FLAG_CLOSE);
                }
            }
            else
            {
                // fix BUG-31195
                // qci::moveNextRecord() 시간을 Fetch Time에 추가
                sStatement->setFetchStartTime(mmtSessionManager::getBaseTime());
                /* BUG-19456 */
                sStatement->setFetchEndTime(0);

                IDV_SQL_OPTIME_BEGIN( sStatement->getStatistics(),
                                      IDV_OPTM_INDEX_QUERY_FETCH);

                sBeginFetchTime = ID_TRUE;

                sSession->setExecutingStatement(sStatement); /* BUG-45473 */
                if (qci::moveNextRecord(sQciStmt,
                                        sStatement->getSmiStmt(),
                                        &sRecordExist) != IDE_SUCCESS)
                {
                    // fix BUG-31195
                    sBeginFetchTime = ID_FALSE;

                    sStatement->setFetchStartTime(0);
                    /* BUG-19456 */
                    sStatement->setFetchEndTime(0);

                    IDV_SQL_OPTIME_END( sStatement->getStatistics(),
                                        IDV_OPTM_INDEX_QUERY_FETCH );

                    IDE_TEST_RAISE( sSession->processShardRetryError( sStatement,
                                                                      &sStmtRetryMax,
                                                                      &sRebuildRetryMax )
                                    != IDE_SUCCESS,
                                    MoveNextRecordFailAbort );

                    switch (ideGetErrorCode() & E_ACTION_MASK)
                    {
                        case E_ACTION_IGNORE:
                            IDE_RAISE(MoveNextRecordFailIgnore);
                            break;

                        case E_ACTION_RETRY:
                            IDE_TEST(doRetry(sStatement) != IDE_SUCCESS);

                            sRetry = ID_TRUE;
                            break;

                        case E_ACTION_REBUILD:
                            IDE_TEST(doRebuild(sStatement) != IDE_SUCCESS);

                            sRetry = ID_TRUE;
                            break;

                        case E_ACTION_ABORT:
                            IDE_RAISE(MoveNextRecordFailAbort);
                            break;

                        case E_ACTION_FATAL:
                            IDE_CALLBACK_FATAL("fatal error returned from qci::moveNextRecord()");
                            break;

                        default:
                            IDE_CALLBACK_FATAL("invalid error action returned from qci::moveNextRecord()");
                            break;
                    }

                    sSession->setExecutingStatement(NULL); /* BUG-45473 */
                }
                else
                {
                    sSession->setExecutingStatement(NULL); /* BUG-45473 */

                    if (sStatement->getStmtType() == QCI_STMT_DEQUEUE)
                    {
                        if (sRecordExist == ID_TRUE)
                        {
                            sSession->setExecutingStatement(NULL);
                            sSession->endQueueWait();

                            sStatement->beginFetch(MMC_RESULTSET_FIRST);
                        }
                        else
                        {
                            // fix BUG-31195
                            sBeginFetchTime = ID_FALSE;

                            sStatement->setFetchStartTime(0);
                            /* BUG-19456 */
                            sStatement->setFetchEndTime(0);

                            IDV_SQL_OPTIME_END( sStatement->getStatistics(),
                                                IDV_OPTM_INDEX_QUERY_FETCH );

                            if (sSession->isQueueTimedOut() != ID_TRUE)
                            {
                                sExecutionFlag = MMC_EXECUTION_FLAG_QUEUE_EMPTY;
                                //fix BUG-21361 queue drop의 동시성 문제로 서버 비정상 종료할수 있음.
                                sSession->beginQueueWait();
                                //fix BUG-19321
                                IDE_TEST(doEnd(aExecuteContext, sExecutionFlag) != IDE_SUCCESS);
                                sSession->setExecutingStatement(sStatement);
                                aExecuteContext->mSuspended = ID_TRUE;
                            }
                            else
                            {
                                sSession->setExecutingStatement(NULL);
                                sSession->endQueueWait();

                                IDE_TEST(doEnd(aExecuteContext, sExecutionFlag) != IDE_SUCCESS);
                            }
                        }
                    }
                    else
                    {
                        if (sRecordExist == ID_TRUE)
                        {
                            IDE_TEST(sStatement->beginFetch(MMC_RESULTSET_FIRST) != IDE_SUCCESS);

                            // PROJ-2331
                            sResultSet->mBaseRow.mIsFirstRow = ID_TRUE;
                        }
                        else
                        {
                            // fix BUG-31195
                            sBeginFetchTime = ID_FALSE;

                            sStatement->setFetchStartTime(0);
                            /* BUG-19456 */
                            sStatement->setFetchEndTime(0);

                            IDV_SQL_OPTIME_END( sStatement->getStatistics(),
                                                IDV_OPTM_INDEX_QUERY_FETCH );

                            if (sResultSetCount == 1)
                            {
                                sStatement->setFetchFlag(MMC_FETCH_FLAG_CLOSE);

                                mmcStatement::makePlanTreeBeforeCloseCursor( sStatement,

                                                                             sStatement );

                                IDE_TEST(doEnd(aExecuteContext, sExecutionFlag) != IDE_SUCCESS);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            mmcStatement::makePlanTreeBeforeCloseCursor( sStatement,
                                                         sStatement );

            if (qci::closeCursor(sQciStmt, sSmiStmt) != IDE_SUCCESS)
            {
                IDE_TEST(ideIsRetry() != IDE_SUCCESS);

                IDE_TEST(doRetry(sStatement) != IDE_SUCCESS);

                sRetry = ID_TRUE;
            }
            else
            {
                IDE_TEST(doEnd(aExecuteContext, sExecutionFlag) != IDE_SUCCESS);
            }
        }

    } while (sRetry == ID_TRUE);

    if ( sStatement->getExecutingTrans() != NULL )
    {
        sStatement->releaseShareTransSmiStmtLock( sStatement->getExecutingTrans() );
    }

    /* PROJ-2177 User Interface - Cancel */
    sStatement->setExecuting(ID_FALSE);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ExecuteFailIgnore);
    {
        SChar sMsg[MAX_ERROR_MSG_LEN + 256 + 64];

        idlOS::snprintf(sMsg, ID_SIZEOF(sMsg), "mmcStatement::execute code=0x%x %s", ideGetErrorCode(), ideGetErrorMsg());

        IDE_SET(ideSetErrorCode(mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG, sMsg));
    }
    IDE_EXCEPTION(ExecuteFailAbort);
    {
        // No Action
    }
    IDE_EXCEPTION(MoveNextRecordFailIgnore);
    {
        SChar sMsg[64];

        idlOS::snprintf(sMsg, ID_SIZEOF(sMsg), "qci::moveNextRecord code=0x%x", ideGetErrorCode());

        IDE_SET(ideSetErrorCode(mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG, sMsg));
    }
    IDE_EXCEPTION(MoveNextRecordFailAbort);
    {
        mmcStatement::makePlanTreeBeforeCloseCursor( sStatement,
                                                     sStatement );

        qci::closeCursor(sQciStmt, sSmiStmt);
    }

    IDE_EXCEPTION( RestoreResultSetValues );
    {
        sStatement->setResultSetCount(sOldResultSetHWM);
        sStatement->setEnableResultSetCount(sOldResultSetHWM);
    }

    IDE_EXCEPTION_END;
    {
        IDE_PUSH();

        /* PROJ-2177 User Interface - Cancel */
        sStatement->setExecuting(ID_FALSE);

        // fix BUG-31195
        if (sBeginFetchTime == ID_TRUE)
        {
            sStatement->setFetchStartTime(0);
            /* BUG-19456 */
            sStatement->setFetchEndTime(0);

            IDV_SQL_OPTIME_END( sStatement->getStatistics(),
                                IDV_OPTM_INDEX_QUERY_FETCH );

            sBeginFetchTime = ID_FALSE;
        }

        sSession->setExecutingStatement(NULL);
        sSession->endQueueWait();

        if (sStatement->isStmtBegin() == ID_TRUE)
        {
            /* PROJ-2337 Homogeneous database link
             *  Dequeue 후에 예외 처리를 수행하는 경우, Queue에서 얻었던 데이터를 반납해야 한다.
             *  TODO Dequeue 외에 다른 문제는 없는지 확인해야 한다.
             */
            if ( ( sStatement->getStmtType() == QCI_STMT_DEQUEUE ) &&
                 ( sExecutionFlag == MMC_EXECUTION_FLAG_SUCCESS ) )
            {
                sExecutionFlag = MMC_EXECUTION_FLAG_FAILURE;
            }
            else
            {
                /* Nothing to do */
            }

            /* PROJ-2223 Altibase Auditing */
            mmtAuditManager::auditByAccess( sStatement, sExecutionFlag );
            
            /*
             * [BUG-24187] Rollback될 statement는 Internal CloseCurosr를
             * 수행할 필요가 없습니다.
             */
            sStatement->setSkipCursorClose();
            sStatement->clearStmt(MMC_STMT_BIND_NONE);

            /* BUG-47650 BUG-38585 IDE_ASSERT remove */
            (void)sStatement->endStmt( sExecutionFlag );
        }
        else
        {
            /* PROJ-2733 Begin이 실패했더라도 clearStmt는 해 주어야 한다.
                         SQLExecute()를 다시 했을때 바인딩 된 Paramdata로 인해
                         Invalid statement processing request 에러가 발생하기 때문이다. */
            sStatement->clearStmt(MMC_STMT_BIND_NONE);
        }

        sStatement->setExecuteFlag(ID_FALSE);
        
        IDE_DASSERT( sStatement->getIsShareTransSmiStmtLocked() == ID_FALSE );

        /* BUG-29078
         * XA_END 수신전에 XA_ROLLBACK을 먼저 수신한 경우에 XA_END를 Heuristic하게 처리한다.
         */
        if ( (sSession->isXaSession() == ID_TRUE) && 
             (sSession->getXaAssocState() == MMD_XA_ASSOC_STATE_ASSOCIATED) &&
             (sSession->getLastXid() != NULL) )
        {
            mmdXa::heuristicEnd(sSession, sSession->getLastXid());
        }

        IDE_POP();
    }

    return IDE_FAILURE;
}


IDE_RC mmtServiceThread::execute(cmiProtocolContext *aProtocolContext,
                                 mmcStatement       *aStatement,
                                 idBool              aDoAnswer,  
                                 idBool             *aSuspended,
                                 UInt               *aResultSetCount,
                                 SLong              *aAffectedRowCount,
                                 SLong              *aFetchedRowCount)
{
    mmtCmsExecuteContext sExecuteContext;

    sExecuteContext.mProtocolContext  = aProtocolContext;
    sExecuteContext.mStatement        = aStatement;
    sExecuteContext.mAffectedRowCount = -1;
    sExecuteContext.mFetchedRowCount  = -1;
    sExecuteContext.mSuspended        = ID_FALSE;
    sExecuteContext.mCollectionData   = NULL;
    sExecuteContext.mCursor           = 0;

    aStatement->setSimpleQuerySelectExecuted(ID_FALSE);

    IDE_TEST(doExecute(&sExecuteContext) != IDE_SUCCESS);

    if( (aDoAnswer == ID_TRUE) && (sExecuteContext.mSuspended != ID_TRUE) )
    {
        IDE_TEST(answerExecuteResult(&sExecuteContext, aStatement->getSession()) != IDE_SUCCESS);

        if ( aStatement->getSession()->isNeedRebuildNoti() == ID_TRUE )
        {
            IDE_TEST( sendShardRebuildNoti( aProtocolContext )
                      != IDE_SUCCESS );
        }
    }

    *aSuspended = sExecuteContext.mSuspended;
    
    if( aAffectedRowCount != NULL )
    {
        *aAffectedRowCount = sExecuteContext.mAffectedRowCount;
    }
    if (aFetchedRowCount != NULL)
    {
        *aFetchedRowCount = sExecuteContext.mFetchedRowCount;
    }

    if( aResultSetCount != NULL )
    {
        *aResultSetCount = aStatement->getResultSetCount();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC mmtServiceThread::executeIPCDASimpleQuery(cmiProtocolContext *aProtocolContext,
                                                 mmcStatement       *aStatement,
                                                 UShort             *aBindColInfo,
                                                 idBool              aDoAnswer,
                                                 idBool             *aSuspended,
                                                 UInt               *aResultSetCount,
                                                 SLong              *aAffectedRowCount,
                                                 SLong              *aFetchedRowCount,
                                                 UChar              *aBindBuffer)
{
    mmtCmsExecuteContext sExecuteContext;

    qciStatement * sQciStmt;
    UInt           sShmSize           = 0;
    UChar        * sShmResult         = NULL;
    UInt           sRowCount          = 0;
    UInt           sResultSetCount    = 0;
    UInt           sOldResultSetHWM   = 0;
    mmcSession   * sSession           = aStatement->getSession();
    mmcTransObj  * sTrans             = NULL;
    UInt           sIsTransBegin      = ID_FALSE;
    idBool         sRetry             = ID_FALSE;
    /* BUG-46804 */
    idBool         sIsReplStmt        = ID_FALSE;
    mmcTransObj  * sShareTrans        = NULL;
    idBool         sIsSmiStmtBegin    = ID_FALSE;
    idBool         sIsDummyBegin      = ID_FALSE;

    sExecuteContext.mProtocolContext  = aProtocolContext;
    sExecuteContext.mStatement        = aStatement;
    sExecuteContext.mAffectedRowCount = -1;
    sExecuteContext.mFetchedRowCount  = -1;
    sExecuteContext.mSuspended        = ID_FALSE;
    sExecuteContext.mCollectionData   = NULL;
    sExecuteContext.mCursor           = 0;

    aStatement->setSimpleQuerySelectExecuted(ID_FALSE);

    /* BUG-46625 IPCDA FATAL */
    // BUG-36203 PSM Optimize
    IDE_TEST( aStatement->changeStmtState() != IDE_SUCCESS );

    sQciStmt   = aStatement->getQciStmt();

    /* BUG-46804 */
    sIsReplStmt = qci::hasReplicationTable( sQciStmt );
    
    /* PROJ-2616 */
    /* For SimpleQuery*/
    sShmResult = aProtocolContext->mSimpleQueryFetchIPCDAWriteBlock.mData;
    sShmSize   = cmbBlockGetIPCDASimpleQueryDataBlockSize();

    /* PROJ-2177 User Interface - Cancel */
    aStatement->setExecuting(ID_TRUE);

    if (sSession->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT)
    {
        sTrans = aStatement->allocTrans();
        mmcTrans::begin( sTrans, 
                         sSession->getStatSQL(), 
                         0,
                         sSession,
                         &sIsDummyBegin );
        sIsTransBegin = ID_TRUE;
    }
    else
    {
        sTrans = sSession->getTransPtr();
    }

    sShareTrans = aStatement->getShareTransForSmiStmtLock(sTrans);
    if (sShareTrans != NULL)
    {
        aStatement->acquireShareTransSmiStmtLock(sShareTrans);
    }

    do
    {
        sRetry = ID_FALSE;
        if ( qci::fastExecute( mmcTrans::getSmiTrans(sTrans),
                               sQciStmt,
                               aBindColInfo,
                               aBindBuffer,
                               sShmSize,
                               sShmResult,
                               &sRowCount )
                  != IDE_SUCCESS )
        {
            /* PROJ-2705 Memory Parition Simple Query */
            switch ( ideGetErrorCode() & E_ACTION_MASK )
            {
                case E_ACTION_REBUILD:
                    /* BUG-46715 IPCDA Simple Query Stress Test FATAL */
                    if (sSession->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT)
                    {
                        IDE_TEST(mmcTrans::commit(sTrans, sSession) != IDE_SUCCESS);
                        sIsTransBegin = ID_FALSE;
                        mmcTrans::begin( sTrans, 
                                         sSession->getStatSQL(), 
                                         0, 
                                         sSession,
                                         &sIsDummyBegin );
                        sIsTransBegin = ID_TRUE;
                    }

                    /* BUG-47029 rebuild시에만 smiStatement.begin을 수행 */
                    aStatement->setCursorFlag(SMI_STATEMENT_ALL_CURSOR);
                    
                    IDE_TEST( aStatement->beginSmiStmt( sTrans, aStatement->getSmiStatementFlag( sSession, aStatement, sTrans) ) != IDE_SUCCESS );
                    sIsSmiStmtBegin = ID_TRUE;

                    IDE_TEST( aStatement->rebuild() != IDE_SUCCESS );

                    sIsSmiStmtBegin = ID_FALSE;
                    IDE_TEST( aStatement->endSmiStmt(SMI_STATEMENT_RESULT_FAILURE) != IDE_SUCCESS );
                    sRetry = ID_TRUE;
                    break;
                default:
                    IDE_RAISE( ExecuteFailAbort );
                    break;
            }
        }
    } while ( sRetry == ID_TRUE );

    /* BUG-46804 */
    IDV_SQL_ADD_DIRECT(aStatement->getStatistics(), mExecuteSuccessCount, 1);
    IDV_SESS_ADD_DIRECT(sSession->getStatistics(), IDV_STAT_INDEX_EXECUTE_SUCCESS_COUNT, 1);
    switch (aStatement->getStmtType())
    {
        case QCI_STMT_INSERT:
            if ( sIsReplStmt != ID_TRUE )
            {
                IDV_SQL_ADD_DIRECT(aStatement->getStatistics(), mExecuteInsertSuccessCount, 1);
                IDV_SESS_ADD_DIRECT(sSession->getStatistics(), IDV_STAT_INDEX_EXECUTE_INSERT_SUCCESS_COUNT, 1);
            }
            else
            {
                IDV_SQL_ADD_DIRECT(aStatement->getStatistics(), mExecuteReplInsertSuccessCount, 1);
                IDV_SESS_ADD_DIRECT(sSession->getStatistics(), IDV_STAT_INDEX_EXECUTE_REPL_INSERT_SUCCESS_COUNT, 1);
            }
            break;

        case QCI_STMT_UPDATE:
            if ( sIsReplStmt != ID_TRUE )
            {
                IDV_SQL_ADD_DIRECT(aStatement->getStatistics(), mExecuteUpdateSuccessCount, 1);
                IDV_SESS_ADD_DIRECT(sSession->getStatistics(), IDV_STAT_INDEX_EXECUTE_UPDATE_SUCCESS_COUNT, 1);
            }
            else
            {
                IDV_SQL_ADD_DIRECT(aStatement->getStatistics(), mExecuteReplUpdateSuccessCount, 1);
                IDV_SESS_ADD_DIRECT(sSession->getStatistics(), IDV_STAT_INDEX_EXECUTE_REPL_UPDATE_SUCCESS_COUNT, 1);
            }
            break;

        case QCI_STMT_DELETE:
            if ( sIsReplStmt != ID_TRUE )
            {
                IDV_SQL_ADD_DIRECT(aStatement->getStatistics(), mExecuteDeleteSuccessCount, 1);
                IDV_SESS_ADD_DIRECT(sSession->getStatistics(), IDV_STAT_INDEX_EXECUTE_DELETE_SUCCESS_COUNT, 1);
            }
            else
            {
                IDV_SQL_ADD_DIRECT(aStatement->getStatistics(), mExecuteReplDeleteSuccessCount, 1);
                IDV_SESS_ADD_DIRECT(sSession->getStatistics(), IDV_STAT_INDEX_EXECUTE_REPL_DELETE_SUCCESS_COUNT, 1);
            }
            break;

        case QCI_STMT_SELECT:
        case QCI_STMT_SELECT_FOR_FIXED_TABLE:
            IDV_SQL_ADD_DIRECT(aStatement->getStatistics(), mExecuteSelectSuccessCount, 1);
            IDV_SESS_ADD_DIRECT(sSession->getStatistics(), IDV_STAT_INDEX_EXECUTE_SELECT_SUCCESS_COUNT, 1);
            break;

        default:
            break;
    }

    if (sShareTrans != NULL)
    {
        aStatement->releaseShareTransSmiStmtLock(sShareTrans);
        sShareTrans = NULL;
    }

    sOldResultSetHWM = aStatement->getResultSetHWM();
    aStatement->setStmtState(MMC_STMT_STATE_EXECUTED);
    aStatement->setFetchFlag(MMC_FETCH_FLAG_CLOSE);

    /* BUG-45482 */
    sResultSetCount = qci::getResultSetCount(sQciStmt);
    aStatement->setResultSetCount(sResultSetCount);
    aStatement->setEnableResultSetCount(sResultSetCount);
    IDE_TEST_RAISE(aStatement->initializeResultSet(sResultSetCount) != IDE_SUCCESS, RestoreResultSetValues);

    /* BUG-46804 NonSimplQuery의 경우  MMC_FETCH_FLAG_CLOSE가 아닌경우 IDV_STAT_INDEX_FETCH_FAILURE_COUNT가 증가한다
     * ipcda simplqQuery의 경우 MMC_FETCH_FLAG_CLOSE만 셋팅되므로 IDV_STAT_INDEX_FETCH_FAILURE_COUNT 경우가 없다. */
    if (sResultSetCount > 0)
    {
        IDV_SQL_ADD_DIRECT(aStatement->getStatistics(), mFetchSuccessCount, 1);
        IDV_SESS_ADD_DIRECT(sSession->getStatistics(), IDV_STAT_INDEX_FETCH_SUCCESS_COUNT, 1);
    }

    if ((aStatement->getStmtType() == QCI_STMT_SELECT) ||
        (aStatement->getStmtType() == QCI_STMT_SELECT_FOR_UPDATE)) /* BUG-45257 */
    {
        qci::getRowCount(sQciStmt, &sExecuteContext.mAffectedRowCount, &sExecuteContext.mFetchedRowCount);
        aStatement->setSimpleQuerySelectExecuted(ID_TRUE);
    }
    else
    {
        sExecuteContext.mAffectedRowCount = sRowCount;
    }

    if (sSession->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT)
    {
        IDE_TEST(mmcTrans::commit(sTrans, sSession) != IDE_SUCCESS);
        sIsTransBegin = ID_FALSE;
    }
    
    /*PROJ-2616*/
    /*To avoid cursor_close */
    aStatement->setStmtState(MMC_STMT_STATE_PREPARED);

    aStatement->setExecuting(ID_FALSE);

    if( (aDoAnswer == ID_TRUE) && (sExecuteContext.mSuspended != ID_TRUE) )
    {
        IDE_TEST(answerExecuteResult(&sExecuteContext, sSession) != IDE_SUCCESS);

        if ( sSession->isNeedRebuildNoti() == ID_TRUE )
        {
            IDE_TEST( sendShardRebuildNoti( aProtocolContext )
                      != IDE_SUCCESS );
        }
    }

    *aSuspended = sExecuteContext.mSuspended;

    if( aAffectedRowCount != NULL )
    {
        *aAffectedRowCount = sExecuteContext.mAffectedRowCount;
    }
    if (aFetchedRowCount != NULL)
    {
        *aFetchedRowCount = sExecuteContext.mFetchedRowCount;
    }

    if( aResultSetCount != NULL )
    {
        *aResultSetCount = aStatement->getResultSetCount();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ExecuteFailAbort);
    {
        /* BUG-46804 */
        IDV_SQL_ADD_DIRECT(aStatement->getStatistics(), mExecuteFailureCount, 1);
        IDV_SESS_ADD_DIRECT(sSession->getStatistics(), IDV_STAT_INDEX_EXECUTE_FAILURE_COUNT, 1);
    }
    IDE_EXCEPTION( RestoreResultSetValues );
    {
        aStatement->setResultSetCount(sOldResultSetHWM);
        aStatement->setEnableResultSetCount(sOldResultSetHWM);
    }
    IDE_EXCEPTION_END;

    if (sShareTrans != NULL)
    {
        aStatement->releaseShareTransSmiStmtLock(sShareTrans);
        sShareTrans = NULL;
    }

    if (sIsSmiStmtBegin == ID_TRUE)
    {
        aStatement->endSmiStmt(SMI_STATEMENT_RESULT_FAILURE);
    }

    if (sIsTransBegin == ID_TRUE)
    {
        IDE_DASSERT(sSession->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT);
        IDE_DASSERT(sTrans->mSmiTrans.isBegin() == ID_TRUE);
        (void)mmcTrans::rollback(sTrans, sSession, NULL);
    }

    aStatement->setExecuting(ID_FALSE);

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::executeProtocol(cmiProtocolContext *aProtocolContext,
                                         cmiProtocol        *aProtocol,
                                         void               *aSessionOwner,
                                         void               *aUserContext)
{
    UInt        sStatementID;
    UInt        sRowNumber;
    UChar       sOption;

    mmcTask          *sTask          = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread        = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    mmcStatement     *sStatement;
    idBool            sSuspended     = ID_FALSE;

    UShort       * sBindColInfo       = NULL; /*PROJ-2616*/
    UShort         sBindColNum        = 0;    /*PROJ-2616*/

    smSCN          sRequestSCN;
    smSCN          sTxFirstStmtSCN;
    SLong          sTxFirstStmtTime = 0;
    UChar          sDistLevel       = 0;

    UInt           sClientTouchNodeArr[SDI_NODE_MAX_COUNT] = {0, };
    UShort         sClientTouchNodeCount = 0;
    UInt           i = 0;

    /* TASK-7219 Non-shard DML */
    UInt           sStmtExecSeqForShardTx = 0;

    SM_INIT_SCN(&sRequestSCN);
    SM_INIT_SCN(&sTxFirstStmtSCN);

    /* PROJ-2733-Protocol */
    switch (aProtocol->mOpID)
    {
        case CMP_OP_DB_ExecuteV3:
            CMI_RD4(aProtocolContext, &sStatementID);
            CMI_RD4(aProtocolContext, &sRowNumber);
            CMI_RD1(aProtocolContext, sOption);
            CMI_RD8(aProtocolContext, &sRequestSCN);
            /* PROJ-2733-DistTxInfo */
            CMI_RD8(aProtocolContext, &sTxFirstStmtSCN);
            CMI_RD8(aProtocolContext, (ULong *)&sTxFirstStmtTime);
            CMI_RD1(aProtocolContext, sDistLevel);
            /* TASK-7219 Non-shard DML */
            CMI_RD4(aProtocolContext, &sStmtExecSeqForShardTx);
            /* BUG-48315 */
            CMI_RD2(aProtocolContext, &sClientTouchNodeCount);
            IDE_DASSERT(sClientTouchNodeCount <= SDI_NODE_MAX_COUNT);
            for (i = 0; i < sClientTouchNodeCount; i++)
            {
                CMI_RD4(aProtocolContext, &(sClientTouchNodeArr[i]));
            }
            break;

        case CMP_OP_DB_ExecuteV2:
        case CMP_OP_DB_Execute:
            CMI_RD4(aProtocolContext, &sStatementID);
            CMI_RD4(aProtocolContext, &sRowNumber);
            CMI_RD1(aProtocolContext, sOption);
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
    {
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

    #if defined(DEBUG)
    ideLog::log(IDE_SD_18, "= [%s] executeProtocol"
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
    IDE_TEST_RAISE(sThread->atomicCheck(sStatement, &(sOption))
                                        != IDE_SUCCESS, SkipExecute);

    sSession->setClientTouchNodeCount(sClientTouchNodeCount);  /* BUG-48384 */

    /* BUG-48315 ShardCLI, User|LibSession에서만 TouchNode를 보낸다. */
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

    switch (sOption)
    {
        case CMP_DB_EXECUTE_NORMAL_EXECUTE:
            sStatement->setArray(ID_FALSE);
            sStatement->setRowNumber(sRowNumber);

            if ((cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) &&
                (sStatement->isSimpleQuery() == ID_TRUE) )
            {
                IDE_TEST(sThread->executeIPCDASimpleQuery(aProtocolContext,
                                                          sStatement,
                                                          sBindColInfo,
                                                          ID_TRUE,
                                                          &sSuspended,
                                                          NULL, 
                                                          NULL,
                                                          NULL,
                                                          NULL)
                         != IDE_SUCCESS);
            }
            else
            {
                IDE_TEST(sThread->execute(aProtocolContext,
                                          sStatement,
                                          ID_TRUE,
                                          &sSuspended,
                                          NULL, 
                                          NULL,
                                          NULL) != IDE_SUCCESS);
            }
            break;

        case CMP_DB_EXECUTE_ARRAY_EXECUTE:
            sStatement->setArray(ID_TRUE);
            sStatement->setRowNumber(sRowNumber);

            if ((cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) &&
                (sStatement->isSimpleQuery() == ID_TRUE))
            {
                IDE_TEST(sThread->executeIPCDASimpleQuery(aProtocolContext,
                                                          sStatement,
                                                          sBindColInfo,
                                                          ID_TRUE,
                                                          &sSuspended,
                                                          NULL, 
                                                          NULL,
                                                          NULL,
                                                          NULL) != IDE_SUCCESS);
            }
            else
            {
                IDE_TEST(sThread->execute(aProtocolContext,
                                          sStatement,
                                          ID_TRUE,
                                          &sSuspended,
                                          NULL, 
                                          NULL,
                                          NULL) != IDE_SUCCESS);
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
            if( sRowNumber == 1)
            {
                IDE_TEST_RAISE((sThread->atomicBegin(sStatement) != IDE_SUCCESS), SkipExecute)
            }

            IDE_TEST_RAISE(sThread->atomicExecute(sStatement, aProtocolContext) != IDE_SUCCESS, SkipExecute)
            break;

        case CMP_DB_EXECUTE_ATOMIC_BEGIN:
            sThread->atomicInit(sStatement);
            break;
        case CMP_DB_EXECUTE_ATOMIC_END:
            // 이 시점에서 sRowNumber 는 1이다.
            sStatement->setRowNumber(sRowNumber);
            IDE_TEST(sThread->atomicEnd(sStatement, aProtocolContext) != IDE_SUCCESS );
            break;
        default:
            IDE_RAISE(InvalidExecuteOption);
            break;
    }

    IDE_EXCEPTION_CONT(SkipExecute);

    return (sSuspended == ID_TRUE) ? IDE_CM_STOP : IDE_SUCCESS;

    IDE_EXCEPTION(InvalidStatementState);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION(InvalidExecuteOption);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_EXECUTE_OPTION));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aProtocolContext,
                                      aProtocol->mOpID,
                                      sRowNumber,
                                      sSession);
}

IDE_RC mmtServiceThread::outBindLobCallback(void         *aMmSession,
                                            smLobLocator *aOutLocator,
                                            smLobLocator *aOutFirstLocator,
                                            UShort        aOutCount)
{
    mmcSession    *sSession = (mmcSession *)aMmSession;
    mmcLobLocator *sLobLocator;
    UShort         sIndex;

    for (sIndex = 0; sIndex < aOutCount; sIndex++)
    {
        IDE_TEST(sSession->findLobLocator(&sLobLocator,
                                          aOutFirstLocator[sIndex]) != IDE_SUCCESS);

        if (sLobLocator != NULL)
        {
            IDE_TEST(mmcLob::addLocator(sLobLocator,
                                        sSession->getInfo()->mCurrStmtID,
                                        aOutLocator[sIndex]) != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST(sSession->allocLobLocator(&sLobLocator,
                                               sSession->getInfo()->mCurrStmtID,
                                               aOutLocator[sIndex]) != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::closeOutBindLobCallback(void         *aMmSession,
                                                 smLobLocator *aOutFirstLocator,
                                                 UShort        aOutCount)
{
    mmcSession *sSession = (mmcSession *)aMmSession;
    UShort      sIndex;
    idBool      sFound;

    for (sIndex = 0; sIndex < aOutCount; sIndex++)
    {
        IDE_TEST(sSession->freeLobLocator(aOutFirstLocator[sIndex],
                                          &sFound) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1518
IDE_RC mmtServiceThread::atomicCheck(mmcStatement * aStatement, UChar *aOption)
{
    // INSERT 구문이 아닐때는 일반 ARRAY로 변경한다.
    if(aStatement->getStmtType() != QCI_STMT_INSERT)
    {
        switch((*aOption))
        {
            case CMP_DB_EXECUTE_ATOMIC_EXECUTE:
                (*aOption) = CMP_DB_EXECUTE_ARRAY_EXECUTE;
                break;
            case CMP_DB_EXECUTE_ATOMIC_BEGIN:
                (*aOption) = CMP_DB_EXECUTE_ARRAY_BEGIN;
                break;
            case CMP_DB_EXECUTE_ATOMIC_END:
                (*aOption) = CMP_DB_EXECUTE_ARRAY_END;
                break;
        }
    }
    else
    {
        if((*aOption) == CMP_DB_EXECUTE_ATOMIC_EXECUTE)
        {
            IDE_TEST( aStatement->getAtomicExecSuccess() != ID_TRUE);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void mmtServiceThread::atomicInit(mmcStatement * aStatement)
{
    aStatement->setAtomic(ID_TRUE);
    aStatement->setAtomicExecSuccess(ID_TRUE);
    // BUG-21489
    aStatement->clearAtomicLastErrorCode();
}

// A7 과 A5 client 공용임
IDE_RC mmtServiceThread::atomicBegin(mmcStatement * aStatement)
{
    idBool            sRetry;
    mmcAtomicInfo    *sAtomicInfo = aStatement->getAtomicInfo();
    qciStatement     *sQciStmt    = aStatement->getQciStmt();

    IDE_TEST_RAISE( aStatement->getAtomicExecSuccess() != ID_TRUE, AtomicExecuteFail);

    // PROJ-2163
    IDE_TEST(aStatement->reprepare() != IDE_SUCCESS);

    IDE_TEST(aStatement->beginStmt() != IDE_SUCCESS);

    IDV_SQL_OPTIME_BEGIN( aStatement->getStatistics(), IDV_OPTM_INDEX_QUERY_EXECUTE );

    aStatement->setExecuteFlag(ID_TRUE);

    do
    {
        sRetry = ID_FALSE;
        // Rebuild 에러를 처리한다.
        if ( qci::atomicBegin(sQciStmt,
                              aStatement->getSmiStmt()) != IDE_SUCCESS)
        {
            switch (ideGetErrorCode() & E_ACTION_MASK)
            {
                case E_ACTION_IGNORE:
                    IDE_RAISE(AtomicExecuteFail);
                    break;
    
                case E_ACTION_RETRY:
                    IDE_TEST(doRetry(aStatement) != IDE_SUCCESS);
                    sRetry = ID_TRUE;
                    break;
    
                case E_ACTION_REBUILD:
                    IDE_TEST(doRebuild(aStatement) != IDE_SUCCESS);
                    sRetry = ID_TRUE;
                    break;
    
                case E_ACTION_ABORT:
                    IDE_RAISE(ExecuteFailAbort);
                    break;
    
                case E_ACTION_FATAL:
                    IDE_CALLBACK_FATAL("fatal error returned from mmcStatement::atomicBegin()");
                    break;
    
                default:
                    IDE_CALLBACK_FATAL("invalid error action returned from mmcStatement::atomicBegin()");
                    break;
            }
        }
    } while(sRetry == ID_TRUE);

    sAtomicInfo->mIsCursorOpen = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(AtomicExecuteFail);
    {
        // No Action
    }
    IDE_EXCEPTION(ExecuteFailAbort);
    {
        // No Action
    }
    IDE_EXCEPTION_END;
    {
        aStatement->setAtomicExecSuccess(ID_FALSE);
        // BUG-21489
        aStatement->setAtomicLastErrorCode(ideGetErrorCode());
    }

    return IDE_FAILURE;
}

// A5용이 별도로 있음
IDE_RC mmtServiceThread::atomicExecute(mmcStatement * aStatement, cmiProtocolContext *aProtocolContext)
{
    qciStatement          *sQciStmt    = aStatement->getQciStmt();
    mmtCmsExecuteContext   sExecuteContext;

    sExecuteContext.mProtocolContext  = aProtocolContext;
    sExecuteContext.mStatement        = aStatement;
    sExecuteContext.mAffectedRowCount = -1;
    sExecuteContext.mFetchedRowCount  = -1;
    sExecuteContext.mSuspended        = ID_FALSE;
    sExecuteContext.mCollectionData   = NULL;
    sExecuteContext.mCursor           = 0;

    IDE_TEST(qci::atomicInsert(sQciStmt) != IDE_SUCCESS);

    IDE_TEST( sendBindOut( aStatement->getSession(), &sExecuteContext ) != IDE_SUCCESS );

    // PROJ-2163
    // insert 후에 바로 데이타를 bind 해야하므로 EXEC_PREPARED 상태로 변경해야 한다.
    IDE_TEST( qci::atomicSetPrepareState( sQciStmt ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END
    {
        aStatement->setAtomicExecSuccess(ID_FALSE);
        // BUG-21489
        aStatement->setAtomicLastErrorCode(ideGetErrorCode());
    }

    return IDE_FAILURE;
}

// A5용이 별도로 있음
IDE_RC mmtServiceThread::atomicEnd(mmcStatement * aStatement, cmiProtocolContext *aProtocolContext)
{
    mmcSession            *sSession    = aStatement->getSession();
    idvSQL                *sStatistics = aStatement->getStatistics();
    qciStatement          *sQciStmt    = aStatement->getQciStmt();
    mmcAtomicInfo         *sAtomicInfo = aStatement->getAtomicInfo();
    mmtCmsExecuteContext   sExecuteContext;

    sExecuteContext.mProtocolContext  = aProtocolContext;
    sExecuteContext.mStatement        = aStatement;
    sExecuteContext.mAffectedRowCount = -1;
    sExecuteContext.mFetchedRowCount  = -1;
    sExecuteContext.mSuspended        = ID_FALSE;
    sExecuteContext.mCollectionData   = NULL;
    sExecuteContext.mCursor           = 0;

    // List 형태의 프로토콜이 올때 1번째 row의 바인드가 실패시 Begin이 안할수 있다.
    IDE_TEST_RAISE( aStatement->isStmtBegin() != ID_TRUE, AtomicExecuteFail);

    IDE_TEST_RAISE( aStatement->getAtomicExecSuccess() != ID_TRUE, AtomicExecuteFail);

    sAtomicInfo->mIsCursorOpen = ID_FALSE;

    if ( qci::atomicEnd(sQciStmt) != IDE_SUCCESS)
    {
        switch (ideGetErrorCode() & E_ACTION_MASK)
        {
            case E_ACTION_IGNORE:
                IDE_RAISE(AtomicExecuteFail);
                break;

            // fix BUG-30449
            // RETRY 에러를 클라이언트에게 ABORT로 전달해
            // ATOMIC ARRAY INSERT 실패를 알려준다.
            case E_ACTION_RETRY:
            case E_ACTION_REBUILD:
                IDE_RAISE(ExecuteRetry);
                break;

            case E_ACTION_ABORT:
                IDE_RAISE(ExecuteFailAbort);
                break;

            case E_ACTION_FATAL:
                IDE_CALLBACK_FATAL("fatal error returned from mmcStatement::atomicEnd()");
                break;

            default:
                IDE_CALLBACK_FATAL("invalid error action returned from mmcStatement::atomicEnd()");
                break;
        }
    }

    qci::getRowCount(sQciStmt, &sExecuteContext.mAffectedRowCount, &sExecuteContext.mFetchedRowCount);

    IDV_SQL_OPTIME_END( sStatistics, IDV_OPTM_INDEX_QUERY_EXECUTE );

    IDV_SESS_ADD_DIRECT(sSession->getStatistics(),
                        IDV_STAT_INDEX_EXECUTE_SUCCESS_COUNT, 1);

    IDV_SQL_ADD_DIRECT(sStatistics, mExecuteSuccessCount, 1);
    IDV_SQL_ADD_DIRECT(sStatistics, mProcessRow, (ULong)(sExecuteContext.mAffectedRowCount));

    aStatement->setStmtState(MMC_STMT_STATE_EXECUTED);

    if (qci::closeCursor(aStatement->getQciStmt(), aStatement->getSmiStmt()) == IDE_SUCCESS)
    {
        IDE_TEST(doEnd(&sExecuteContext, MMC_EXECUTION_FLAG_SUCCESS) != IDE_SUCCESS);
    }

    IDE_TEST(answerExecuteResult(&sExecuteContext, sSession) != IDE_SUCCESS);

    if ( sSession->isNeedRebuildNoti() == ID_TRUE )
    {
        IDE_TEST( sendShardRebuildNoti( aProtocolContext )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(AtomicExecuteFail);
    {
        // BUG-21489
        IDE_SET(ideSetErrorCodeAndMsg(sAtomicInfo->mAtomicLastErrorCode,
                                      sAtomicInfo->mAtomicErrorMsg));

    }
    IDE_EXCEPTION(ExecuteRetry);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_ATOMIC_EXECUTE_ERROR));
    }
    IDE_EXCEPTION(ExecuteFailAbort);
    {
        // No Action
    }
    IDE_EXCEPTION_END
    {
        IDE_PUSH();

        IDV_SQL_OPTIME_END( sStatistics,
                            IDV_OPTM_INDEX_QUERY_EXECUTE );

        IDV_SESS_ADD_DIRECT(sSession->getStatistics(),
                            IDV_STAT_INDEX_EXECUTE_FAILURE_COUNT, 1);
        IDV_SQL_ADD_DIRECT(sStatistics, mExecuteFailureCount, 1);

        aStatement->setExecuteFlag(ID_FALSE);

        if (aStatement->isStmtBegin() == ID_TRUE)
        {
            /* PROJ-2223 Altibase Auditing */
            mmtAuditManager::auditByAccess( aStatement, MMC_EXECUTION_FLAG_FAILURE );
            
            /*
             * [BUG-24187] Rollback될 statement는 Internal CloseCurosr를
             * 수행할 필요가 없습니다.
             */
            aStatement->setSkipCursorClose();
            aStatement->clearStmt(MMC_STMT_BIND_NONE);

            IDE_ASSERT(aStatement->endStmt(MMC_EXECUTION_FLAG_FAILURE) == IDE_SUCCESS);
        }

        IDE_POP();
    }
    return IDE_FAILURE;
}

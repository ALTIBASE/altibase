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
 

/***********************************************************************
 * $Id: ulaAPI.c 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mtcd.h>
#include <mtcdTypes.h>
#include <ulaErrorMgr.h>
#include <ulaLog.h>
#include <ulaXLogCollector.h>
#include <ulaComm.h>
#include <alaAPI.h>
#include <sqlcli.h>
#include <ulnTypes.h>
#include <ulnData.h>
#include <ulnConv.h>
#include <ulnConvNumeric.h>

#include <ulaConvFmMT.h>
#include <ulaConvNumeric.h>
#include <alaTypes.h>

/* NOTICE : MTD에서 사용되는 Date Format 형식이 4.5.1 Branch와 Main Trunk이 다름 */
#define ALA_MTD_DATE_FORMAT         "YYYY-MM-DD HH:MI:SS.SSSSSS"
#define ALA_MTD_DATE_FORMAT_LEN     (26)

#define ALA_DEFAULT_DATE_FORMAT     "YYYY-MM-DD HH:MI:SS.SSSSSS"
#define ALA_DEFAULT_DATE_FORMAT_LEN (26)

ACI_RC ulaGetAltibaseSQLColumn(ALA_Column   *aColumn,
                               ALA_Value    *aValue,
                               UInt          aBufferSize,
                               SChar        *aOutBuffer,
                               ALA_ErrorMgr *aOutErrorMgr)
{
    acp_rc_t      sRc;
    acp_sint32_t  sUsedSize          = 0;

    acp_bool_t    sNeedDateFormat    = ACP_FALSE;
    acp_bool_t    sNeedSingleQuote   = ACP_FALSE;
    acp_bool_t    sNeedParentthesis  = ACP_FALSE; // ()
    acp_bool_t    sIsNull            = ACP_FALSE;
    acp_bool_t    sIsSingleQuoteText = ACP_FALSE;

    acp_char_t   *sPrevPtr           = NULL;
    acp_char_t   *sCurrPtr           = NULL;
    acp_uint32_t  sSingleQuoteCnt    = 0;

    ACI_TEST_RAISE(aColumn == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aValue == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutBuffer == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(aBufferSize < 1, ERR_PARAMETER_INVALID);
    aOutBuffer[0] = '\0';

    if (aValue->value != NULL)
    {
        switch (aColumn->mDataType)
        {
            case MTD_DATE_ID :
                // BUG-18550
                if (MTD_DATE_IS_NULL((const mtdDateType *)aValue->value) == 0)
                {
                    sNeedDateFormat = ACP_TRUE;
                    sNeedSingleQuote = ACP_TRUE;
                }
                else
                {
                    sIsNull = ACP_TRUE;
                }
                break;

            case MTD_CHAR_ID :
            case MTD_VARCHAR_ID :
                sIsSingleQuoteText = ACP_TRUE;
                sNeedSingleQuote = ACP_TRUE;
                break;

            case MTD_NCHAR_ID :
            case MTD_NVARCHAR_ID :
                sRc = acpCStrCat((acp_char_t*)(aOutBuffer + sUsedSize),
                                 aBufferSize - sUsedSize,
                                 "UNISTR",
                                 6);
                /* BUGBUG : ETRUNC 가 처리되고 있지 않다. */
                ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);
                sUsedSize += 6;

                sNeedSingleQuote = ACP_TRUE;
                sNeedParentthesis = ACP_TRUE; // ()
                break;

            case MTD_BYTE_ID :
            case MTD_VARBYTE_ID :
            case MTD_NIBBLE_ID :
                sNeedSingleQuote = ACP_TRUE;
                break;

            case MTD_BIT_ID :
                sRc = acpCStrCat((acp_char_t*)(aOutBuffer + sUsedSize),
                                 aBufferSize - sUsedSize,
                                 "BIT",
                                 3);
                /* BUGBUG : ETRUNC 가 처리되고 있지 않다. */
                ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);
                sUsedSize += 3;

                sNeedSingleQuote = ACP_TRUE;
                break;

            case MTD_VARBIT_ID :
                sRc = acpCStrCat((acp_char_t*)(aOutBuffer + sUsedSize),
                                 aBufferSize - sUsedSize,
                                 "VARBIT",
                                 6);
                /* BUGBUG : ETRUNC 가 처리되고 있지 않다. */
                ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);
                sUsedSize += 6;

                sNeedSingleQuote = ACP_TRUE;
                break;

            case MTD_BLOB_ID :
            case MTD_CLOB_ID :
            case MTD_GEOMETRY_ID :
                sIsNull = ACP_TRUE;
                break;

            default :
                break;
        }

        // BUG-18550
        if (sNeedDateFormat == ACP_TRUE)
        {
            sRc = acpCStrCat((acp_char_t*)(aOutBuffer + sUsedSize),
                             aBufferSize - sUsedSize,
                             "TO_DATE(",
                             8);
            /* BUGBUG : ETRUNC 가 처리되고 있지 않다. */
            ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);
            sUsedSize += 8;
        }

        if (sIsNull != ACP_TRUE)
        {
            if (sNeedParentthesis == ACP_TRUE)
            {
                /*
                 * BUGBUG : 경계체크가 이루어지고 있지 않다
                 */
                aOutBuffer[sUsedSize++] = '(';
                aOutBuffer[sUsedSize]   = '\0';
            }

            if (sNeedSingleQuote == ACP_TRUE)
            {
                /*
                 * BUGBUG : 경계체크가 이루어지고 있지 않다
                 */
                aOutBuffer[sUsedSize++] = '\'';
                aOutBuffer[sUsedSize]   = '\0';
            }

            ACI_TEST(ALA_GetAltibaseText(aColumn,
                                         aValue,
                                         aBufferSize - sUsedSize,
                                         (SChar*)&(aOutBuffer[sUsedSize]),
                                         aOutErrorMgr)
                     != ALA_SUCCESS);

            // BUG-18715
            if (sIsSingleQuoteText == ACP_TRUE)
            {
                // Forward Pass : Count Single Quote
                sCurrPtr = (acp_char_t*)&(aOutBuffer[sUsedSize]);
                while (*sCurrPtr != '\0')
                {
                    if (*sCurrPtr == '\'')
                    {
                        sSingleQuoteCnt ++;
                    }
                    sCurrPtr ++;
                }

                // Set End Point
                sPrevPtr = sCurrPtr - 1;
                sCurrPtr += sSingleQuoteCnt;
                ACI_TEST_RAISE((acp_uint32_t)(sCurrPtr - (acp_char_t*)aOutBuffer)>= aBufferSize,
                               ERR_PARAMETER_INVALID);
                *sCurrPtr = '\0';

                // Backward Pass : Insert Single Quote
                for (sCurrPtr --;
                     sPrevPtr >= (acp_char_t*)&(aOutBuffer[sUsedSize]);
                     sPrevPtr --, sCurrPtr --)
                {
                    *sCurrPtr = *sPrevPtr;
                    if (*sPrevPtr == '\'')
                    {
                        *(--sCurrPtr) = '\'';
                    }
                }
            }

            if (sNeedSingleQuote == ACP_TRUE)
            {
                sRc = acpCStrCat((acp_char_t*)(aOutBuffer + sUsedSize),
                                 aBufferSize - sUsedSize,
                                 "\'",
                                 1);
                /* BUGBUG : ETRUNC 가 처리되고 있지 않다. */
                ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);
                sUsedSize += 1;
            }

            if (sNeedParentthesis == ACP_TRUE)
            {
                sRc = acpCStrCat((acp_char_t*)(aOutBuffer + sUsedSize),
                                 aBufferSize - sUsedSize,
                                 ")",
                                 1);
                /* BUGBUG : ETRUNC 가 처리되고 있지 않다. */
                ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);
                sUsedSize += 1;
            }

        }
        else
        {
            sRc = acpCStrCat((acp_char_t*)(aOutBuffer + sUsedSize),
                             aBufferSize - sUsedSize,
                             "NULL",
                             4);
            /* BUGBUG : ETRUNC 가 처리되고 있지 않다. */
            ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);
            sUsedSize += 4;
        }

        // BUG-18550
        if (sNeedDateFormat == ACP_TRUE)
        {
            sRc = acpCStrCat((acp_char_t*)(aOutBuffer + sUsedSize),
                             aBufferSize - sUsedSize,
                             ", \'"ALA_DEFAULT_DATE_FORMAT"\')",
                             ALA_DEFAULT_DATE_FORMAT_LEN + 5);
            /* BUGBUG : ETRUNC 가 처리되고 있지 않다. */
            ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);
            sUsedSize += ALA_DEFAULT_DATE_FORMAT_LEN + 5;
        }
    }
    else
    {
        ACI_TEST_RAISE(aBufferSize < 5, ERR_PARAMETER_INVALID);
        acpMemCpy(aOutBuffer, "NULL", 5);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Altibase SQL Column");
    }

    ACI_EXCEPTION(ERR_PARAMETER_INVALID)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_INVALID,
                        "Altibase SQL Column");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaGetAltibaseSQLInsert(ALA_Table    *aTable,
                               ALA_XLog     *aXLog,
                               UInt          aBufferSize,
                               SChar        *aOutBuffer,
                               ALA_ErrorMgr *aOutErrorMgr)
{
    acp_rc_t      sRc             = ACP_RC_SUCCESS;
    ALA_Column   *sColumn         = NULL;
    ALA_Value    *sValue          = NULL;
    acp_sint32_t  sUsedSize       = 0;
    acp_bool_t    sIsHiddenColumn = ACP_FALSE;
    acp_uint32_t  sIndex;

    ACI_TEST_RAISE(aTable == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutBuffer == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(aBufferSize < 1, ERR_PARAMETER_INVALID);
    aOutBuffer[0] = '\0';

    sRc = acpSnprintf((acp_char_t*)aOutBuffer,
                      aBufferSize,
                      "INSERT INTO \"%s\".\"%s\" VALUES (", // BUG-18609
                      aTable->mToUserName,
                      aTable->mToTableName);
    /* BUGBUG : ETRUNC 는 따로 처리해야 하는데... */
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);

    sUsedSize = acpCStrLen((acp_char_t*)aOutBuffer, aBufferSize);

    // VALUE Clause
    for (sIndex = 0; sIndex < aXLog->mColumn.mColCnt; sIndex++)
    {
        ACI_TEST( ulaMetaGetColumnInfo( (ulaTable *)aTable,
                                        aXLog->mColumn.mCIDArray[sIndex],
                                        (ulaColumn **)&sColumn,
                                        (ulaErrorMgr *)aOutErrorMgr )
                  != ACI_SUCCESS);

        ACE_DASSERT(sColumn != NULL);
        ACI_TEST_RAISE(sColumn == NULL, ERR_PARAMETER_INVALID);

        /* hidden column이 아니면 내용을 출력한다. */
        sIsHiddenColumn = ulaMetaIsHiddenColumn( (ulaColumn *)sColumn );
        if ( sIsHiddenColumn != ACP_TRUE )
        {
            if ( sIndex > 0 ) 
            {
                /*
                 * acpCStrCat 내부에서 부르는 strlen 을 피하기 위해서 + sUsedSize함
                 */
                sRc = acpCStrCat((acp_char_t*)aOutBuffer + sUsedSize,
                                 aBufferSize - sUsedSize,
                                 ", ",
                                 2);
                /* BUGBUG : ETRUNC 가 처리되고 있지 않다. */
                ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);
                sUsedSize = acpCStrLen( (acp_char_t*)aOutBuffer, aBufferSize);
            }
            else
            {
                /* do nothing */
            } 
            
            sValue = &(aXLog->mColumn.mAColArray[sIndex]);

            // Column Value
            ACI_TEST(ulaGetAltibaseSQLColumn(sColumn,
                                             sValue,
                                             aBufferSize - sUsedSize,
                                             &(aOutBuffer[sUsedSize]),
                                             aOutErrorMgr)
                     != ACI_SUCCESS);
        }
        else
        {
            /* do nothing */
        }
    }

    /*
     * acpCStrCat 내부에서 부르는 strlen 을 피하기 위해서 + sUsedSize함
     */
    sRc = acpCStrCat((acp_char_t*)(aOutBuffer + sUsedSize),
                     aBufferSize - sUsedSize,
                     ")",
                     1);
    /* BUGBUG : ETRUNC 가 처리되고 있지 않다. */
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);
    sUsedSize = acpCStrLen((acp_char_t*)aOutBuffer, aBufferSize);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Altibase SQL Insert");
    }

    ACI_EXCEPTION(ERR_PARAMETER_INVALID)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_INVALID,
                        "Altibase SQL Insert");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaGetAltibaseSQLDelete(ALA_Table    *aTable,
                               ALA_XLog     *aXLog,
                               UInt          aBufferSize,
                               SChar        *aOutBuffer,
                               ALA_ErrorMgr *aOutErrorMgr);

ACI_RC ulaGetAltibaseSQLUpdate(ALA_Table    *aTable,
                               ALA_XLog     *aXLog,
                               UInt          aBufferSize,
                               SChar        *aOutBuffer,
                               ALA_ErrorMgr *aOutErrorMgr);

ACI_RC ulaConvertCharSet(const mtlModule  *aSrcCharSet,
                         const mtlModule  *aDestCharSet,
                         mtdNcharType     *aSource,
                         UChar            *aResult,
                         UInt             *aResultLen);

/* Error Handling API */
ALA_RC ALA_ClearErrorMgr(ALA_ErrorMgr *aOutErrorMgr)
{
    ACI_TEST_RAISE(aOutErrorMgr == NULL, ERR_ERROR_MGR_NULL);

    ulaClearErrorMgr((ulaErrorMgr *)aOutErrorMgr);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_ERROR_MGR_NULL)
    {
        (void)ulaLogTrace((ulaErrorMgr *)aOutErrorMgr,
                          /* ULA_TRC_E_ERROR_MGR_NULL); */
                          ULA_TRC_E_ERROR_MGR_NULL);
    }
    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_GetErrorCode(const ALA_ErrorMgr *aErrorMgr,
                        UInt               *aOutErrorCode)
{
    ACI_TEST_RAISE(aErrorMgr == NULL, ERR_ERROR_MGR_NULL);

    // 내부 인덱스 정보(마지막 12비트)를 제거합니다.
    *aOutErrorCode = ACI_E_ERROR_CODE(aErrorMgr->mErrorCode);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_ERROR_MGR_NULL)
    {
        (void)ulaLogTrace((ulaErrorMgr *)aErrorMgr, ULA_TRC_E_ERROR_MGR_NULL);
    }
    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_GetErrorLevel(const ALA_ErrorMgr *aErrorMgr,
                         ALA_ErrorLevel     *aOutErrorLevel)
{
    acp_uint32_t sAction;

    ACI_TEST_RAISE(aErrorMgr == NULL, ERR_ERROR_MGR_NULL);

    sAction = aErrorMgr->mErrorCode & ACI_E_ACTION_MASK;
    switch (sAction)
    {
        case ACI_E_ACTION_FATAL :
            *aOutErrorLevel = ALA_ERROR_FATAL;
            break;

        case ACI_E_ACTION_ABORT :
            *aOutErrorLevel = ALA_ERROR_ABORT;
            break;

        case ACI_E_ACTION_IGNORE :
            *aOutErrorLevel = ALA_ERROR_INFO;
            break;

        default :
            ACI_RAISE(ERR_INVALID_ACTION);
    }

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_ERROR_MGR_NULL)
    {
        (void)ulaLogTrace((ulaErrorMgr *)aErrorMgr, ULA_TRC_E_ERROR_MGR_NULL);
    }
    ACI_EXCEPTION(ERR_INVALID_ACTION)
    {
        (void)ulaLogTrace((ulaErrorMgr *)aErrorMgr,
                          ULA_TRC_E_INVALID_ACTION,
                          sAction);
    }
    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_GetErrorMessage(const ALA_ErrorMgr   *aErrorMgr,
                           const SChar         **aOutErrorMessage)
{
    ACI_TEST_RAISE(aErrorMgr == NULL, ERR_ERROR_MGR_NULL);

    *aOutErrorMessage = aErrorMgr->mErrorMessage;

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_ERROR_MGR_NULL)
    {
        (void)ulaLogTrace((ulaErrorMgr *)aErrorMgr, ULA_TRC_E_ERROR_MGR_NULL);
    }
    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

/* Logging API */
ALA_RC ALA_EnableLogging(const SChar   *aLogDirectory,
                         const SChar   *aLogFileName,
                         UInt           aFileSize,
                         UInt           aMaxFileNumber,
                         ALA_ErrorMgr  *aOutErrorMgr)
{
    ACI_TEST(ulaEnableLogging((acp_char_t*)aLogDirectory,
                              (acp_char_t*)aLogFileName,
                              aFileSize,
                              aMaxFileNumber,
                              (ulaErrorMgr *)aOutErrorMgr) != ACI_SUCCESS);

    return ALA_SUCCESS;

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_DisableLogging(ALA_ErrorMgr *aOutErrorMgr)
{
    ACI_TEST(ulaDisableLogging((ulaErrorMgr *)aOutErrorMgr) != ACI_SUCCESS);

    return ALA_SUCCESS;

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}


/* Preparation API */
ALA_RC ALA_InitializeAPI(ALA_BOOL      aUseAltibaseODBCDriver,
                         ALA_ErrorMgr *aOutErrorMgr)
{
    acp_bool_t sInitCM = ACP_FALSE;
    acp_bool_t sInitMT = ACP_FALSE;

    if (aUseAltibaseODBCDriver != ALA_TRUE)
    {
        ACI_TEST_RAISE(cmiInitialize(ACP_UINT32_MAX) != ACI_SUCCESS,
                       ERR_INITIALIZE);
        sInitCM = ACP_TRUE;
    }

    ACI_TEST_RAISE(mtcInitializeForClient((acp_char_t *)"US7ASCII")
                   != ACI_SUCCESS,
                   ERR_INITIALIZE);
    sInitMT = ACP_TRUE;

    ulaCommInitialize();

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_INITIALIZE)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_API_INITIALIZE);

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    if (sInitMT != ACP_FALSE)
    {
        (void)mtcFinalizeForClient();
    }

    if (sInitCM != ACP_FALSE)
    {
        (void)cmiFinalize();
    }

    return ALA_FAILURE;
}

ALA_RC ALA_CreateXLogCollector(const SChar  *aXLogSenderName,
                               const SChar  *aSocketInfo,
                               SInt          aXLogPoolSize,
                               ALA_BOOL      aUseCommittedTxBuffer,
                               UInt          aACKPerXLogCount,
                               ALA_Handle   *aOutHandle,
                               ALA_ErrorMgr *aOutErrorMgr)
{
    acp_rc_t          sRc;
    ulaXLogCollector *sXLogCollector = NULL;

    ACI_TEST_RAISE(aOutHandle == NULL, ERR_PARAMETER_NULL);

    sRc = acpMemAlloc((void **)&sXLogCollector, ACI_SIZEOF(ulaXLogCollector));
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ALA_ERR_MEM_ALLOC);

    acpMemSet(sXLogCollector, 0x00, ACI_SIZEOF(ulaXLogCollector));

    ACI_TEST_RAISE(ulaXLogCollectorInitialize(sXLogCollector,
                                              (acp_char_t*)aXLogSenderName,
                                              (acp_char_t*)aSocketInfo,
                                              aXLogPoolSize,
                                              (acp_bool_t)aUseCommittedTxBuffer,
                                              aACKPerXLogCount,
                                              (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS,
                   ERR_INNER_API);

    *aOutHandle = (ALA_Handle)sXLogCollector;

    (void)ulaLogTrace((ulaErrorMgr *)aOutErrorMgr,
                      ULA_TRC_I_XC_CREATE,
                      aXLogSenderName);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Handle");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ALA_ERR_MEM_ALLOC)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr, ulaERR_ABORT_MEMORY_ALLOC);

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    if (sXLogCollector != NULL)
    {
        acpMemFree(sXLogCollector);
    }

    return ALA_FAILURE;
}
ALA_RC ALA_SetXLogPoolSize(ALA_Handle     aHandle, 
                           SInt           aXLogPoolSize,
                           ALA_ErrorMgr  *aOutErrorMgr)
{
    ulaXLogCollector *sXLogCollector = (ulaXLogCollector *)aHandle;

    ACI_TEST_RAISE(sXLogCollector == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(ulaXLogCollectorSetXLogPoolSize(sXLogCollector,
                                               (acp_sint32_t)aXLogPoolSize,
                                               (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS, ERR_INNER_API);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Handle");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}
ALA_RC ALA_AddAuthInfo(ALA_Handle        aHandle,
                       const SChar      *aAuthInfo,
                       ALA_ErrorMgr     *aOutErrorMgr)
{
    ulaXLogCollector *sXLogCollector = (ulaXLogCollector *)aHandle;

    ACI_TEST_RAISE(sXLogCollector == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(ulaXLogCollectorAddAuthInfo(sXLogCollector,
                                               (acp_char_t*)aAuthInfo,
                                               (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS,
                   ERR_INNER_API);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Handle");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_RemoveAuthInfo(ALA_Handle    aHandle,
                          const SChar  *aAuthInfo,
                          ALA_ErrorMgr *aOutErrorMgr)
{
    ulaXLogCollector *sXLogCollector = (ulaXLogCollector *)aHandle;

    ACI_TEST_RAISE(sXLogCollector == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(ulaXLogCollectorRemoveAuthInfo(sXLogCollector,
                                                  (acp_char_t*)aAuthInfo,
                                                  (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS,
                   ERR_INNER_API);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Handle");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_SetHandshakeTimeout(ALA_Handle    aHandle,
                               UInt          aSecond,
                               ALA_ErrorMgr *aOutErrorMgr)
{
    ulaXLogCollector *sXLogCollector = (ulaXLogCollector *)aHandle;

    ACI_TEST_RAISE(sXLogCollector == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(ulaXLogCollectorSetHandshakeTimeout
                                                (sXLogCollector,
                                                aSecond,
                                                (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS,
                   ERR_INNER_API);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Handle");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_SetReceiveXLogTimeout(ALA_Handle    aHandle,
                                 UInt          aSecond,
                                 ALA_ErrorMgr *aOutErrorMgr)
{
    ulaXLogCollector *sXLogCollector = (ulaXLogCollector *)aHandle;

    ACI_TEST_RAISE(sXLogCollector == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(ulaXLogCollectorSetReceiveXLogTimeout
                                                (sXLogCollector,
                                                 aSecond,
                                                 (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS,
                   ERR_INNER_API);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Handle");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}


/* Control API */
ALA_RC ALA_Handshake(ALA_Handle aHandle, ALA_ErrorMgr *aOutErrorMgr)
{
    ulaXLogCollector *sXLogCollector = (ulaXLogCollector *)aHandle;

    ACI_TEST_RAISE(sXLogCollector == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(ulaXLogCollectorHandshakeBefore(sXLogCollector,
                                                   (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS,
                   ERR_INNER_API);

    ACI_TEST_RAISE(ulaXLogCollectorHandshake(sXLogCollector,
                                             (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS,
                   ERR_INNER_API);

    (void)ulaLogTrace((ulaErrorMgr *)aOutErrorMgr,
                      ULA_TRC_I_XC_HANDSHAKE,
                      sXLogCollector->mXLogSenderName);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Handle");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_ReceiveXLog(ALA_Handle    aHandle,
                       ALA_BOOL     *aOutInsertXLogInQueue,
                       ALA_ErrorMgr *aOutErrorMgr)
{
    ulaXLogCollector *sXLogCollector = (ulaXLogCollector *)aHandle;

    ACI_TEST_RAISE(sXLogCollector == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(ulaXLogCollectorReceiveXLog
                                        (sXLogCollector,
                                         (acp_bool_t *)aOutInsertXLogInQueue,
                                         (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS,
                   ERR_INNER_API);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Handle");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_GetXLog(ALA_Handle       aHandle,
                   const ALA_XLog **aOutXLog,
                   ALA_ErrorMgr    *aOutErrorMgr)
{
    ulaXLogCollector *sXLogCollector = (ulaXLogCollector *)aHandle;

    ACI_TEST_RAISE(sXLogCollector == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(ulaXLogCollectorGetXLog(sXLogCollector,
                                           (ulaXLog **)aOutXLog,
                                           (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS,
                   ERR_INNER_API);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Handle");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_SendACK(ALA_Handle aHandle, ALA_ErrorMgr *aOutErrorMgr)
{
    ulaXLogCollector *sXLogCollector = (ulaXLogCollector *)aHandle;

    ACI_TEST_RAISE(sXLogCollector == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(ulaXLogCollectorSendACK(sXLogCollector,
                                           (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS,
                   ERR_INNER_API);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Handle");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_FreeXLog(ALA_Handle    aHandle,
                    ALA_XLog     *aXLog,
                    ALA_ErrorMgr *aOutErrorMgr)
{
    ulaXLogCollector *sXLogCollector = (ulaXLogCollector *)aHandle;

    ACI_TEST_RAISE(sXLogCollector == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(ulaXLogCollectorFreeXLogMemory(sXLogCollector,
                                                  (ulaXLog *)aXLog,
                                                  (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS,
                   ERR_INNER_API);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Handle");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}


/* Finish API */
ALA_RC ALA_DestroyXLogCollector(ALA_Handle aHandle, ALA_ErrorMgr *aOutErrorMgr)
{
    ulaXLogCollector *sXLogCollector = (ulaXLogCollector *)aHandle;

    ACI_TEST_RAISE(sXLogCollector == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(ulaXLogCollectorDestroy(sXLogCollector,
                                           (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS,
                   ERR_INNER_API);

    (void)ulaLogTrace((ulaErrorMgr *)aOutErrorMgr,
                      ULA_TRC_I_XC_DESTROY,
                      sXLogCollector->mXLogSenderName);

    acpMemFree(sXLogCollector);
    sXLogCollector = NULL;

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Handle");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    if (sXLogCollector != NULL)
    {
        acpMemFree(sXLogCollector);
    }

    return ALA_FAILURE;
}

ALA_RC ALA_DestroyAPI(ALA_BOOL      aUseAltibaseODBCDriver,
                      ALA_ErrorMgr *aOutErrorMgr)
{
    acp_bool_t sDestroyMT = ACP_FALSE;
    acp_bool_t sDestroyCM = ACP_FALSE;

    ulaCommDestroy();

    ACI_TEST_RAISE(mtcFinalizeForClient() != ACI_SUCCESS, ERR_DESTROY);
    sDestroyMT = ACP_TRUE;

    if (aUseAltibaseODBCDriver != ALA_TRUE)
    {
        ACI_TEST_RAISE(cmiFinalize() != ACI_SUCCESS, ERR_DESTROY);
        sDestroyCM = ACP_TRUE;
    }

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_DESTROY)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr, ulaERR_IGNORE_API_DESTROY);

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    if (sDestroyMT != ACP_TRUE)
    {
        mtcFinalizeForClient();
    }

    if (sDestroyCM != ACP_TRUE)
    {
        (void)cmiFinalize();
    }

    return ALA_FAILURE;
}

/* XLog API */
ALA_RC ALA_GetXLogHeader(const ALA_XLog        *aXLog,
                         const ALA_XLogHeader **aOutXLogHeader,
                         ALA_ErrorMgr          *aOutErrorMgr)
{
    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutXLogHeader == NULL, ERR_PARAMETER_NULL);

    *aOutXLogHeader = &(aXLog->mHeader);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "XLog Header");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_GetXLogPrimaryKey(const ALA_XLog            *aXLog,
                             const ALA_XLogPrimaryKey **aOutXLogPrimaryKey,
                             ALA_ErrorMgr              *aOutErrorMgr)
{
    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutXLogPrimaryKey == NULL, ERR_PARAMETER_NULL);

    *aOutXLogPrimaryKey = &(aXLog->mPrimaryKey);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "XLog Primary Key");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_GetXLogColumn(const ALA_XLog        *aXLog,
                         const ALA_XLogColumn **aOutXLogColumn,
                         ALA_ErrorMgr          *aOutErrorMgr)
{
    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutXLogColumn == NULL, ERR_PARAMETER_NULL);

    *aOutXLogColumn = &(aXLog->mColumn);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "XLog Column");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_GetXLogSavepoint(const ALA_XLog           *aXLog,
                            const ALA_XLogSavepoint **aOutXLogSavepoint,
                            ALA_ErrorMgr             *aOutErrorMgr)
{
    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutXLogSavepoint == NULL, ERR_PARAMETER_NULL);

    *aOutXLogSavepoint = &(aXLog->mSavepoint);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "XLog Savepoint");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_GetXLogLOB(const ALA_XLog     *aXLog,
                      const ALA_XLogLOB **aOutXLogLOB,
                      ALA_ErrorMgr       *aOutErrorMgr)
{
    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutXLogLOB == NULL, ERR_PARAMETER_NULL);

    *aOutXLogLOB = &(aXLog->mLOB);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "XLog LOB");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}


/* Meta Information API */
ALA_RC ALA_GetProtocolVersion(const ALA_ProtocolVersion *aOutProtocolVersion,
                              ALA_ErrorMgr              *aOutErrorMgr)
{
    ACI_TEST_RAISE(ulaMetaGetProtocolVersion
                            ((ulaProtocolVersion *)aOutProtocolVersion,
                             (ulaErrorMgr *)aOutErrorMgr)
                            != ACI_SUCCESS,
                            ERR_INNER_API);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_GetReplicationInfo(ALA_Handle              aHandle,
                              const ALA_Replication **aOutReplication,
                              ALA_ErrorMgr           *aOutErrorMgr)
{
    ulaMeta          *sMeta          = NULL;
    ulaXLogCollector *sXLogCollector = (ulaXLogCollector *)aHandle;

    ACI_TEST_RAISE(sXLogCollector == NULL, ERR_PARAMETER_NULL);

    sMeta = ulaXLogCollectorGetMeta(sXLogCollector);

    ACI_TEST_RAISE(ulaMetaGetReplicationInfo(sMeta,
                                             (ulaReplication **)aOutReplication,
                                             (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS,
                   ERR_INNER_API);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Handle");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_GetTableInfo(ALA_Handle        aHandle,
                        ULong             aTableOID,
                        const ALA_Table **aOutTable,
                        ALA_ErrorMgr     *aOutErrorMgr)
{
    ulaMeta          *sMeta          = NULL;
    ulaXLogCollector *sXLogCollector = (ulaXLogCollector *)aHandle;

    ACI_TEST_RAISE(sXLogCollector == NULL, ERR_PARAMETER_NULL);

    sMeta = ulaXLogCollectorGetMeta(sXLogCollector);

    ACI_TEST_RAISE(ulaMetaGetTableInfo(sMeta,
                                       aTableOID,
                                       (ulaTable **)aOutTable,
                                       (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS,
                   ERR_INNER_API);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Handle");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_GetTableInfoByName(ALA_Handle        aHandle,
                              const SChar      *aFromUserName,
                              const SChar      *aFromTableName,
                              const ALA_Table **aOutTable,
                              ALA_ErrorMgr     *aOutErrorMgr)
{
    ulaMeta          *sMeta          = NULL;
    ulaXLogCollector *sXLogCollector = (ulaXLogCollector *)aHandle;

    ACI_TEST_RAISE(sXLogCollector == NULL, ERR_PARAMETER_NULL);

    sMeta = ulaXLogCollectorGetMeta(sXLogCollector);

    ACI_TEST_RAISE(ulaMetaGetTableInfoByName(sMeta,
                                             (acp_char_t*)aFromUserName,
                                             (acp_char_t*)aFromTableName,
                                             (ulaTable **)aOutTable,
                                             (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS,
                   ERR_INNER_API);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Handle");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_GetColumnInfo(const ALA_Table   *aTable,
                         UInt               aColumnID,
                         const ALA_Column **aOutColumn,
                         ALA_ErrorMgr      *aOutErrorMgr)
{
    ACI_TEST_RAISE(ulaMetaGetColumnInfo((ulaTable *)aTable,
                                        aColumnID,
                                        (ulaColumn **)aOutColumn,
                                        (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS,
                   ERR_INNER_API);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_GetIndexInfo(const ALA_Table  *aTable,
                        UInt              aIndexID,
                        const ALA_Index **aOutIndex,
                        ALA_ErrorMgr     *aOutErrorMgr)
{
    ACI_TEST_RAISE(ulaMetaGetIndexInfo((ulaTable *)aTable,
                                       aIndexID,
                                       (ulaIndex **)aOutIndex,
                                       (ulaErrorMgr *)aOutErrorMgr)
                   != ACI_SUCCESS,
                   ERR_INNER_API);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}


/* Monitoring */
ALA_RC ALA_GetXLogCollectorStatus
                    (ALA_Handle               aHandle,
                     ALA_XLogCollectorStatus *aOutXLogCollectorStatus,
                     ALA_ErrorMgr            *aOutErrorMgr)
{
    ulaXLogCollector *sXLogCollector = (ulaXLogCollector *)aHandle;

    ACI_TEST_RAISE(sXLogCollector == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(ulaXLogCollectorGetXLogCollectorStatus
                            (sXLogCollector,
                             (ulaXLogCollectorStatus *)aOutXLogCollectorStatus,
                             (ulaErrorMgr *)aOutErrorMgr)
                            != ACI_SUCCESS,
                            ERR_INNER_API);

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Handle");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_INNER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

/* ulnDataBuildColumnFromMT함수를 복사해온 것으로 uln과 ula두 함수의 차이는
   endian assign이 아닌 memcpy로 처리된다는 점입니다.
   ala의 경우 Endian처리가 recvXLog하며 이미 되어있어 endian assign이 필요 없습니다.
*/
ACI_RC ulaDataBuildColumnFromMT( ulnFnContext * aFnContext,
                                 acp_uint8_t  * aSrc,
                                 ulnColumn    * aColumnValue )
{
    acp_uint64_t sLen64;
    acp_uint32_t sLen32;
    acp_uint16_t sLen16;
    acp_uint8_t  sLen8;
    ulnLob     * sLob;

    switch ( aColumnValue->mMtype )
    {
        case ULN_MTYPE_NULL :
            *( aColumnValue->mBuffer + 0 ) = *( aSrc + 0 );

            aColumnValue->mDataLength = SQL_NULL_DATA;;
            aColumnValue->mMTLength   = 1;
            aColumnValue->mPrecision  = 0;
            break;

        case ULN_MTYPE_BINARY :
            acpMemCpy( &sLen32, aSrc,  ACI_SIZEOF(acp_uint32_t) );

            aColumnValue->mBuffer     = aSrc + 8;
            aColumnValue->mDataLength = sLen32;
            aColumnValue->mMTLength   = aColumnValue->mDataLength + 8;
            aColumnValue->mPrecision  = 0;

            if ( aColumnValue->mDataLength == 0 )
            {
                aColumnValue->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_CHAR :
        case ULN_MTYPE_VARCHAR :
        case ULN_MTYPE_NCHAR :
        case ULN_MTYPE_NVARCHAR :
            acpMemCpy( &sLen16, aSrc,  ACI_SIZEOF(acp_uint16_t) );

            aColumnValue->mBuffer     = aSrc + 2;
            aColumnValue->mDataLength = sLen16;
            aColumnValue->mMTLength   = aColumnValue->mDataLength + 2;
            aColumnValue->mPrecision  = 0;

            if ( aColumnValue->mDataLength == 0 )
            {
                aColumnValue->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_FLOAT :
        case ULN_MTYPE_NUMERIC :

            sLen8 = *((acp_uint8_t*)aSrc);

            if ( sLen8 != 0 )
            {
                ACI_TEST( ulncMtNumericToCmNumeric( (cmtNumeric*)aColumnValue->mBuffer,
                                                    (mtdNumericType*)aSrc )
                          != ACI_SUCCESS );

                aColumnValue->mDataLength = ACI_SIZEOF(cmtNumeric);
                aColumnValue->mMTLength   = sLen8 + 1;
                aColumnValue->mPrecision  = 0;
            }
            else
            {
                aColumnValue->mDataLength = SQL_NULL_DATA;
                aColumnValue->mMTLength   = 1;
                aColumnValue->mPrecision  = 0;
            }
            break;

        case ULN_MTYPE_SMALLINT :
            acpMemCpy( aColumnValue->mBuffer, aSrc, ACI_SIZEOF(acp_uint16_t) );

            aColumnValue->mDataLength = 2;
            aColumnValue->mMTLength   = 2;
            aColumnValue->mPrecision  = 0;

            if (*((acp_sint16_t*)aColumnValue->mBuffer) == MTD_SMALLINT_NULL )
            {
                aColumnValue->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_INTEGER :
            acpMemCpy( aColumnValue->mBuffer, aSrc, ACI_SIZEOF(acp_uint32_t) );

            aColumnValue->mDataLength = 4;
            aColumnValue->mMTLength   = 4;
            aColumnValue->mPrecision  = 0;

            if (*((acp_sint32_t*)aColumnValue->mBuffer) == MTD_INTEGER_NULL )
            {
                aColumnValue->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_BIGINT :
            acpMemCpy( aColumnValue->mBuffer, aSrc, ACI_SIZEOF(acp_uint64_t) );

            aColumnValue->mDataLength = 8;
            aColumnValue->mMTLength   = 8;
            aColumnValue->mPrecision  = 0;

            if (*((acp_sint64_t*)aColumnValue->mBuffer) == MTD_BIGINT_NULL )
            {
                aColumnValue->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_REAL :
            acpMemCpy( aColumnValue->mBuffer, aSrc, ACI_SIZEOF(acp_uint32_t) );

            aColumnValue->mDataLength = 4;
            aColumnValue->mMTLength   = 4;
            aColumnValue->mPrecision  = 0;

            if ( ( *(acp_uint32_t*)(aColumnValue->mBuffer) & MTD_REAL_EXPONENT_MASK )
                 == MTD_REAL_EXPONENT_MASK )
            {
                aColumnValue->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_DOUBLE :
            acpMemCpy( aColumnValue->mBuffer, aSrc, ACI_SIZEOF(acp_uint64_t) );

            aColumnValue->mDataLength = 8;
            aColumnValue->mMTLength   = 8;
            aColumnValue->mPrecision  = 0;

            if ( ( *(acp_uint64_t*)(aColumnValue->mBuffer) & MTD_DOUBLE_EXPONENT_MASK )
                 == MTD_DOUBLE_EXPONENT_MASK )
            {
                aColumnValue->mDataLength = SQL_NULL_DATA;
            }
            break;

            /* LOB_LOCATOR + lobsize */
        case ULN_MTYPE_BLOB :
        case ULN_MTYPE_CLOB :
        case ULN_MTYPE_BLOB_LOCATOR :
        case ULN_MTYPE_CLOB_LOCATOR :
            sLob = (ulnLob *)aColumnValue->mBuffer;
            ulnLobInitialize( sLob, (ulnMTypeID)aColumnValue->mMtype );

            acpMemCpy( &sLen64, aSrc, ACI_SIZEOF(acp_uint64_t) );

            sLob->mOp->mSetLocator( aFnContext, sLob, sLen64 );

            aColumnValue->mDataLength = ACI_SIZEOF(ulnLob);
            aColumnValue->mMTLength   = 12;
            aColumnValue->mPrecision  = 0;

            if ( sLen64 == MTD_LOCATOR_NULL )
            {
                aColumnValue->mDataLength = SQL_NULL_DATA;
            }

            acpMemCpy( &sLen32, aSrc+8, ACI_SIZEOF(acp_uint32_t) );

            sLob->mSize = sLen32;
            break;

        case ULN_MTYPE_TIMESTAMP :
            acpMemCpy( aColumnValue->mBuffer, aSrc, ACI_SIZEOF(acp_uint16_t) );
            acpMemCpy( aColumnValue->mBuffer+2, aSrc+2, ACI_SIZEOF(acp_uint16_t) );
            acpMemCpy( aColumnValue->mBuffer+4, aSrc+4, ACI_SIZEOF(acp_uint32_t) );

            aColumnValue->mDataLength = 8;
            aColumnValue->mMTLength   = 8;
            aColumnValue->mPrecision  = 0;

            if ( MTD_DATE_IS_NULL( (mtdDateType*)aColumnValue->mBuffer ) )
            {
                aColumnValue->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_INTERVAL :
            acpMemCpy( aColumnValue->mBuffer, aSrc, ACI_SIZEOF(acp_uint64_t) );
            acpMemCpy( aColumnValue->mBuffer+8, aSrc+8, ACI_SIZEOF(acp_uint64_t) );

            aColumnValue->mDataLength = 16;
            aColumnValue->mMTLength   = 16;
            aColumnValue->mPrecision  = 0;

            if ( MTD_INTERVAL_IS_NULL( (mtdIntervalType*)aColumnValue->mBuffer ) )
            {
                aColumnValue->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_BIT :
        case ULN_MTYPE_VARBIT :
            acpMemCpy( &sLen32, aSrc, ACI_SIZEOF(acp_uint32_t) );

            aColumnValue->mBuffer     = aSrc + 4;
            aColumnValue->mDataLength = BIT_TO_BYTE( sLen32 );
            aColumnValue->mMTLength   = (aColumnValue->mDataLength) + 4;
            aColumnValue->mPrecision  = sLen32;

            if ( aColumnValue->mDataLength == 0)
            {
                aColumnValue->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_NIBBLE :
            sLen8 = *( aSrc + 0 );

            if ( sLen8 == MTD_NIBBLE_NULL_LENGTH )
            {
                aColumnValue->mBuffer     = aSrc;
                aColumnValue->mDataLength = SQL_NULL_DATA;
                aColumnValue->mMTLength   = 1;
                aColumnValue->mPrecision  = 0;
            }
            else
            {
                aColumnValue->mBuffer     = aSrc + 1;
                aColumnValue->mDataLength = ( sLen8 + 1 ) >> 1;
                aColumnValue->mMTLength   = aColumnValue->mDataLength + 1;
                aColumnValue->mPrecision  = sLen8;
            }
            break;

        case ULN_MTYPE_BYTE :
        case ULN_MTYPE_VARBYTE :
            acpMemCpy( &sLen16, aSrc, ACI_SIZEOF(acp_uint16_t) );

            aColumnValue->mBuffer     = aSrc + 2;
            aColumnValue->mDataLength = sLen16;
            aColumnValue->mMTLength   = aColumnValue->mDataLength + 2;
            aColumnValue->mPrecision  = 0;

            if ( sLen16 == 0 )
            {
                aColumnValue->mDataLength = SQL_NULL_DATA;
            }
            break;

        default :
            ACE_ASSERT( 0 );
            break;
    }

    return ALA_SUCCESS;

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

/* Data Type Conversion API
 *
 * PROJ-2160 CM타입제거로 인해 MT타입=>ODBC변환이 가능하다.
 * PROJ-2160 이전 : MT -> CMT -> ulnColumn -> ODBC
 * PROJ-2160 이후 : MT -> ulnColumn -> ODBC
 */
ALA_RC ALA_GetODBCCValue(ALA_Column   *aColumn,
                         ALA_Value    *aAltibaseValue,
                         SInt          aODBCCTypeID,
                         UInt          aODBCCValueBufferSize,
                         void         *aOutODBCCValueBuffer,
                         ALA_BOOL     *aOutIsNull,
                         UInt         *aOutODBCCValueSize,
                         ALA_ErrorMgr *aOutErrorMgr)
{
    acp_rc_t         sRc;
    cmtAny           sCMTValue;          // CMT
    acp_bool_t       sIsCMTInit        = ACP_FALSE;

    ulnColumn        sColumnValue;       // ulnColumn
    acp_uint32_t     sColumnBufferLength;
    acp_bool_t       sIsColumnInit     = ACP_FALSE;

    ulnFnContext     sFnContext;
    ulnStmt          sStmt;              // CMT->ODBC 변환 시의 예외 메시지 포함
    ulnDbc           sDbc;               // DATE Format 포함
    acp_bool_t       sIsDiagHeaderInit = ACP_FALSE;

    ulnAppBuffer     sUserBuffer;        // ODBC
    ulnIndLenPtrPair sIndLenPtrPair;
    ulvSLen          sPairIndicator;
    ulvSLen          sPairLength;

    //BUG-22342
    acp_char_t      *sNlsUse;
    acp_char_t       sDefaultNLS[]     = "US7ASCII";
    //BUG-26058 : ALA NCHAR
    acp_char_t      *sAlaNcharSet;
    acp_char_t       sDefaultAlaNcharSet[] = "UTF16";

    //BUG-24226
    acp_bool_t       sIsAlloc           = ACP_FALSE; //메모리가 할당 되었는지

    ACI_TEST_RAISE(aColumn == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aAltibaseValue == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutODBCCValueBuffer == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutIsNull == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutODBCCValueSize == NULL, ERR_PARAMETER_NULL);

    // 지원하지 않는 타입을 검사합니다.
    ACI_TEST_RAISE((aColumn->mDataType == MTD_BLOB_ID)     ||
                   (aColumn->mDataType == MTD_CLOB_ID)     ||
                   (aColumn->mDataType == MTD_GEOMETRY_ID),
                   ERR_PARAMETER_INVALID);

    ACI_TEST_RAISE((aODBCCTypeID == SQL_C_BLOB_LOCATOR) ||
                   (aODBCCTypeID == SQL_C_CLOB_LOCATOR) ||
                   (aODBCCTypeID == SQL_C_FILE),
                   ERR_PARAMETER_INVALID);

    *aOutODBCCValueSize = 0;

    // Null Value인지 검사합니다.
    if (aAltibaseValue->value != NULL)
    {
        *aOutIsNull = ALA_FALSE;
    }
    else
    {
        *aOutIsNull = ALA_TRUE;

        goto exit_success;
    }

    /* [MT -> CMT] 변환 : 이 작업은 PROJ-2160에 의해 무의미해졌으나 sColumnBufferLength를 구하기 위해 남겨둡니다. */

    // CMT를 초기화합니다.
    ACI_TEST_RAISE(cmtAnyInitialize(&sCMTValue) != ACI_SUCCESS,
                   ERR_CMT_INITIALIZE);
    sIsCMTInit = ACP_TRUE;

    // MT를 CMT로 변환합니다. (MTD_XXXX_ID -> CMT_ID_XXXX)
    // Numeric에 적용될 Byte Order를 설정합니다. ODBC는 Little Endian입니다.
    ACI_TEST_RAISE(ulaConvConvertFromMTToCMT(&sCMTValue,
                                             (void *)aAltibaseValue->value,
                                             aColumn->mDataType,
                                             ULA_BYTEORDER_LITTLE_ENDIAN,
                                             0) // bug-19174
                   != ACI_SUCCESS, ERR_MT_TO_CMT_CONVERT);

    /* [CMT -> ODBC] */
    // ulnColumn을 초기화합니다.

    switch (sCMTValue.mType)
    {
        case CMT_ID_BIT :
            sColumnBufferLength = (sCMTValue.mValue.mBit.mPrecision + 7) / 8
                + ACI_SIZEOF(acp_uint32_t);
            break;

        case CMT_ID_NIBBLE :
            sColumnBufferLength = (sCMTValue.mValue.mNibble.mPrecision + 1) / 2
                + ACI_SIZEOF(acp_uint8_t);
            break;

        case CMT_ID_VARIABLE :
        case CMT_ID_BINARY :
            sColumnBufferLength = 
                cmtVariableGetSize(&(sCMTValue.mValue.mVariable));
            break;

            // PROJ-1697 [효율성] 2가지 통신 프로토콜 개선 (A4를 이겨라)
        case CMT_ID_IN_BIT :
        case CMT_ID_IN_NIBBLE :
        case CMT_ID_IN_VARIABLE :
        case CMT_ID_IN_BINARY :
            ACE_ASSERT(0);

        default :
            sColumnBufferLength = 64;   // 다른 타입은 64바이트 미만입니다.
            break;
    }

    acpMemSet( &sColumnValue, 0x00, ACI_SIZEOF( ulnColumn ) );
    sColumnValue.mMtype =
        (acp_uint16_t)ulnTypeMap_MTD_MTYPE(aColumn->mDataType);
    ACI_TEST_RAISE(sColumnValue.mMtype == ULN_MTYPE_MAX, ERR_PARAMETER_INVALID);

    // Dummy ulnFnContext를 생성합니다.
    acpMemSet(&sFnContext, 0x00, ACI_SIZEOF(ulnFnContext));
    acpMemSet(&sStmt,      0x00, ACI_SIZEOF(ulnStmt));
    acpMemSet(&sDbc,       0x00, ACI_SIZEOF(ulnDbc));

    sFnContext.mHandle.mStmt = &sStmt;
    sStmt.mParentDbc         = &sDbc;
    //BUG-22342
    sFnContext.mObjType = ULN_OBJ_TYPE_STMT;

    sRc = acpEnvGet("ALTIBASE_NLS_USE", &sNlsUse);
    if (ACP_RC_IS_ENOENT(sRc))
    {
        sNlsUse = sDefaultNLS;
    }
    else
    {
        ACE_ASSERT(ACP_RC_IS_SUCCESS(sRc));
    }

    //BUG-26058 : ALA NCHAR
    sRc = acpEnvGet("ALTIBASE_ALA_NCHARSET", &sAlaNcharSet);
    if (ACP_RC_IS_ENOENT(sRc))
    {
        sAlaNcharSet = sDefaultAlaNcharSet;
    }
    else
    {
        ACE_ASSERT(ACP_RC_IS_SUCCESS(sRc));
    }

    //내부적으로 malloc함 두번째 파라메터를 NULL 넣어 해제 해야 함
    ACI_TEST_RAISE(ulnDbcSetNlsCharsetString(&sDbc, sNlsUse, acpCStrLen(sNlsUse, ACP_SINT32_MAX))
                   != ACI_SUCCESS , ALA_ERR_MEM_ALLOC) ;

    ACI_TEST_RAISE(ulnDbcSetNlsNcharCharsetString(&sDbc, sAlaNcharSet, acpCStrLen(sAlaNcharSet, ACP_SINT32_MAX))
                   != ACI_SUCCESS , ALA_ERR_MEM_ALLOC) ;

    // Memory 할당이 되었는지 확인
    sIsAlloc = ACP_TRUE;

    ACE_ASSERT(sDbc.mNlsCharsetString != NULL);
    ACE_ASSERT(sDbc.mNlsNcharCharsetString != NULL);

    ACI_TEST_RAISE(mtlModuleByName
                   ((const mtlModule **)&(sDbc.mCharsetLangModule),
                    sDbc.mNlsCharsetString,
                    acpCStrLen(sDbc.mNlsCharsetString, 30))
                   != ACI_SUCCESS, ERR_PARAMETER_INVALID);

    ACI_TEST_RAISE(mtlModuleByName
                   ((const mtlModule **)&(sDbc.mNcharCharsetLangModule),
                    sDbc.mNlsNcharCharsetString,
                    acpCStrLen(sDbc.mNlsNcharCharsetString, 30))
                   != ACI_SUCCESS, ERR_PARAMETER_INVALID);

    //mClientCharsetLangModule
    ACI_TEST_RAISE(mtlModuleByName
                   ((const mtlModule **)&(sDbc.mClientCharsetLangModule),
                    sDbc.mNlsCharsetString,
                    acpCStrLen(sDbc.mNlsCharsetString, 30))
                   != ACI_SUCCESS, ERR_PARAMETER_INVALID);

    sDbc.mDateFormat = (acp_char_t *)ALA_DEFAULT_DATE_FORMAT;

    ACI_TEST_RAISE(ulnCreateDiagHeader(&(sStmt.mObj), NULL)
                   != ACI_SUCCESS, ERR_DIAG_HEADER_CREATE);
    sIsDiagHeaderInit = ACP_TRUE;

    /* 아래의 타입은 memcopy가 발생하며, 그 외의 타입은 그냥 주소값을 가진다. */
    switch ( (&sColumnValue)->mMtype )
    {
        case ULN_MTYPE_FLOAT :
        case ULN_MTYPE_NUMERIC :
        case ULN_MTYPE_INTEGER :
        case ULN_MTYPE_SMALLINT :
        case ULN_MTYPE_BIGINT :
        case ULN_MTYPE_REAL :
        case ULN_MTYPE_DOUBLE :
        case ULN_MTYPE_BLOB :
        case ULN_MTYPE_CLOB :
        case ULN_MTYPE_BLOB_LOCATOR :
        case ULN_MTYPE_CLOB_LOCATOR :
        case ULN_MTYPE_TIMESTAMP :
        case ULN_MTYPE_INTERVAL :
            sRc = acpMemAlloc( (void **)&sColumnValue.mBuffer, sColumnBufferLength );
            ACI_TEST_RAISE( ACP_RC_NOT_SUCCESS(sRc), ALA_ERR_MEM_ALLOC );
            sIsColumnInit = ACP_TRUE;
            break;
        default:
            break;
    }

    ACI_TEST_RAISE( ulaDataBuildColumnFromMT( &sFnContext,
                                              (acp_uint8_t *)aAltibaseValue->value,
                                              &sColumnValue )
                   != ACI_SUCCESS, ERR_CMT_TO_COLUMN_COPY );

    // 사용자 버퍼를 설정합니다.
    acpMemSet(&sUserBuffer, 0x00, ACI_SIZEOF(ulnAppBuffer));
    sUserBuffer.mCTYPE = ulnTypeMap_SQLC_CTYPE((acp_sint16_t)aODBCCTypeID);
    ACI_TEST_RAISE(sUserBuffer.mCTYPE == ULN_CTYPE_MAX,
                   ERR_PARAMETER_INVALID);
    sUserBuffer.mBuffer       = (acp_uint8_t *)aOutODBCCValueBuffer;
    sUserBuffer.mBufferSize   = (ulvULen)aODBCCValueBufferSize;
    sUserBuffer.mColumnStatus = ULN_ROW_SUCCESS;

    // 기타 정보를 초기화합니다.
    sPairIndicator = 0;
    sPairLength    = 0;
    sIndLenPtrPair.mIndicatorPtr = &sPairIndicator;
    sIndLenPtrPair.mLengthPtr    = &sPairLength;

    // ulnColumn을 ODBC로 변환합니다. (CMT_ID_XXXX -> SQL_C_XXXX)
    ACI_TEST_RAISE(ulnConvert(&sFnContext,
                              &sUserBuffer,
                              &sColumnValue,
                              0,
                              &sIndLenPtrPair)
                   != ACI_SUCCESS, ERR_COLUMN_TO_ODBC_CONVERT);

    //BUG-22342
    //내부적으로 malloc함 두번째 파라메터를 NULL 넣어 해제 해야 함
    (void)ulnDbcSetNlsCharsetString(&sDbc, NULL, 0);

    (void)ulnDbcSetNlsNcharCharsetString(&sDbc, NULL, 0);

    ACE_DASSERT(sPairIndicator != SQL_NULL_DATA);
    if (sPairIndicator == SQL_NULL_DATA)
    {
        *aOutIsNull = ALA_TRUE;
    }
    *aOutODBCCValueSize = (acp_uint32_t) sPairLength;
    ACI_TEST_RAISE(sUserBuffer.mColumnStatus != ULN_ROW_SUCCESS,
                   ERR_COLUMN_TO_ODBC_CONVERT);

    // 메모리를 정리합니다.
    sIsDiagHeaderInit = ACP_FALSE;
    ACI_TEST_RAISE(ulnDestroyDiagHeader(&(sStmt.mObj.mDiagHeader),
                                        ULN_DIAG_HDR_DESTROY_CHUNKPOOL)
                   != ACI_SUCCESS, ERR_DIAG_HEADER_DESTROY);

    if ( sIsColumnInit == ACP_TRUE )
    {
        acpMemFree(sColumnValue.mBuffer);
    }

    sIsCMTInit = ACP_FALSE;
    ACI_TEST_RAISE(cmtAnyFinalize(&sCMTValue) != ACI_SUCCESS,
                   ERR_CMT_FINALIZE);

  exit_success :

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *) aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "ODBC C Value");

        (void)ulaLogError((ulaErrorMgr *) aOutErrorMgr);
    }
    ACI_EXCEPTION(ERR_PARAMETER_INVALID)
    {
        ulaSetErrorCode((ulaErrorMgr *) aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_INVALID,
                        "ODBC C Value");

        (void)ulaLogError((ulaErrorMgr *) aOutErrorMgr);
    }
    ACI_EXCEPTION(ALA_ERR_MEM_ALLOC)
    {
        ulaSetErrorCode((ulaErrorMgr *) aOutErrorMgr,
                        ulaERR_ABORT_MEMORY_ALLOC);

        (void)ulaLogError((ulaErrorMgr *) aOutErrorMgr);
    }
    ACI_EXCEPTION(ERR_CMT_INITIALIZE)
    {
        ulaSetErrorCode((ulaErrorMgr *) aOutErrorMgr,
                        ulaERR_IGNORE_CMT_INITIALIZE);

        (void)ulaLogError((ulaErrorMgr *) aOutErrorMgr);
    }
    ACI_EXCEPTION(ERR_CMT_FINALIZE)
    {
        ulaSetErrorCode((ulaErrorMgr *) aOutErrorMgr,
                        ulaERR_IGNORE_CMT_FINALIZE);

        (void)ulaLogError((ulaErrorMgr *) aOutErrorMgr);
    }
    ACI_EXCEPTION(ERR_DIAG_HEADER_CREATE)
    {
        ulaSetErrorCode((ulaErrorMgr *) aOutErrorMgr,
                        ulaERR_IGNORE_DIAG_HEADER_CREATE);

        (void)ulaLogError((ulaErrorMgr *) aOutErrorMgr);
    }
    ACI_EXCEPTION(ERR_DIAG_HEADER_DESTROY)
    {
        ulaSetErrorCode((ulaErrorMgr *) aOutErrorMgr,
                        ulaERR_IGNORE_DIAG_HEADER_DESTROY);

        (void)ulaLogError((ulaErrorMgr *) aOutErrorMgr);
    }
    ACI_EXCEPTION(ERR_MT_TO_CMT_CONVERT)
    {
        ulaSetErrorCode((ulaErrorMgr *) aOutErrorMgr,
                        ulaERR_IGNORE_MT_TO_CMT_CONVERT);

        (void)ulaLogError((ulaErrorMgr *) aOutErrorMgr);
    }
    ACI_EXCEPTION( ERR_CMT_TO_COLUMN_COPY )
    {
        ulaSetErrorCode( (ulaErrorMgr *)aOutErrorMgr,
                         ulaERR_IGNORE_CMT_TO_COLUMN_COPY );

        (void)ulaLogError( (ulaErrorMgr *)aOutErrorMgr );
    }
    ACI_EXCEPTION(ERR_COLUMN_TO_ODBC_CONVERT)
    {
        ulaSetErrorCode((ulaErrorMgr *) aOutErrorMgr,
                        ulaERR_IGNORE_COLUMN_TO_ODBC_CONVERT);

        (void)ulaLogError((ulaErrorMgr *) aOutErrorMgr);
    }
    ACI_EXCEPTION_END;

    //BUG-22342
    //내부적으로 malloc함 두번째 파라메터를 NULL 넣어 해제 해야 함
    if (sIsAlloc != ACP_FALSE)
    {
        (void)ulnDbcSetNlsCharsetString(&sDbc, NULL, 0);
    }
    // 메모리를 정리합니다.
    if (sIsDiagHeaderInit != ACP_FALSE)
    {
        (void)ulnDestroyDiagHeader(&(sStmt.mObj.mDiagHeader),
                                   ULN_DIAG_HDR_DESTROY_CHUNKPOOL);
    }
    if ( sIsColumnInit == ACP_TRUE )
    {
        acpMemFree(sColumnValue.mBuffer);
    }
    if (sIsCMTInit != ACP_FALSE)
    {
        (void)cmtAnyFinalize(&sCMTValue);
    }

    return ALA_FAILURE;
}

ALA_RC ALA_GetInternalNumericInfo(ALA_Column   *aColumn,
                                  ALA_Value    *aAltibaseValue,
                                  SInt         *aOutSign,
                                  SInt         *aOutExponent,
                                  ALA_ErrorMgr *aOutErrorMgr)
{
    mtdNumericType *sNumeric;

    ACI_TEST_RAISE(aColumn == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aAltibaseValue == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutSign == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutExponent == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE((aColumn->mDataType != MTD_FLOAT_ID) &&
                   (aColumn->mDataType != MTD_NUMERIC_ID),
                   ERR_PARAMETER_INVALID);

    sNumeric = (mtdNumericType *)aAltibaseValue->value;

    if ((sNumeric->signExponent & 0x80) != 0)
    {   // 양수
        *aOutSign = 1;

        *aOutExponent = ((acp_sint32_t)(sNumeric->signExponent & 0x7F) - 64) * 2
                      + ((sNumeric->mantissa[0] < 10) ? -1 : 0);
    }
    else
    {   // 음수
        *aOutSign = 0;

        *aOutExponent = (64 - (acp_sint32_t)(sNumeric->signExponent & 0x7F)) * 2
                      + ((sNumeric->mantissa[0] >= 90) ? -1 : 0);
    }

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Internal Numeric Info");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_PARAMETER_INVALID)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_INVALID,
                        "Internal Numeric Info");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

// MT Module을 사용합니다.
ALA_RC ALA_GetAltibaseText(ALA_Column   *aColumn,
                           ALA_Value    *aValue,
                           UInt          aBufferSize,
                           SChar        *aOutBuffer,
                           ALA_ErrorMgr *aOutErrorMgr)
{
    acp_rc_t         sRc;
    const mtdModule *sMtd = NULL;
    ACI_RC           sEncodeResult;

    mtdCharType     *sChar;
    mtdByteType     *sByte;
    mtdNibbleType   *sNibble;
    mtdBitType      *sBit;

    acp_uint32_t     sTextLength;
    acp_uint32_t     sIndex;
    acp_uint32_t     sFence;
    acp_uint32_t     sBitIndex;
    acp_uint32_t     sBitPos;
    acp_char_t       sTargetChar;

    //BUG-26058 : ALA NCHAR
    acp_uint32_t     sOffset;
    acp_char_t      *sTempBuffer = NULL;
    acp_uint32_t     sTempBufferLen;
    acp_char_t      *sAlaNcharSet = NULL;
    acp_char_t       sDefaultAlaNcharSet[] = "UTF16";
    mtlModule       *sSrcCharSet;
    mtlModule       *sDestCharSet;

    // BUG-22609 AIX 최적화 오류 수정
    // switch 에 acp_uint32_t 형으로 음수값이 2번이상올때 서버 죽음
    acp_sint32_t     sType;//BUG-22816

    ACI_TEST_RAISE(aColumn == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aValue == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutBuffer == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(aBufferSize < 1, ERR_PARAMETER_INVALID);
    aOutBuffer[0] = '\0';


    //BUG-26058 : ALA NCHAR
    sRc = acpEnvGet("ALTIBASE_ALA_NCHARSET", &sAlaNcharSet);

    if ( ACP_RC_NOT_SUCCESS(sRc))
    {
        sAlaNcharSet = sDefaultAlaNcharSet;
    }

    if (aValue->value != NULL)
    {
        sType = (acp_sint32_t)aColumn->mDataType; //BUG-22816

        switch (sType)
        {
            case MTD_FLOAT_ID :
            case MTD_NUMERIC_ID :
            case MTD_DOUBLE_ID :
            case MTD_REAL_ID :
            case MTD_BIGINT_ID :
            case MTD_INTEGER_ID :
            case MTD_SMALLINT_ID :
                ACI_TEST_RAISE(aBufferSize < 49, ERR_PARAMETER_INVALID);

                ACI_TEST_RAISE(mtdModuleById(&sMtd, aColumn->mDataType)
                               != ACI_SUCCESS,
                               ERR_MODULE_GET);

                // BUG-18550
                if ((*sMtd->isNull)(NULL,
                                    (const void *)aValue->value,
                                    MTD_OFFSET_USELESS) != ACP_TRUE)
                {
                    ACI_TEST_RAISE((*sMtd->encode)(NULL,
                                                   (void *)aValue->value,
                                                   aValue->length,
                                                   NULL,
                                                   0,
                                                   (acp_uint8_t *)aOutBuffer,
                                                   (acp_uint32_t*)&aBufferSize,
                                                   &sEncodeResult)
                                   != ACI_SUCCESS,
                                   ERR_ENCODE);
                    ACI_TEST_RAISE(sEncodeResult != ACI_SUCCESS, ERR_ENCODE);
                }
                else
                {
                    acpMemCpy(aOutBuffer, (acp_char_t *)"NULL", 5);
                }

                break;

            case MTD_DATE_ID :
                ACI_TEST_RAISE(aBufferSize < ALA_DEFAULT_DATE_FORMAT_LEN + 1,
                               ERR_PARAMETER_INVALID);

                ACI_TEST_RAISE(mtdModuleById(&sMtd, aColumn->mDataType)
                               != ACI_SUCCESS,
                               ERR_MODULE_GET);

                // BUG-18550
                if ((*sMtd->isNull)(NULL,
                                    (const void *)aValue->value,
                                    MTD_OFFSET_USELESS) != ACP_TRUE)
                {
                    ACI_TEST_RAISE((*sMtd->encode)(NULL,
                                                   (void *)aValue->value,
                                                   aValue->length,
                                                   (acp_uint8_t *)
                                                        ALA_MTD_DATE_FORMAT,
                                                   ALA_MTD_DATE_FORMAT_LEN,
                                                   (acp_uint8_t *)aOutBuffer,
                                                   (acp_uint32_t*)&aBufferSize,
                                                   &sEncodeResult)
                                   != ACI_SUCCESS,
                                   ERR_ENCODE);
                    ACI_TEST_RAISE(sEncodeResult != ACI_SUCCESS, ERR_ENCODE);
                }
                else
                {
                    acpMemCpy(aOutBuffer, (acp_char_t *)"NULL", 5);
                }
                break;

            case MTD_CHAR_ID :
            case MTD_VARCHAR_ID :
                sChar = (mtdCharType *)aValue->value;
                ACI_TEST_RAISE(aBufferSize < (acp_uint32_t)sChar->length + 1,
                               ERR_PARAMETER_INVALID);

                ACI_TEST_RAISE(mtdModuleById(&sMtd, aColumn->mDataType)
                               != ACI_SUCCESS,
                               ERR_MODULE_GET);

                ACI_TEST_RAISE((*sMtd->encode)(NULL,
                                               (void *)aValue->value,
                                               (acp_uint32_t)sChar->length,
                                               NULL,
                                               0,
                                               (acp_uint8_t *)aOutBuffer,
                                               (acp_uint32_t*)&aBufferSize,
                                               &sEncodeResult) != ACI_SUCCESS,
                               ERR_ENCODE);
                ACI_TEST_RAISE(sEncodeResult != ACI_SUCCESS, ERR_ENCODE);
                break;

            //BUG-26058 : ALA NCHAR
            case MTD_NCHAR_ID :
            case MTD_NVARCHAR_ID :
                ACI_TEST_RAISE(mtlModuleByName((const mtlModule **)&sSrcCharSet,
                                               sAlaNcharSet,
                                               acpCStrLen(sAlaNcharSet, 10))
                               != ACI_SUCCESS,
                               ERR_PARAMETER_INVALID);

                //utf16으로 변환 한다.
                ACI_TEST_RAISE(mtlModuleByName
                                        ((const mtlModule **)&sDestCharSet,
                                         sDefaultAlaNcharSet,
                                         acpCStrLen(sDefaultAlaNcharSet, 10))
                               != ACI_SUCCESS,
                               ERR_PARAMETER_INVALID);

                sRc = acpMemAlloc((void **)&sTempBuffer, aBufferSize);
                ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ALA_ERR_MEM_ALLOC);

                ACI_TEST_RAISE(ulaConvertCharSet
                               ((const mtlModule *)sSrcCharSet,
                                (const mtlModule *)sDestCharSet,
                                (mtdNcharType *)aValue->value,
                                (UChar *)sTempBuffer,
                                (UInt *)&sTempBufferLen)
                               != ACI_SUCCESS,
                               ERR_PARAMETER_INVALID);

                sTextLength = (acp_uint32_t)sTempBufferLen * 2 +
                                        ((acp_uint32_t)sTempBufferLen / 2) ;
                ACI_TEST_RAISE(aBufferSize < (sTextLength + 1),
                               ERR_PARAMETER_INVALID);

                sFence = (acp_uint32_t)sTempBufferLen;
                sOffset = 0;
                for (sIndex = 0; sIndex < sFence; sIndex++)
                {
                    if (sIndex % 2 == 0)
                    {
                        aOutBuffer[sOffset ++] = '\\';
                    }

                    sTargetChar = (acp_char_t)
                                  ((sTempBuffer[sIndex] & 0xF0) >> 4);
                    aOutBuffer[sOffset ++] = (sTargetChar < 10)
                                           ? (sTargetChar + '0')
                                           : (sTargetChar + 'A' - 10);

                    sTargetChar = (acp_char_t)(sTempBuffer[sIndex] & 0x0F);
                    aOutBuffer[sOffset ++] = (sTargetChar < 10)
                                               ? (sTargetChar + '0')
                                               : (sTargetChar + 'A' - 10);
                }
                aOutBuffer[sTextLength] = '\0';

                acpMemFree(sTempBuffer);
                sTempBuffer = NULL;
                break;

            case MTD_BYTE_ID :
            case MTD_VARBYTE_ID :
                sByte = (mtdByteType *)aValue->value;
                sTextLength = (acp_uint32_t)sByte->length * 2;
                ACI_TEST_RAISE(aBufferSize < (sTextLength + 1),
                               ERR_PARAMETER_INVALID);

                sFence = (acp_uint32_t)sByte->length;
                for (sIndex = 0; sIndex < sFence; sIndex++)
                {
                    sTargetChar = (acp_char_t)
                                  ((sByte->value[sIndex] & 0xF0) >> 4);
                    aOutBuffer[sIndex * 2] = (sTargetChar < 10)
                                           ? (sTargetChar + '0')
                                           : (sTargetChar + 'A' - 10);

                    sTargetChar = (acp_char_t)(sByte->value[sIndex] & 0x0F);
                    aOutBuffer[sIndex * 2 + 1] = (sTargetChar < 10)
                                               ? (sTargetChar + '0')
                                               : (sTargetChar + 'A' - 10);
                }
                aOutBuffer[sTextLength] = '\0';
                break;

            case MTD_NIBBLE_ID :
                sNibble = (mtdNibbleType *)aValue->value;

                ACI_TEST_RAISE(mtdModuleById(&sMtd, aColumn->mDataType)
                               != ACI_SUCCESS,
                               ERR_MODULE_GET);

                /* BUG-31539 Nibble의 길이가 255이면 NULL 값이므로,
                 * 사용자에게 길이를 0으로 보여줘야 합니다.
                 */
                if ((*sMtd->isNull)(NULL,
                                    (const void *)aValue->value,
                                    MTD_OFFSET_USELESS) != ACP_TRUE)
                {
                    sTextLength = (acp_uint32_t)sNibble->length;
                }
                else
                {
                    sTextLength = 0;
                }

                ACI_TEST_RAISE(aBufferSize < (sTextLength + 1),
                               ERR_PARAMETER_INVALID);

                sFence = (sTextLength + 1) / 2;
                for (sIndex = 0; sIndex < sFence; sIndex++)
                {
                    sTargetChar = (acp_char_t)
                                  ((sNibble->value[sIndex] & 0xF0) >> 4);
                    aOutBuffer[sIndex * 2] = (sTargetChar < 10)
                                           ? (sTargetChar + '0')
                                           : (sTargetChar + 'A' - 10);

                    // 마지막에는 '\0'으로 덮어쓰므로 검사 미필요

                    sTargetChar = (acp_char_t)(sNibble->value[sIndex] & 0x0F);
                    aOutBuffer[sIndex * 2 + 1] = (sTargetChar < 10)
                                               ? (sTargetChar + '0')
                                               : (sTargetChar + 'A' - 10);
                }
                aOutBuffer[sTextLength] = '\0';
                break;

            case MTD_BIT_ID :
            case MTD_VARBIT_ID :
                sBit = (mtdBitType *)aValue->value;
                sTextLength = (acp_uint32_t)sBit->length;
                ACI_TEST_RAISE(aBufferSize < (sTextLength + 1),
                               ERR_PARAMETER_INVALID);

                sFence = (acp_uint32_t)(sBit->length + 7) / 8;
                for (sIndex = 0; sIndex < sFence; sIndex++)
                {
                    for (sBitIndex = 0; sBitIndex < 8; sBitIndex++)
                    {
                        sBitPos = sIndex * 8 + sBitIndex;
                        if (sBitPos == sTextLength)
                        {
                            break;
                        }

                        sTargetChar = (acp_char_t)((sBit->value[sIndex]
                                      << sBitIndex >> 7) & 0x01);
                        aOutBuffer[sBitPos] = sTargetChar + '0';
                    }
                }
                aOutBuffer[sTextLength] = '\0';
                break;

            case MTD_BLOB_ID :
            case MTD_CLOB_ID :
            case MTD_GEOMETRY_ID :
            default :
                break;
        }
    }

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Altibase Text");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ALA_ERR_MEM_ALLOC)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_ABORT_MEMORY_ALLOC);

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_PARAMETER_INVALID)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_INVALID,
                        "Altibase Text");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_MODULE_GET)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_MTD_MODULE_GET,
                        "Altibase Text",
                        aColumn->mDataType);

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_ENCODE)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_MTD_ENCODE,
                        "Altibase Text");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    if (sTempBuffer != NULL)
    {
        acpMemFree(sTempBuffer);
    }

    return ALA_FAILURE;
}

/*******************************************************************************
 * @breif  컬럼값이 NULL인지 확인한다.
 *
 * @param  aColumn      : 컬럼의 메타 정보이다.
 * @param  aValue       : NULL인지 확인하고자 하는 컬럼값이다.
 * @param  aOutIsNull   : NULL값 여부를 확인한 결과이다.
 * @param  aOutErrorMgr : 에러 메세지를 설정한다.
 *
 * @remark 컬럼값이 NULL인지 확인하여 boolean형 결과를 넘겨준다.
 *
 ******************************************************************************/
ALA_RC ALA_IsNullValue( ALA_Column   * aColumn,
                        ALA_Value    * aValue,
                        ALA_BOOL     * aOutIsNull,
                        ALA_ErrorMgr * aOutErrorMgr )
{
    const mtdModule * sMtd = NULL;

    ACI_TEST_RAISE( aColumn    == NULL, ERR_PARAMETER_NULL );
    ACI_TEST_RAISE( aValue     == NULL, ERR_PARAMETER_NULL );
    ACI_TEST_RAISE( aOutIsNull == NULL, ERR_PARAMETER_NULL );

    if ( aValue->value == NULL )
    {
        *aOutIsNull = ALA_TRUE;
    }
    else
    {
        ACI_TEST_RAISE( mtdModuleById( &sMtd, aColumn->mDataType )
                        != ACI_SUCCESS,
                        ERR_MODULE_GET );

        if ( ( *sMtd->isNull )( NULL,
                                (const void *)aValue->value,
                                MTD_OFFSET_USELESS ) == ACP_TRUE )
        {
            *aOutIsNull = ALA_TRUE;
        }
        else
        {
            *aOutIsNull = ALA_FALSE;
        }
    }

    return ALA_SUCCESS;

    ACI_EXCEPTION( ERR_PARAMETER_NULL )
    {
        ulaSetErrorCode( (ulaErrorMgr *)aOutErrorMgr,
                         ulaERR_IGNORE_PARAMETER_NULL,
                         "Is Null Value" );

        (void)ulaLogError( (ulaErrorMgr *)aOutErrorMgr );
    }

    ACI_EXCEPTION( ERR_MODULE_GET )
    {
        ulaSetErrorCode( (ulaErrorMgr *)aOutErrorMgr,
                         ulaERR_IGNORE_MTD_MODULE_GET,
                         "Is Null Value",
                         aColumn->mDataType );

        (void)ulaLogError( (ulaErrorMgr *)aOutErrorMgr );
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_GetAltibaseSQL(ALA_Table    *aTable,
                          ALA_XLog     *aXLog,
                          UInt          aBufferSize,
                          SChar        *aOutBuffer,
                          ALA_ErrorMgr *aOutErrorMgr)
{
    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutBuffer == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(aBufferSize < 1, ERR_PARAMETER_INVALID);

    switch (aXLog->mHeader.mType)
    {
        case XLOG_TYPE_COMMIT :
            ACI_TEST_RAISE(aBufferSize < 7, ERR_PARAMETER_INVALID);
            acpMemCpy(aOutBuffer, (acp_char_t *)"COMMIT", 7);
            break;

        case XLOG_TYPE_ABORT :
            ACI_TEST_RAISE(aBufferSize < 9, ERR_PARAMETER_INVALID);
            acpMemCpy(aOutBuffer, (acp_char_t *)"ROLLBACK", 9);
            break;

        case XLOG_TYPE_INSERT :
            ACI_TEST_RAISE(aTable == NULL, ERR_PARAMETER_NULL);
            ACI_TEST_RAISE(ulaGetAltibaseSQLInsert(aTable,
                                                   aXLog,
                                                   aBufferSize,
                                                   aOutBuffer,
                                                   aOutErrorMgr)
                           != ACI_SUCCESS,
                           ERR_OUTER_API);
            break;

        case XLOG_TYPE_UPDATE :
            ACI_TEST_RAISE(aTable == NULL, ERR_PARAMETER_NULL);
            ACI_TEST_RAISE(ulaGetAltibaseSQLUpdate(aTable,
                                                   aXLog,
                                                   aBufferSize,
                                                   aOutBuffer,
                                                   aOutErrorMgr)
                           != ACI_SUCCESS,
                           ERR_OUTER_API);
            break;

        case XLOG_TYPE_DELETE :
            ACI_TEST_RAISE(aTable == NULL, ERR_PARAMETER_NULL);
            ACI_TEST_RAISE(ulaGetAltibaseSQLDelete(aTable,
                                                   aXLog,
                                                   aBufferSize,
                                                   aOutBuffer,
                                                   aOutErrorMgr)
                           != ACI_SUCCESS,
                           ERR_OUTER_API);
            break;

        case XLOG_TYPE_SP_SET :
            ACI_TEST_RAISE(aBufferSize <
                                (10 + aXLog->mSavepoint.mSPNameLen + 2 + 1),
                           ERR_PARAMETER_INVALID);
            acpMemCpy(aOutBuffer, (acp_char_t *)"SAVEPOINT ", 10);
            aOutBuffer[10] = '\"';  // BUG-18609
            acpMemCpy(&(aOutBuffer[10 + 1]),
                      aXLog->mSavepoint.mSPName,
                      aXLog->mSavepoint.mSPNameLen);
            // BUG-18609
            aOutBuffer[10 + aXLog->mSavepoint.mSPNameLen + 1] = '\"';
            break;

        case XLOG_TYPE_SP_ABORT :
            ACI_TEST_RAISE(aBufferSize <
                                (22 + aXLog->mSavepoint.mSPNameLen + 2 + 1),
                           ERR_PARAMETER_INVALID);
            acpMemCpy(aOutBuffer, (acp_char_t *)"ROLLBACK TO SAVEPOINT ", 22);
            aOutBuffer[22] = '\"';  // BUG-18609
            acpMemCpy(&(aOutBuffer[22 + 1]),
                      aXLog->mSavepoint.mSPName,
                      aXLog->mSavepoint.mSPNameLen);
            // BUG-18609
            aOutBuffer[22 + aXLog->mSavepoint.mSPNameLen + 1] = '\"';
            break;

        case XLOG_TYPE_LOB_CURSOR_OPEN :
        case XLOG_TYPE_LOB_CURSOR_CLOSE :
        case XLOG_TYPE_LOB_PREPARE4WRITE :
        case XLOG_TYPE_LOB_PARTIAL_WRITE :
        case XLOG_TYPE_LOB_FINISH2WRITE :
        case XLOG_TYPE_LOB_TRIM :
        default :
            ACI_RAISE(ERR_PARAMETER_INVALID);
            break;
    }

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Altibase SQL");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_PARAMETER_INVALID)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_INVALID,
                        "Altibase SQL");

        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION(ERR_OUTER_API)
    {
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }

    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

ALA_RC ALA_IsHiddenColumn( const ALA_Column    * aColumn,
                                 ALA_BOOL      * aIsHiddenColumn,
                                 ALA_ErrorMgr  * aOutErrorMgr )
{
    acp_bool_t sIsHiddenColumn = ACP_FALSE;

    ACI_TEST_RAISE ( aColumn == NULL, ERR_PARAMETER_NULL );
    ACI_TEST_RAISE ( aIsHiddenColumn == NULL, ERR_PARAMETER_NULL );

    sIsHiddenColumn = ulaMetaIsHiddenColumn( (ulaColumn *)aColumn );
    if ( sIsHiddenColumn == ACP_FALSE )
    {
        *aIsHiddenColumn = ALA_FALSE;
    }
    else
    {
        *aIsHiddenColumn = ALA_TRUE;
    }

    return ALA_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Column IsHiddenColumn");
        (void)ulaLogError((ulaErrorMgr *)aOutErrorMgr);
    }
    ACI_EXCEPTION_END;

    return ALA_FAILURE;
}

// TODO : 문제가 생기면 주석 제거
// #ifdef __cplusplus
//     }
// #endif


ACI_RC ulaGetAltibaseSQLDelete(ALA_Table    *aTable,
                               ALA_XLog     *aXLog,
                               UInt          aBufferSize,
                               SChar        *aOutBuffer,
                               ALA_ErrorMgr *aOutErrorMgr)
{
    acp_rc_t      sRc;
    ALA_Column   *sColumn;
    ALA_Value    *sValue;
    acp_sint32_t  sUsedSize = 0;
    acp_uint32_t  sIndex;

    ACI_TEST_RAISE(aTable == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutBuffer == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(aBufferSize < 1, ERR_PARAMETER_INVALID);
    aOutBuffer[0] = '\0';

    sRc = acpSnprintf((acp_char_t*)aOutBuffer,
                      aBufferSize,
                      "DELETE FROM \"%s\".\"%s\" WHERE ",     // BUG-18609
                      aTable->mToUserName,
                      aTable->mToTableName);
    /* BUGBUG : ETRUNC 는 따로 처리해야 하는데... */
    
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);

    sUsedSize = acpCStrLen((acp_char_t*)aOutBuffer, aBufferSize);

    // WHERE Clause
    for (sIndex = 0; sIndex < aXLog->mPrimaryKey.mPKColCnt; sIndex++)
    {
        sColumn = aTable->mPKColumnArray[sIndex];
        ACE_DASSERT(sColumn != NULL);
        ACI_TEST_RAISE(sColumn == NULL, ERR_PARAMETER_INVALID);

        sValue = &(aXLog->mPrimaryKey.mPKColArray[sIndex]);

        // Column Name
        sRc = acpSnprintf((acp_char_t*)(aOutBuffer + sUsedSize),
                          aBufferSize - sUsedSize,
                          "\"%s\" = ",            // BUG-18609
                          sColumn->mColumnName);
        /* BUGBUG : ETRUNC 가 처리되고 있지 않다. */
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);
        sUsedSize = acpCStrLen((acp_char_t*)aOutBuffer, aBufferSize);

        // Column Value
        ACI_TEST(ulaGetAltibaseSQLColumn(sColumn,
                                         sValue,
                                         aBufferSize - sUsedSize,
                                         &(aOutBuffer[sUsedSize]),
                                         aOutErrorMgr)
                 != ACI_SUCCESS);

        if (sIndex < (aXLog->mPrimaryKey.mPKColCnt - 1))
        {
            sRc = acpCStrCat((acp_char_t*)(aOutBuffer + sUsedSize),
                             aBufferSize - sUsedSize,
                             " AND ",
                             5);
            /* BUGBUG : ETRUNC 가 처리되고 있지 않다. */
            ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);
            sUsedSize = acpCStrLen((acp_char_t*)aOutBuffer, aBufferSize);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Altibase SQL Delete");
    }

    ACI_EXCEPTION(ERR_PARAMETER_INVALID)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_INVALID,
                        "Altibase SQL Delete");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaGetAltibaseSQLUpdate(ALA_Table    *aTable,
                               ALA_XLog     *aXLog,
                               UInt          aBufferSize,
                               SChar        *aOutBuffer,
                               ALA_ErrorMgr *aOutErrorMgr)
{
    acp_rc_t      sRc             = ACP_RC_SUCCESS;
    ALA_Column   *sColumn         = NULL;
    ALA_Value    *sValue          = NULL;
    acp_sint32_t  sUsedSize       = 0;
    acp_bool_t    sIsHiddenColumn = ACP_FALSE;
    acp_uint32_t  sIndex;

    ACI_TEST_RAISE(aTable == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutBuffer == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(aBufferSize < 1, ERR_PARAMETER_INVALID);
    aOutBuffer[0] = '\0';

    sRc = acpSnprintf((acp_char_t*)aOutBuffer,
                      aBufferSize,
                      "UPDATE \"%s\".\"%s\" SET ",            // BUG-18609
                      aTable->mToUserName,
                      aTable->mToTableName);
    /* BUGBUG : ETRUNC 는 따로 처리해야 하는데... */

    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);

    sUsedSize = acpCStrLen((acp_char_t*)aOutBuffer, aBufferSize);

    // SET Clause
    for (sIndex = 0; sIndex < aXLog->mColumn.mColCnt; sIndex++)
    {
        ACI_TEST( ulaMetaGetColumnInfo( (ulaTable *)aTable,
                                        aXLog->mColumn.mCIDArray[sIndex],
                                        (ulaColumn **)&sColumn,
                                        (ulaErrorMgr *)aOutErrorMgr )
                  != ACI_SUCCESS);
        ACE_DASSERT(sColumn != NULL);
        ACI_TEST_RAISE(sColumn == NULL, ERR_PARAMETER_INVALID);

        /* hidden column이 아니면 내용을 출력한다. */
        sIsHiddenColumn = ulaMetaIsHiddenColumn( (ulaColumn *)sColumn );
        if ( sIsHiddenColumn != ACP_TRUE )
        {
            if ( sIndex > 0 )
            {
                sRc = acpCStrCat((acp_char_t*)aOutBuffer + sUsedSize,
                                 aBufferSize - sUsedSize,
                                 ", ",
                                 2);
                /* BUGBUG : ETRUNC 가 처리되고 있지 않다. */
                ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);
                sUsedSize = acpCStrLen( (acp_char_t*)aOutBuffer, aBufferSize);
            }
            else
            {
                /* do nothing */
            }

            sValue = &(aXLog->mColumn.mAColArray[sIndex]);

            // Column Name
            sRc = acpSnprintf((acp_char_t*)(aOutBuffer + sUsedSize),
                              aBufferSize - sUsedSize,
                              "\"%s\" = ",            // BUG-18609
                              sColumn->mColumnName);
            /* BUGBUG : ETRUNC 가 처리되고 있지 않다. */

            ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);
            sUsedSize = acpCStrLen((acp_char_t*)aOutBuffer, aBufferSize);

            // Column Value
            ACI_TEST(ulaGetAltibaseSQLColumn(sColumn,
                                             sValue,
                                             aBufferSize - sUsedSize,
                                             &(aOutBuffer[sUsedSize]),
                                             aOutErrorMgr)
                     != ACI_SUCCESS);
        }
        else
        {
            /* do nothing */
        }
    }

    sRc = acpCStrCat((acp_char_t*)(aOutBuffer + sUsedSize),
                     aBufferSize - sUsedSize,
                     " WHERE ",
                     7);

    /* BUGBUG : ETRUNC 가 처리되고 있지 않다. */
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);
    sUsedSize = acpCStrLen((acp_char_t*)aOutBuffer, aBufferSize);

    // WHERE Clause
    for (sIndex = 0; sIndex < aXLog->mPrimaryKey.mPKColCnt; sIndex++)
    {
        sColumn = aTable->mPKColumnArray[sIndex];
        ACE_DASSERT(sColumn != NULL);
        ACI_TEST_RAISE(sColumn == NULL, ERR_PARAMETER_INVALID);

        sValue = &(aXLog->mPrimaryKey.mPKColArray[sIndex]);

        // Column Name
        sRc = acpSnprintf((acp_char_t*)(aOutBuffer + sUsedSize),
                          aBufferSize - sUsedSize,
                          "\"%s\" = ",            // BUG-18609
                          sColumn->mColumnName);

        /* BUGBUG : ETRUNC 가 처리되고 있지 않다. */
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);
        sUsedSize = acpCStrLen((acp_char_t*)aOutBuffer, aBufferSize);

        // Column Value
        ACI_TEST(ulaGetAltibaseSQLColumn(sColumn,
                                         sValue,
                                         aBufferSize - sUsedSize,
                                         &(aOutBuffer[sUsedSize]),
                                         aOutErrorMgr)
                 != ACI_SUCCESS);

        if (sIndex < (aXLog->mPrimaryKey.mPKColCnt - 1))
        {
            sRc = acpCStrCat((acp_char_t*)(aOutBuffer + sUsedSize),
                             aBufferSize - sUsedSize,
                             " AND ",
                             5);

            /* BUGBUG : ETRUNC 가 처리되고 있지 않다. */
            ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_PARAMETER_INVALID);
            sUsedSize = acpCStrLen( (acp_char_t*)aOutBuffer, aBufferSize);
        }
    }
    
    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_NULL,
                        "Altibase SQL Update");
    }

    ACI_EXCEPTION(ERR_PARAMETER_INVALID)
    {
        ulaSetErrorCode((ulaErrorMgr *)aOutErrorMgr,
                        ulaERR_IGNORE_PARAMETER_INVALID,
                        "Altibase SQL Update");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


ACI_RC ulaConvertCharSet(const mtlModule  *aSrcCharSet,
                         const mtlModule  *aDestCharSet,
                         mtdNcharType     *aSource,
                         UChar            *aResult,
                         UInt             *aResultLen)
{
    acp_rc_t             sRc;
    aciConvCharSetList   sIdnDestCharSet;
    aciConvCharSetList   sIdnSrcCharSet;
    acp_uint8_t         *sSourcePtr = NULL;
    acp_uint8_t         *sTempPtr = NULL;
    acp_uint8_t         *sSourceFence = NULL;
    acp_uint8_t         *sResultValue = NULL;
    acp_uint8_t         *sResultFence = NULL;
    acp_sint32_t         sSrcRemain = 0;
    acp_sint32_t         sDestRemain = 0;
    acp_sint32_t         sTempRemain = 0;

    sDestRemain  = aDestCharSet->maxPrecision( aSource->length );

    sSourcePtr   = aSource->value;
    sSourceFence = sSourcePtr + aSource->length;
    sSrcRemain   = aSource->length;

    sResultValue = aResult;
    sResultFence = sResultValue + sDestRemain;

    if (aSrcCharSet->id != aDestCharSet->id)
    {
        sIdnSrcCharSet = mtlGetIdnCharSet(aSrcCharSet);
        sIdnDestCharSet = mtlGetIdnCharSet(aDestCharSet);

        while ( sSrcRemain > 0 )
        {
            ACI_TEST_RAISE(sResultValue >= sResultFence,
                           ERR_INVALID_DATA_LENGTH);

            sTempRemain = sDestRemain;

            sRc = aciConvConvertCharSet(sIdnSrcCharSet,
                                        sIdnDestCharSet,
                                        sSourcePtr,
                                        sSrcRemain,
                                        sResultValue,
                                        &sDestRemain,
                                        -1);
            ACI_TEST(ACP_RC_NOT_SUCCESS(sRc));

            sTempPtr = sSourcePtr;

            (void)(*aSrcCharSet->nextCharPtr)(&sSourcePtr, sSourceFence);

            sResultValue += (sTempRemain - sDestRemain);

            sSrcRemain -= (sSourcePtr - sTempPtr);
        }

        *aResultLen = sResultValue - aResult;
    }
    else
    {
        acpMemCpy(aResult, aSource->value, aSource->length);

        *aResultLen = aSource->length;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_INVALID_DATA_LENGTH)
    {
        ACI_SET(aciSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH));
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

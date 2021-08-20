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
 * $Id: ulaXLogCollector.c 91487 2021-08-18 01:51:15Z yoonhee.kim $
 **********************************************************************/

#include <aclMem.h>
#include <aclMemTlsf.h>

#include <mtcd.h>
#include <ulsEnvHandle.h>
#include <ulsByteOrder.h>
#include <ulaComm.h>
#include <ulaXLogCollector.h>

#define ULA_TLSF_INITIAL_POOL_SIZE  (10 * 1024 * 1024)

static ACI_RC ulaStrToSInt32(acp_char_t    *aStr,
                             acp_size_t     aStrLen,
                             acp_sint32_t  *aNumber)
{
    acp_rc_t      sRc;
    acp_size_t    sStrLen = 0;

    acp_sint32_t  sSign = 0;
    acp_uint32_t  sResult = 0;

    if (aStrLen == 0)
    {
        sStrLen = acpCStrLen(aStr, 50);
    }
    else
    {
        sStrLen = aStrLen;
    }

    sRc = acpCStrToInt32(aStr, sStrLen, &sSign, &sResult, 10, NULL);
    /*
     * BUGBUG Due to the BUG-29561.
     *        Because there is no way but to explore source code to 
     *        find out what error can occur in acpCStrToInt32(),
     *        just assert it to be successful.
     */
    ACE_ASSERT(ACP_RC_IS_SUCCESS(sRc));

    ACI_TEST_RAISE(sResult > (acp_uint32_t)ACP_SINT32_MAX,
                   ERR_NUMBER_OUT_OF_RANGE);

    *aNumber = (acp_sint32_t)sResult;

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_NUMBER_OUT_OF_RANGE)
    {
        /*
         * BUGBUG : Must add error code
         */
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulaConvertEndianValue(acp_uint32_t  aDataType,
                                    acp_bool_t    aEncrypt,
                                    void         *aValue,
                                    ulaErrorMgr  *aErrorMgr)
{
    const mtdModule *sMtd = NULL;
    ulsHandle        sHandle;

    if (aValue != NULL)
    {
        if (aDataType == MTD_GEOMETRY_ID)
        {
            // Geometry는 MTD에 없으므로, ULS를 사용합니다.
            acpMemSet(&sHandle, 0x00, ACI_SIZEOF(ulsHandle));
            (void)ulsInitEnv(&sHandle);
            ACI_TEST_RAISE(ulsEndian(&sHandle, (stdGeometryType *) aValue)
                           != ACS_SUCCESS, ERR_GEOMETRY_ENDIAN);
        }
        else
        {
            if (aEncrypt == ACP_TRUE)
            {
                if (aDataType == MTD_CHAR_ID)
                {
                    ACI_TEST_RAISE(mtdModuleById(&sMtd, MTD_ECHAR_ID)
                                   != ACI_SUCCESS, ERR_MODULE_GET);
                }
                else if (aDataType == MTD_VARCHAR_ID)
                {
                    ACI_TEST_RAISE(mtdModuleById(&sMtd, MTD_EVARCHAR_ID)
                                   != ACI_SUCCESS, ERR_MODULE_GET);
                }
                else
                {
                    ACE_DASSERT(0);
                }
            }
            else
            {
                ACI_TEST_RAISE(mtdModuleById(&sMtd, aDataType)
                               != ACI_SUCCESS, ERR_MODULE_GET);
            }

            (*sMtd->endian)(aValue);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_GEOMETRY_ENDIAN)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_GEMOETRY_ENDIAN,
                        "Geometry Endian",
                        aDataType);
    }
    ACI_EXCEPTION(ERR_MODULE_GET)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_MTD_MODULE_GET,
                        "MTD Endian",
                        aDataType);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* 
 * 아래와 같은 문자열에서 key로 value를 얻는다.
 *
 * "key1=value1;key2=value2;..."
 */
static ACI_RC ulaGetValueByKey(const acp_char_t  *aText,
                               const acp_char_t  *aKey,
                               acp_uint32_t       aMaxValueBufferSize,
                               acp_char_t        *aOutValueBuffer,
                               const acp_char_t **aOutNext,
                               ulaErrorMgr       *aErrorMgr)
{
    acp_rc_t       sRc;
    acp_char_t    *sStartPtr;
    acp_sint32_t   sEqualPos;
    acp_sint32_t   sSemicolonPos;

    acp_uint32_t   sKeySize;         // '\0' 제외한 크기
    acp_uint32_t   sValueSize = 0;       // '\0' 제외한 크기

    ACI_TEST_RAISE(aText == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aKey == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutValueBuffer == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutNext == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE(aMaxValueBufferSize < 1, ERR_PARAMETER_INVALID);

    sStartPtr          = (acp_char_t *)aText;
    sKeySize           = acpCStrLen(aKey, 30);
    aOutValueBuffer[0] = '\0';

    while (sStartPtr != NULL)
    {
        // '='를 검색한다.
        sRc = acpCStrFindChar(sStartPtr,
                              '=',
                              &sEqualPos,
                              0,
                              ACP_CSTR_CASE_SENSITIVE);

        if (ACP_RC_IS_SUCCESS(sRc)) // Key가 있을 경우
        {
            // ';'를 검색하여 Value의 길이를 알아낸다.
            sRc = acpCStrFindChar(sStartPtr,
                                  ';',
                                  &sSemicolonPos,
                                  sEqualPos + 1,
                                  ACP_CSTR_CASE_SENSITIVE);

            if (ACP_RC_IS_SUCCESS(sRc)) // found semicolon
            {
                sValueSize = (acp_uint32_t)(sSemicolonPos - sEqualPos - 1);
                *aOutNext  = sStartPtr + sSemicolonPos + 1;
            }
            else
            {
                if (ACP_RC_IS_ENOENT(sRc)) // not found semicolon
                {
                    /*
                     * BUGBUG : 입력되는 텍스트의 사이즈를 알 방법이 없어
                     *          대충 200 으로
                     */
                    sValueSize = acpCStrLen(sStartPtr + sEqualPos + 1, 200);
                    *aOutNext  = NULL;
                }
                else
                {
                    ACE_ASSERT(0);
                }
            }

            // Key가 일치하는지 확인한다.
            if ((sKeySize == (acp_uint32_t)sEqualPos) &&
                (acpCStrCaseCmp(aKey, sStartPtr, sKeySize) == 0))
            {
                // 길이만큼 Value를 복사한다.
                ACI_TEST_RAISE(sValueSize >= aMaxValueBufferSize,
                               ERR_PARAMETER_INVALID);
                acpMemCpy(aOutValueBuffer,
                          sStartPtr + sEqualPos + 1,
                          sValueSize);
                aOutValueBuffer[sValueSize] = '\0';
                break;
            }
        }
        else
        {
            if (ACP_RC_IS_ENOENT(sRc)) // Key가 없을 경우
            {
                *aOutNext = NULL;
            }
            else
            {
                ACE_ASSERT(0);
            }
        }

        sStartPtr = (acp_char_t *)(*aOutNext);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Value By Key Get");
    }
    ACI_EXCEPTION(ERR_PARAMETER_INVALID)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_INVALID,
                        "Value By Key Get");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * -----------------------------------------------------------------------------
 *  XLog Pool
 * -----------------------------------------------------------------------------
 */
static ACI_RC ulaXLogCollectorAllocXLogMemory(ulaXLogCollector  *aCollector,
                                              ulaXLog          **aXLog,
                                              acp_sint32_t      *aXLogFreeCount,
                                              ulaErrorMgr       *aErrorMgr)
{
    acp_rc_t   sRc;
    acp_bool_t sXLogPoolLock = ACP_FALSE;

    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);

    sRc = acpThrMutexLock(&aCollector->mXLogPoolMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_XLOG_POOL_LOCK);
    sXLogPoolLock = ACP_TRUE;

    /* XLog Pool에서 XLog를 얻습니다. */
    if (aCollector->mXLogFreeCount > 0)
    {
        sRc = aclMemPoolAlloc(&aCollector->mXLogPool, (void **)aXLog);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MEM_POOL_ALLOC);
        aCollector->mXLogFreeCount--;

        // XLog를 초기화합니다.
        acpMemSet(*aXLog, 0x00, ACI_SIZEOF(ulaXLog));
    }
    else
    {
        *aXLog = NULL;
    }

    *aXLogFreeCount = aCollector->mXLogFreeCount;

    sXLogPoolLock = ACP_FALSE;
    sRc = acpThrMutexUnlock(&aCollector->mXLogPoolMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_XLOG_POOL_UNLOCK);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "XLog Memory Alloc");
    }
    ACI_EXCEPTION(ERR_MEM_POOL_ALLOC)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_MEM_POOL_ALLOC);
    }
    ACI_EXCEPTION(ERR_MUTEX_XLOG_POOL_LOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_XLOG_POOL_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_XLOG_POOL_UNLOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_XLOG_POOL_MUTEX_NAME);
    }
    ACI_EXCEPTION_END;

    if (sXLogPoolLock == ACP_TRUE)
    {
        (void)acpThrMutexUnlock(&aCollector->mXLogPoolMutex);
    }

    return ACI_FAILURE;
}

ACI_RC ulaXLogCollectorFreeXLogMemory(ulaXLogCollector *aCollector,
                                      ulaXLog          *aXLog,
                                      ulaErrorMgr      *aErrorMgr)
{
    acp_rc_t       sRc;
    acp_uint32_t   sIndex;
    acp_bool_t     sXLogPoolLock = ACP_FALSE;

    ulaTable       *sTableInfo = NULL;
    ulaColumn      *sColumnInfo = NULL;

    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);

    /* ulaComm에서 할당한 메모리를 해제합니다. */
    // Primary Key
    if (aXLog->mPrimaryKey.mPKColArray != NULL)
    {
        for (sIndex = 0; sIndex < aXLog->mPrimaryKey.mPKColCnt; sIndex++)
        {
            if (aXLog->mPrimaryKey.mPKColArray[sIndex].value != NULL)
            {
                (void)aclMemFree( aCollector->mMemAllocator,
                                  aXLog->mPrimaryKey.mPKColArray[sIndex].value );
                aXLog->mPrimaryKey.mPKColArray[sIndex].value = NULL;
            }
        }

        (void)aclMemFree( aCollector->mMemAllocator,
                          aXLog->mPrimaryKey.mPKColArray );
        aXLog->mPrimaryKey.mPKColArray = NULL;
    }

    // Column
    if (aXLog->mColumn.mBColArray != NULL)
    {
        ACI_TEST(ulaMetaGetTableInfo( &aCollector->mMeta, 
                                      aXLog->mHeader.mTableOID,
                                      &sTableInfo,
                                      aErrorMgr )
                 != ACI_SUCCESS);
        ACI_TEST_RAISE( sTableInfo == NULL, ERR_TABLE_NOT_FOUND );

        for (sIndex = 0; sIndex < aXLog->mColumn.mColCnt; sIndex++)
        {
            if ( aXLog->mColumn.mBColArray[sIndex].value != NULL )
            {
                ACI_TEST( ulaMetaGetColumnInfo( sTableInfo,
                                                aXLog->mColumn.mCIDArray[sIndex],
                                                &sColumnInfo,
                                                aErrorMgr )
                          != ACI_SUCCESS );
                ACI_TEST_RAISE( sColumnInfo == NULL, ERR_COLUMN_NOT_FOUND );

                /* if data type is LOB, recovery value pointer and length */
                if ( ( sColumnInfo->mDataType == MTD_BLOB_ID ) ||
                     ( sColumnInfo->mDataType == MTD_CLOB_ID ) )
                {
                    aXLog->mColumn.mBColArray[sIndex].value = 
                        (acp_char_t *)aXLog->mColumn.mBColArray[sIndex].value - ULA_LOB_DUMMY_HEADER_LEN;
                }
                else
                {
                    /* do nothing */
                }

                (void)aclMemFree( aCollector->mMemAllocator,
                                  aXLog->mColumn.mBColArray[sIndex].value );
                aXLog->mColumn.mBColArray[sIndex].value = NULL;
            }
            else
            {
                /* do nothing */
            }
        }

        (void)aclMemFree( aCollector->mMemAllocator,
                          aXLog->mColumn.mBColArray );
        aXLog->mColumn.mBColArray = NULL;
    }

    if ( aXLog->mColumn.mAColArray != NULL )
    {
        ACI_TEST(ulaMetaGetTableInfo( &aCollector->mMeta, 
                                      aXLog->mHeader.mTableOID,
                                      &sTableInfo,
                                      aErrorMgr )
                 != ACI_SUCCESS);
        ACI_TEST_RAISE( sTableInfo == NULL, ERR_TABLE_NOT_FOUND );

        for (sIndex = 0; sIndex < aXLog->mColumn.mColCnt; sIndex++)
        {
            if ( aXLog->mColumn.mAColArray[sIndex].value != NULL )
            {
                ACI_TEST( ulaMetaGetColumnInfo( sTableInfo,
                                                aXLog->mColumn.mCIDArray[sIndex],
                                                &sColumnInfo,
                                                aErrorMgr )
                          != ACI_SUCCESS );
                ACI_TEST_RAISE( sColumnInfo == NULL, ERR_COLUMN_NOT_FOUND );

                /* if data type is LOB, recovery value pointer and length */
                if ( ( sColumnInfo->mDataType == MTD_BLOB_ID ) ||
                     ( sColumnInfo->mDataType == MTD_CLOB_ID ) )
                {
                    aXLog->mColumn.mAColArray[sIndex].value = 
                        (acp_char_t *)aXLog->mColumn.mAColArray[sIndex].value - ULA_LOB_DUMMY_HEADER_LEN;
                }
                else
                {
                    /* do nothing */
                }

                (void)aclMemFree( aCollector->mMemAllocator,
                                  aXLog->mColumn.mAColArray[sIndex].value );
                aXLog->mColumn.mAColArray[sIndex].value = NULL;
            }
            else
            {
                /* do nothing */
            }
        }

        (void)aclMemFree( aCollector->mMemAllocator,
                          aXLog->mColumn.mAColArray );
        aXLog->mColumn.mAColArray = NULL;
    }

    if ( aXLog->mColumn.mCIDArray != NULL )
    {
        (void)aclMemFree( aCollector->mMemAllocator,
                          aXLog->mColumn.mCIDArray );
        aXLog->mColumn.mCIDArray = NULL;
    }
    else
    {
        /* do nothing */
    }

    // Savepoint
    if (aXLog->mSavepoint.mSPName != NULL)
    {
        (void)aclMemFree( aCollector->mMemAllocator,
                          aXLog->mSavepoint.mSPName );
        aXLog->mSavepoint.mSPName = NULL;
    }

    // LOB
    if (aXLog->mLOB.mLobPiece != NULL)
    {
        (void)aclMemFree( aCollector->mMemAllocator,
                          aXLog->mLOB.mLobPiece );
        aXLog->mLOB.mLobPiece = NULL;
    }

    /* XLog Pool에 반환합니다. */
    sRc = acpThrMutexLock(&aCollector->mXLogPoolMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_XLOG_POOL_LOCK);
    sXLogPoolLock = ACP_TRUE;

    aclMemPoolFree(&aCollector->mXLogPool, aXLog);
    aCollector->mXLogFreeCount++;

    sXLogPoolLock = ACP_FALSE;
    sRc = acpThrMutexUnlock(&aCollector->mXLogPoolMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_XLOG_POOL_UNLOCK);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "XLog Memory Free");
    }
    ACI_EXCEPTION(ERR_MUTEX_XLOG_POOL_LOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_XLOG_POOL_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_XLOG_POOL_UNLOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_XLOG_POOL_MUTEX_NAME);
    }
    ACI_EXCEPTION( ERR_TABLE_NOT_FOUND )
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_TABLE_NOT_FOUND,
                        "ulaXLogCollectorFreeXLogMemory",
                        aXLog->mHeader.mTableOID);
    }
    ACI_EXCEPTION( ERR_COLUMN_NOT_FOUND )
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_COLUMN_NOT_FOUND,
                        "ulaXLogCollectorFreeXLogMemory",
                        aXLog->mColumn.mCIDArray[sIndex]);
    }
    ACI_EXCEPTION_END;

    if (sXLogPoolLock == ACP_TRUE)
    {
        (void)acpThrMutexUnlock(&aCollector->mXLogPoolMutex);
    }

    return ACI_FAILURE;
}

static ACI_RC ulaXLogCollectorFreeXLogFromLinkedList
                                               (ulaXLogCollector  *aCollector,
                                                ulaXLogLinkedList *aList,
                                                ulaErrorMgr       *aErrorMgr)
{
    ulaXLog *sXLog;

    ACI_TEST_RAISE(aList == NULL, ERR_PARAMETER_NULL);

    do
    {
        ACI_TEST(ulaXLogLinkedListRemoveFromHead(aList, &sXLog, aErrorMgr)
                 != ACI_SUCCESS);

        if (sXLog != NULL)
        {
            ACI_TEST(ulaXLogCollectorFreeXLogMemory(aCollector,
                                                    sXLog,
                                                    aErrorMgr)
                     != ACI_SUCCESS);
        }
    } while (sXLog != NULL);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "XLog Free from Linked List");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * -----------------------------------------------------------------------------
 *  Initialize / Finalize
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogCollectorInitialize(ulaXLogCollector  *aCollector,
                                  const acp_char_t  *aXLogSenderName,
                                  const acp_char_t  *aSocketInfo,
                                  acp_sint32_t       aXLogPoolSize,
                                  acp_bool_t         aUseCommittedTxBuffer,
                                  acp_uint32_t       aACKPerXLogCount,
                                  ulaErrorMgr       *aErrorMgr)
{
    acp_rc_t          sRc = ACP_RC_SUCCESS;
    acp_uint32_t      sNameSize = 0;
    acp_char_t        sSocketType[ULA_SOCKET_TYPE_LEN] = { 0 , };
    acp_char_t        sXLogSenderIP[ULA_IP_LEN] = { 0 , };
    acp_char_t        sXLogSenderPort[ULA_PORT_LEN] = { 0, };
    acp_char_t        sListenPort[ULA_PORT_LEN] = { 0 , };
    acp_sint32_t      sListenPortNo = 0;
    acp_sint32_t      sXLogSenderPortNo = 0;
    acp_char_t        sIpStackBuffer[ULA_IP_STACK_LEN] = { 0 , };
    /* mIPv6: 0(IPv4), 1(IPv6-dual), 2(IPv6-only) */
    acp_sint32_t      sIpStack = 0;
    const acp_char_t *sNextPos = NULL;
    acp_uint32_t      sStage = 0;

    acl_mem_tlsf_init_t sTlsfInit;
    acp_bool_t          sIsThreadSafe = ACP_TRUE;

    ACI_TEST_RAISE(aXLogSenderName == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aSocketInfo == NULL, ERR_PARAMETER_NULL);

    acpMemSet( &sTlsfInit, 0x00, ACI_SIZEOF(acl_mem_tlsf_init_t) );
    sTlsfInit.mPoolSize = ULA_TLSF_INITIAL_POOL_SIZE;

    sNameSize = acpCStrLen(aXLogSenderName, ULA_NAME_LEN);
    ACI_TEST_RAISE((sNameSize == 0) || (sNameSize >= ULA_NAME_LEN),
                   ERR_PARAMETER_INVALID);

    ACI_TEST_RAISE(aXLogPoolSize < ULA_MIN_XLOG_POOL_SIZE,
                   ERR_PARAMETER_INVALID);

    ACI_TEST_RAISE(aACKPerXLogCount < ULA_MIN_ACK_PER_XLOG_SIZE,
                   ERR_PARAMETER_INVALID);

    /* XLog Sender Name */
    acpMemSet(aCollector->mXLogSenderName, 0x00, ULA_NAME_LEN);
    acpMemCpy(aCollector->mXLogSenderName, aXLogSenderName, sNameSize);

    ulaMetaInitialize( &aCollector->mMeta );
    sStage = 1;

    /* TLSF Memory */
    sRc = aclMemAllocGetInstance( ACL_MEM_ALLOC_TLSF,
                                  &sTlsfInit,
                                  &(aCollector->mMemAllocator) );
    ACI_TEST_RAISE( ACP_RC_NOT_SUCCESS( sRc ), ULA_ERR_MEM_ALLOC_GET_INSTANCE );

    sStage = 2;
    sRc = aclMemAllocSetAttr( aCollector->mMemAllocator,
                              ACL_MEM_THREAD_SAFE_ATTR,
                              (void *)&sIsThreadSafe );
    ACI_TEST_RAISE( ACP_RC_NOT_SUCCESS( sRc ), ULA_ERR_MEM_ALLOC_SET_ATTR );

    /* XLog Pool */
    sStage = 3;
    sRc = aclMemPoolCreate(&aCollector->mXLogPool,
                           ACI_SIZEOF(ulaXLog),
                           ACP_MIN(aXLogPoolSize,ULA_MAX_INIT_XLOG_POOL_SIZE),
                           -1);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MEM_POOL_INIT);

    sStage = 4;
    sRc = acpThrMutexCreate(&aCollector->mXLogPoolMutex, ACP_THR_MUTEX_DEFAULT);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_INIT);

    aCollector->mXLogPoolLimit = aXLogPoolSize;
    aCollector->mXLogFreeCount = aXLogPoolSize;

    /* Transaction Table */
    sStage = 5;
    aCollector->mUseCommittedTxBuffer = aUseCommittedTxBuffer;
    ACI_TEST(ulaTransTblInitialize(&aCollector->mTransTbl,
                                   0,
                                   aCollector->mUseCommittedTxBuffer,
                                   aErrorMgr)
             != ACI_SUCCESS);

    /* XLog Queue */
    sStage = 6;
    ACI_TEST(ulaXLogLinkedListInitialize(&aCollector->mXLogQueue,
                                         ACP_TRUE,
                                         aErrorMgr) != ACI_SUCCESS);

    /* Network */
    aCollector->mSocketType = ULA_SOCKET_TYPE_NONE;
    aCollector->mXLogSenderIPCount = 0;
    acpMemSet(aCollector->mXLogSenderIP,
              0x00,
              ACI_SIZEOF(aCollector->mXLogSenderIP));
    acpMemSet(aCollector->mUNIXSocketFile,
              0x00,
              ULA_SOCK_NAME_LEN);

    sStage = 7;
    sRc = acpThrMutexCreate(&aCollector->mAuthInfoMutex, ACP_THR_MUTEX_DEFAULT);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_INIT);

    aCollector->mACKPerXLogCount       = aACKPerXLogCount;
    aCollector->mHandshakeTimeoutSec   = ULA_DEFAULT_HANDSHAKE_TIMEOUT;
    aCollector->mReceiveXLogTimeoutSec = ULA_DEFAULT_RECEIVE_XLOG_TIMEOUT;

    aCollector->mNetworkExitFlag = ACP_TRUE;
    aCollector->mSessionValid    = ACP_FALSE;

    sStage = 8;
    sRc = acpThrMutexCreate(&aCollector->mSendMutex, ACP_THR_MUTEX_DEFAULT);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_INIT);

    sStage = 9;
    sRc = acpThrMutexCreate(&aCollector->mReceiveMutex, ACP_THR_MUTEX_DEFAULT);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_INIT);

    ACI_TEST(ulaGetValueByKey(aSocketInfo,
                              (const acp_char_t *)"SOCKET",
                              ULA_SOCKET_TYPE_LEN,
                              sSocketType,
                              &sNextPos,
                              aErrorMgr)
             != ACI_SUCCESS);

    if (acpCStrCaseCmp("TCP", sSocketType, 3) == 0)
    {
        ACI_TEST(ulaGetValueByKey(aSocketInfo,
                                  (const acp_char_t *)"PEER_IP",
                                  ULA_IP_LEN,
                                  sXLogSenderIP,
                                  &sNextPos,
                                  aErrorMgr)
                 != ACI_SUCCESS);

        ACI_TEST(ulaGetValueByKey(aSocketInfo,
                                  (const acp_char_t *)"PEER_REPLICATION_PORT",
                                  ULA_PORT_LEN,
                                  sXLogSenderPort,
                                  &sNextPos,
                                  aErrorMgr)
                 != ACI_SUCCESS);

        ACI_TEST(ulaGetValueByKey(aSocketInfo,
                                  (const acp_char_t *)"MY_PORT",
                                  ULA_PORT_LEN,
                                  sListenPort,
                                  &sNextPos,
                                  aErrorMgr)
                 != ACI_SUCCESS);

        ACI_TEST_RAISE((sXLogSenderIP[0] == '\0') || (sListenPort[0] == '\0')
                        , ERR_PARAMETER_INVALID);

        if( ulaGetValueByKey(aSocketInfo,
                             (const acp_char_t *)"IP_STACK",
                             ULA_IP_STACK_LEN,
                             sIpStackBuffer,
                             &sNextPos,
                             aErrorMgr)
            == ACI_SUCCESS)
        {
            ACI_TEST(ulaStrToSInt32(sIpStackBuffer,
                                    sizeof(sIpStackBuffer),
                                    &sIpStack) != ACI_SUCCESS);
        
            /* mIPv6: 0(IPv4), 1(IPv6-dual), 2(IPv6-only) */
            ACI_TEST_RAISE((sIpStack < 0 || sIpStack > 2), ERR_PARAMETER_INVALID);
        }


        ACI_TEST(ulaStrToSInt32(sListenPort,
                                ACI_SIZEOF(sListenPort),
                                &sListenPortNo) != ACI_SUCCESS);
 
        if ( sXLogSenderPort[0] != '\0' )
        {
            ACI_TEST( ulaStrToSInt32( sXLogSenderPort,
                                      ACI_SIZEOF( sXLogSenderPort ),
                                      &sXLogSenderPortNo ) 
                      != ACI_SUCCESS);
        }
        else
        {
            sXLogSenderPortNo = 0;
        }
       
        ACI_TEST( ulaXLogCollectorInitializeTCP
                                     ( aCollector,
                                     (const acp_char_t *)sXLogSenderIP,
                                     sXLogSenderPortNo,
                                     sListenPortNo,
                                     sIpStack,
                                     aErrorMgr )
                       != ACI_SUCCESS );
    }
    else if (acpCStrCaseCmp("UNIX", sSocketType, ULA_SOCKET_TYPE_LEN) == 0)
    {
        ACI_TEST(ulaXLogCollectorInitializeUNIX(aCollector, aErrorMgr)
                 != ACI_SUCCESS);
    }
    else
    {
        ACI_RAISE(ERR_SOCKET_TYPE_NOT_SUPPORT);
    }

    // PROJ-1663 : IMPLICIT SAVEPOINT SET
    aCollector->mRemainedXLog = NULL;

    /* ACK 관련 수집 정보 */
    sStage = 10;
    sRc = acpThrMutexCreate(&aCollector->mAckMutex, ACP_THR_MUTEX_DEFAULT);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_INIT);

    aCollector->mRestartSN          = ULA_SN_NULL;
    aCollector->mLastCommitSN       = 0;    // BUG-17659
    aCollector->mLastArrivedSN      = ULA_SN_NULL;
    aCollector->mLastProcessedSN    = ULA_SN_NULL;
    aCollector->mProcessedXLogCount = 0;
    aCollector->mStopACKArrived     = ACP_FALSE;
    aCollector->mKeepAliveArrived   = ACP_FALSE;

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "XLog Collector Init");
    }
    ACI_EXCEPTION(ERR_PARAMETER_INVALID)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_INVALID,
                        "XLog Collector Init");
    }
    ACI_EXCEPTION( ULA_ERR_MEM_ALLOC_GET_INSTANCE )
    {
        ulaSetErrorCode( aErrorMgr, ulaERR_ABORT_MEM_ALLOC_GET_INSTANCE,
                         "XLog Collector Init" );
    }
    ACI_EXCEPTION( ULA_ERR_MEM_ALLOC_SET_ATTR )
    {
        ulaSetErrorCode( aErrorMgr, ulaERR_ABORT_MEM_ALLOC_SET_ATTR,
                         "XLog Collector Init" );
    }
    ACI_EXCEPTION(ERR_MEM_POOL_INIT)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_MEM_POOL_INITIALIZE);
    }
    ACI_EXCEPTION(ERR_MUTEX_INIT)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_INITIALIZE,
                        "XLog Collector MUTEX");
    }
    ACI_EXCEPTION(ERR_SOCKET_TYPE_NOT_SUPPORT)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_SOCKET_TYPE_NOT_SUPPORT);
    }
    ACI_EXCEPTION_END;

    switch (sStage)
    {
        case 10 :
            (void)acpThrMutexDestroy(&aCollector->mReceiveMutex);

        case 9 :
            (void)acpThrMutexDestroy(&aCollector->mSendMutex);

        case 8 :
            (void)acpThrMutexDestroy(&aCollector->mAuthInfoMutex);

        case 7 :

        case 6 :
            (void)ulaTransTblDestroy(&aCollector->mTransTbl, NULL);

        case 5 :
            (void)acpThrMutexDestroy(&aCollector->mXLogPoolMutex);

        case 4 :
            aclMemPoolDestroy(&aCollector->mXLogPool);

        case 3 :

        case 2 :
            (void)aclMemAllocFreeInstance( aCollector->mMemAllocator );
        case 1 :
            ulaMetaDestroy( &aCollector->mMeta );
            break;

        default :
            break;
    }

    return ACI_FAILURE;
}

ACI_RC ulaXLogCollectorSetXLogPoolSize(ulaXLogCollector *aCollector, 
                                       acp_sint32_t      aXLogPoolSize,
                                       ulaErrorMgr      *aErrorMgr)
{
    acp_sint32_t sDiffOfXLogPoolSize = 0;
    acp_sint32_t sAdjustXLogFreeCount = 0;

    ACI_TEST_RAISE(aXLogPoolSize < ULA_MIN_XLOG_POOL_SIZE,
                   ERR_PARAMETER_INVALID);

    sDiffOfXLogPoolSize = aXLogPoolSize - aCollector->mXLogPoolLimit;
    sAdjustXLogFreeCount = aCollector->mXLogFreeCount + sDiffOfXLogPoolSize;

    ACI_TEST_RAISE(sAdjustXLogFreeCount < 0, ERR_PARAMETER_INVALID);
 
    aCollector->mXLogPoolLimit = aXLogPoolSize;
    aCollector->mXLogFreeCount = sAdjustXLogFreeCount;

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_INVALID)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_INVALID,
                        "XLog Collector SetXLogPoolSize");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
ACI_RC ulaXLogCollectorDestroy(ulaXLogCollector *aCollector,
                               ulaErrorMgr      *aErrorMgr)
{
    acp_rc_t           sRc;
    acp_uint32_t       sTrIndex   = ACP_UINT32_MAX;
    acp_uint32_t       sTableSize = 0;
    ulaXLogLinkedList *sList      = NULL;
    acp_uint32_t       sStage     = 0;
    ulaTransTblNode   *sTmpNode   = NULL;

    if (aCollector->mUseCommittedTxBuffer == ACP_TRUE)
    {
        sTableSize = ulaTransTblGetTableSize(&aCollector->mTransTbl);
    }

    /* ACK 관련 수집 정보 */
    sRc = acpThrMutexDestroy(&aCollector->mAckMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_DESTROY);
    sStage = 1;

    /* Network */
    sRc = acpThrMutexDestroy(&aCollector->mAuthInfoMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_DESTROY);
    sStage = 2;

    ACI_TEST(ulaXLogCollectorFinishNetwork(aCollector, aErrorMgr)
             != ACI_SUCCESS);
    sStage = 3;

    sRc = acpThrMutexDestroy(&aCollector->mSendMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_DESTROY);
    sStage = 4;

    sRc = acpThrMutexDestroy(&aCollector->mReceiveMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_DESTROY);
    sStage = 5;

    // PROJ-1663
    if (aCollector->mRemainedXLog != NULL)
    {
        ACI_TEST(ulaXLogCollectorFreeXLogMemory(aCollector,
                                                aCollector->mRemainedXLog,
                                                aErrorMgr)
                 != ACI_SUCCESS);
        aCollector->mRemainedXLog = NULL;
    }
    sStage = 6;

    /* XLog Queue */
    ACI_TEST(ulaXLogCollectorFreeXLogFromLinkedList(aCollector,
                                                    &aCollector->mXLogQueue,
                                                    aErrorMgr)
                   != ACI_SUCCESS);
    sStage = 7;

    sStage = 8;

    /* Transaction Table */
    if (aCollector->mUseCommittedTxBuffer == ACP_TRUE)
    {
        for (sTrIndex = 0; sTrIndex < sTableSize; sTrIndex++)
        {
            sTmpNode = ulaTransTblGetTrNodeDirect(&aCollector->mTransTbl,
                                                  sTrIndex);
            ACE_ASSERT(sTmpNode != NULL);
            sList = &(sTmpNode->mCollectionList);
            ACI_TEST(ulaXLogCollectorFreeXLogFromLinkedList(aCollector,
                                                            sList,
                                                            aErrorMgr)
                     != ACI_SUCCESS);
        }
    }
    sStage = 9;

    ACI_TEST(ulaTransTblDestroy(&aCollector->mTransTbl, aErrorMgr)
             != ACI_SUCCESS);
    sStage = 10;

    /* XLog Pool & TLSF Memory */
    aclMemPoolDestroy(&aCollector->mXLogPool);
    (void)aclMemAllocFreeInstance( aCollector->mMemAllocator );
    sStage = 11;

    sRc = acpThrMutexDestroy(&aCollector->mXLogPoolMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_DESTROY);

    sStage = 12;
    ulaMetaDestroy(&aCollector->mMeta);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_MUTEX_DESTROY)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_DESTROY,
                        "XLog Collector MUTEX");
    }
    ACI_EXCEPTION_END;

    switch (sStage)
    {
        case 0 :
            (void)acpThrMutexDestroy(&aCollector->mAuthInfoMutex);

        case 1 :
            (void)ulaXLogCollectorFinishNetwork(aCollector, NULL);

        case 2 :
            (void)acpThrMutexDestroy(&aCollector->mSendMutex);

        case 3 :
            (void)acpThrMutexDestroy(&aCollector->mReceiveMutex);

        case 4 :
            // PROJ-1663
            if (aCollector->mRemainedXLog != NULL)
            {
                (void)ulaXLogCollectorFreeXLogMemory(aCollector,
                                                     aCollector->mRemainedXLog,
                                                     NULL);
                aCollector->mRemainedXLog = NULL;
            }

        case 5 :
            (void)ulaXLogCollectorFreeXLogFromLinkedList
                                    (aCollector, &aCollector->mXLogQueue, NULL);

        case 6 :

        case 7 :
            if (aCollector->mUseCommittedTxBuffer == ACP_TRUE)
            {
                for ( sTrIndex = 0; sTrIndex < sTableSize; sTrIndex++ )
                {
                    sTmpNode = ulaTransTblGetTrNodeDirect
                                        (&aCollector->mTransTbl, sTrIndex);
                    ACE_ASSERT(sTmpNode != NULL);
                    sList = &(sTmpNode->mCollectionList);
                    (void)ulaXLogCollectorFreeXLogFromLinkedList(aCollector,
                                                                 sList,
                                                                 NULL);
                }
            }

        case 8 :
            (void)ulaTransTblDestroy(&aCollector->mTransTbl, NULL);

        case 9 :
            aclMemPoolDestroy(&aCollector->mXLogPool);
            (void)aclMemAllocFreeInstance( aCollector->mMemAllocator );

        case 10 :
            (void)acpThrMutexDestroy(&aCollector->mXLogPoolMutex);
        case 11 :
            ulaMetaDestroy( &aCollector->mMeta );
            break;
        default :
            break;
    }


    return ACI_FAILURE;
}

/*
 * -----------------------------------------------------------------------------
 *  Network
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogCollectorFinishNetwork(ulaXLogCollector *aCollector,
                                     ulaErrorMgr      *aErrorMgr)
{
    acp_rc_t   sRc;
    acp_bool_t sSessionValid;
    acp_bool_t sSendLock = ACP_FALSE;
    acp_bool_t sReceiveLock = ACP_FALSE;

    sRc = acpThrMutexLock(&aCollector->mSendMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_SEND_LOCK);
    sSendLock = ACP_TRUE;

    sRc = acpThrMutexLock(&aCollector->mReceiveMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_RECEIVE_LOCK);
    sReceiveLock = ACP_TRUE;

    sSessionValid                = aCollector->mSessionValid;
    aCollector->mNetworkExitFlag = ACP_TRUE;
    aCollector->mSessionValid    = ACP_FALSE;

    if (sSessionValid == ACP_TRUE)
    {
        /* Peer Link 종료 */
        ACI_TEST_RAISE(cmiShutdownLink(aCollector->mPeerLink,
                                       CMI_DIRECTION_RDWR)
                       != ACI_SUCCESS, ERR_LINK_SHUTDOWN);
        ACI_TEST_RAISE( cmiFreeCmBlock( &(aCollector->mProtocolContext) )
                        != ACI_SUCCESS, ERR_FREE_CM_BLOCK );
        ACI_TEST_RAISE(cmiFreeLink(aCollector->mPeerLink)
                       != ACI_SUCCESS, ERR_LINK_FREE);
    }

    sReceiveLock = ACP_FALSE;
    sRc = acpThrMutexUnlock(&aCollector->mReceiveMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_RECEIVE_UNLOCK);
    sSendLock = ACP_FALSE;
    sRc = acpThrMutexUnlock(&aCollector->mSendMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_SEND_UNLOCK);

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_FREE_CM_BLOCK )
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_FREE_CM_BLOCK,
                        "finish Network");
        (void)cmiShutdownLink(aCollector->mPeerLink, CMI_DIRECTION_RDWR);
        (void)cmiFreeLink(aCollector->mPeerLink);
    }
    ACI_EXCEPTION(ERR_MUTEX_SEND_LOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_SEND_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_SEND_UNLOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_SEND_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_RECEIVE_LOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_RECEIVE_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_RECEIVE_UNLOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_RECEIVE_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_LINK_SHUTDOWN)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_LINK_SHUTDOWN);
        (void)cmiFreeLink(aCollector->mPeerLink);
    }
    ACI_EXCEPTION(ERR_LINK_FREE)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_LINK_FREE);
    }
    ACI_EXCEPTION_END;

    if (sReceiveLock == ACP_TRUE)
    {
        (void)acpThrMutexUnlock(&aCollector->mReceiveMutex);
    }
    if (sSendLock == ACP_TRUE)
    {
        (void)acpThrMutexUnlock(&aCollector->mSendMutex);
    }

    return ACI_FAILURE;
}

ACI_RC ulaXLogCollectorInitializeTCP( ulaXLogCollector *aCollector,
                                      const acp_char_t *aXLogSenderIP,
                                      acp_sint32_t      aXLogSenderPort,
                                      acp_sint32_t      aXLogCollectorPort,
                                      acp_sint32_t      aXLogCollectorIpStack,
                                      ulaErrorMgr      *aErrorMgr )
{
    acp_uint32_t sIPSize;

    ACI_TEST_RAISE(aCollector->mSocketType != ULA_SOCKET_TYPE_NONE,
                   ERR_ALREADY_INITIALIZE);

    ACI_TEST_RAISE(aXLogSenderIP == NULL, ERR_PARAMETER_NULL);

    sIPSize = acpCStrLen(aXLogSenderIP, ULA_IP_LEN);
    ACI_TEST_RAISE(sIPSize >= ULA_IP_LEN, ERR_PARAMETER_INVALID);

    ACI_TEST_RAISE((aXLogCollectorPort < ULA_MIN_PORT_NO) ||
                   (aXLogCollectorPort > ULA_MAX_PORT_NO),
                   ERR_PARAMETER_INVALID);

    aCollector->mSocketType         = ULA_SOCKET_TYPE_TCP;
    aCollector->mTCPPort            = aXLogCollectorPort;
    aCollector->mTCPIpStack         = aXLogCollectorIpStack;
    aCollector->mXLogSenderIPCount = 1;
    acpMemSet(aCollector->mXLogSenderIP[0], 0x00, ULA_IP_LEN);
    acpMemCpy(aCollector->mXLogSenderIP[0], aXLogSenderIP, sIPSize);
    aCollector->mXLogSenderPort[0] = aXLogSenderPort;

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_ALREADY_INITIALIZE)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_SOCKET_ALREADY_INITIALIZE);
    }
    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "TCP Socket Init");
    }
    ACI_EXCEPTION(ERR_PARAMETER_INVALID)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_INVALID,
                        "TCP Socket Init");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaXLogCollectorInitializeUNIX(ulaXLogCollector *aCollector,
                                      ulaErrorMgr      *aErrorMgr)
{
    acp_rc_t    sRc;
    acp_char_t  sName[ULA_NAME_LEN];
    acp_char_t *sEnvAltibaseHome = NULL;


    ACI_TEST_RAISE(aCollector->mSocketType != ULA_SOCKET_TYPE_NONE,
                   ERR_ALREADY_INITIALIZE);

    aCollector->mSocketType = ULA_SOCKET_TYPE_UNIX;

    acpMemSet(aCollector->mUNIXSocketFile, 0x00, ULA_SOCK_NAME_LEN);
    acpMemCpy(sName, aCollector->mXLogSenderName, ULA_NAME_LEN);

    sRc = acpEnvGet("ALTIBASE_HOME", &sEnvAltibaseHome);
    ACI_TEST_RAISE(ACP_RC_IS_ENOENT(sRc), ERR_NO_ENV);
    ACE_ASSERT(ACP_RC_IS_SUCCESS(sRc));

    (void)acpCStrToUpper(sName, ULA_NAME_LEN);
    (void)acpSnprintf(aCollector->mUNIXSocketFile,
                      ULA_SOCK_NAME_LEN,
                      "%s%c%s%c%s%s",
                      sEnvAltibaseHome,
                      '/',
                      "trc",
                      '/',
                      "rp-",
                      sName);  // XLog Sender의 이름을 대문자로 지정

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_ALREADY_INITIALIZE)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_SOCKET_ALREADY_INITIALIZE);
    }
    ACI_EXCEPTION(ERR_NO_ENV)
    {
        ulaSetErrorCode(aErrorMgr,
                        ulaERR_ABORT_NO_ENV_VARIABLE,
                        "ALTIBASE_HOME");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


/*
 * -----------------------------------------------------------------------------
 *  Preparation
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogCollectorAddAuthInfo(ulaXLogCollector *aCollector,
                                   const acp_char_t *aAuthInfo,
                                   ulaErrorMgr      *aErrorMgr)
{
    acp_rc_t          sRc = ACP_RC_SUCCESS;
    const acp_char_t *sNextPos = NULL;
    acp_char_t        sXLogSenderIP[ULA_IP_LEN] = {0, };
    acp_char_t        sXLogSenderPort[ULA_PORT_LEN] = {0, };
    acp_sint32_t      sXLogSenderPortNo = 0;
    acp_bool_t        sAuthInfoLock = ACP_FALSE;

    ACI_TEST_RAISE(aCollector->mSocketType != ULA_SOCKET_TYPE_TCP,
                   ERR_NOT_TCP_SOCKET);

    ACI_TEST_RAISE(aAuthInfo == NULL, ERR_PARAMETER_NULL);

    ACI_TEST(ulaGetValueByKey(aAuthInfo,
                              (const acp_char_t *)"PEER_IP",
                              ULA_IP_LEN,
                              sXLogSenderIP,
                              &sNextPos,
                              aErrorMgr)
             != ACI_SUCCESS);
    ACI_TEST_RAISE(sXLogSenderIP[0] == '\0', ERR_PARAMETER_INVALID);

    ACI_TEST( ulaGetValueByKey( aAuthInfo,
                                (const acp_char_t *)"PEER_REPLICATION_PORT",
                                ULA_PORT_LEN,
                                sXLogSenderPort,
                                &sNextPos,
                                aErrorMgr )
             != ACI_SUCCESS );

    if ( sXLogSenderPort[0] != '\0' )
    {
        ACI_TEST( ulaStrToSInt32( sXLogSenderPort,
                                  ACI_SIZEOF( sXLogSenderPort ),
                                  &sXLogSenderPortNo ) 
                  != ACI_SUCCESS );
    }
    else
    {
        sXLogSenderPortNo = 0;
    }

    sRc = acpThrMutexLock(&aCollector->mAuthInfoMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_AUTH_INFO_LOCK);
    sAuthInfoLock = ACP_TRUE;

    ACI_TEST_RAISE(aCollector->mXLogSenderIPCount == ULA_MAX_AUTH_INFO_SIZE,
                   ERR_AUTH_INFO_MAX);

    acpMemCpy(aCollector->mXLogSenderIP[aCollector->mXLogSenderIPCount],
              sXLogSenderIP,
              ULA_IP_LEN);
    aCollector->mXLogSenderPort[aCollector->mXLogSenderIPCount] = sXLogSenderPortNo;
    aCollector->mXLogSenderIPCount++;

    sAuthInfoLock = ACP_FALSE;
    sRc = acpThrMutexUnlock(&aCollector->mAuthInfoMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_AUTH_INFO_UNLOCK);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_NOT_TCP_SOCKET)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_SOCKET_NOT_SUPPORT_API);
    }
    ACI_EXCEPTION(ERR_AUTH_INFO_MAX)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_AUTH_INFO_MAX);
    }
    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Authentication Information Add");
    }
    ACI_EXCEPTION(ERR_PARAMETER_INVALID)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_INVALID,
                        "Authentication Information Add");
    }
    ACI_EXCEPTION(ERR_MUTEX_AUTH_INFO_LOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_AUTH_INFO_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_AUTH_INFO_UNLOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_AUTH_INFO_MUTEX_NAME);
    }
    ACI_EXCEPTION_END;

    if (sAuthInfoLock == ACP_TRUE)
    {
        (void)acpThrMutexUnlock(&aCollector->mAuthInfoMutex);
    }

    return ACI_FAILURE;
}

ACI_RC ulaXLogCollectorRemoveAuthInfo(ulaXLogCollector *aCollector,
                                      const acp_char_t *aAuthInfo,
                                      ulaErrorMgr      *aErrorMgr)
{
    acp_rc_t          sRc = ACP_RC_SUCCESS;
    acp_uint32_t      sIndex = 0;
    acp_char_t        sXLogSenderIP[ULA_IP_LEN] = {0, };

    const acp_char_t *sNextPos = NULL;
    acp_bool_t        sAuthInfoLock = ACP_FALSE;

    ACI_TEST_RAISE(aCollector->mSocketType != ULA_SOCKET_TYPE_TCP,
                   ERR_NOT_TCP_SOCKET);

    ACI_TEST_RAISE(aAuthInfo == NULL, ERR_PARAMETER_NULL);

    ACI_TEST(ulaGetValueByKey(aAuthInfo,
                              (const acp_char_t *)"PEER_IP",
                              ULA_IP_LEN,
                              sXLogSenderIP,
                              &sNextPos,
                              aErrorMgr)
             != ACI_SUCCESS);

    ACI_TEST_RAISE(sXLogSenderIP[0] == '\0', ERR_PARAMETER_INVALID);

    sRc = acpThrMutexLock(&aCollector->mAuthInfoMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_AUTH_INFO_LOCK);
    sAuthInfoLock = ACP_TRUE;

    ACI_TEST_RAISE(aCollector->mXLogSenderIPCount == 1, ERR_AUTH_INFO_ONE);

    for (sIndex = 0; sIndex < aCollector->mXLogSenderIPCount; sIndex++)
    {
        if (acpCStrCaseCmp(aCollector->mXLogSenderIP[sIndex],
                           sXLogSenderIP,
                           ULA_IP_LEN) == 0)
        {
            aCollector->mXLogSenderIPCount--;

            if (sIndex != aCollector->mXLogSenderIPCount)
            {
                acpMemCpy
                    (aCollector->mXLogSenderIP[sIndex],
                     aCollector->mXLogSenderIP[aCollector->mXLogSenderIPCount],
                     ULA_IP_LEN);

                aCollector->mXLogSenderPort[sIndex] = aCollector->mXLogSenderPort[aCollector->mXLogSenderIPCount];
            }
            break;
        }
    }

    sAuthInfoLock = ACP_FALSE;
    sRc = acpThrMutexUnlock(&aCollector->mAuthInfoMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_AUTH_INFO_UNLOCK);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_NOT_TCP_SOCKET)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_SOCKET_NOT_SUPPORT_API);
    }
    ACI_EXCEPTION(ERR_AUTH_INFO_ONE)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_AUTH_INFO_ONE);
    }
    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Authentication Information Remove");
    }
    ACI_EXCEPTION(ERR_PARAMETER_INVALID)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_INVALID,
                        "Authentication Information Remove");
    }
    ACI_EXCEPTION(ERR_MUTEX_AUTH_INFO_LOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_AUTH_INFO_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_AUTH_INFO_UNLOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_AUTH_INFO_MUTEX_NAME);
    }
    ACI_EXCEPTION_END;

    if (sAuthInfoLock == ACP_TRUE)
    {
        (void)acpThrMutexUnlock(&aCollector->mAuthInfoMutex);
    }

    return ACI_FAILURE;
}

ACI_RC ulaXLogCollectorSetHandshakeTimeout(ulaXLogCollector *aCollector,
                                           acp_uint32_t      aSecond,
                                           ulaErrorMgr      *aErrorMgr)
{
    ACI_TEST_RAISE((aSecond < ULA_TIMEOUT_MIN) || (aSecond > ULA_TIMEOUT_MAX),
                   ERR_PARAMETER_INVALID);

    aCollector->mHandshakeTimeoutSec = aSecond;

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_INVALID)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_INVALID,
                        "Handshake Timeout");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaXLogCollectorSetReceiveXLogTimeout(ulaXLogCollector *aCollector,
                                             acp_uint32_t      aSecond,
                                             ulaErrorMgr      *aErrorMgr)
{
    ACI_TEST_RAISE((aSecond < ULA_TIMEOUT_MIN) || (aSecond > ULA_TIMEOUT_MAX),
                   ERR_PARAMETER_INVALID);

    aCollector->mReceiveXLogTimeoutSec = aSecond;

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_INVALID)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_INVALID,
                        "Handshake Timeout");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


ACI_RC ulaWakeupPeerSender( ulaXLogCollector * aCollector,
                            ulaErrorMgr      * aErrorMgr,
                            acp_char_t       * aSenderIP,
                            acp_sint32_t       aSenderPort )
{
    cmiLink       * sConnectLink = NULL;
    cmiConnectArg   sConnectArg;
    acp_time_t      sTimeout = acpTimeFromSec( 5 ); /* 5 Sec */

    acp_uint32_t    sFlag         = ULA_WAKEUP_PEER_SENDER_FLAG_SET;
    acp_uint8_t     sState        = 0;

    acp_char_t      sMsg[ULA_ACK_MSG_LEN] = {0, };
    acp_uint32_t    sMsgLen               = acpCStrLen( sMsg, ULA_ACK_MSG_LEN );
    acp_uint64_t    sRemoteVersion        = 0;

    cmiProtocolContext sProtocolContext;

    ACI_TEST_RAISE( aCollector->mSocketType == ULA_SOCKET_TYPE_NONE,
                    ERR_SOCKET_TYPE_NONE );

    ACI_TEST_RAISE( cmiAllocLink( &sConnectLink,
                                  CMI_LINK_TYPE_PEER_CLIENT,
                                  CMI_LINK_IMPL_TCP ) != ACI_SUCCESS,
                    ERR_LINK_ALLOC );
    sState = 1;

    acpMemSet(&sConnectArg, 0x00, ACI_SIZEOF( cmiConnectArg ));
    sConnectArg.mTCP.mPort = aSenderPort;
    sConnectArg.mTCP.mAddr = aSenderIP;
    sConnectArg.mTCP.mBindAddr = NULL;

    /* Initialize Protocol Context & Alloc CM Block */
    ACI_TEST( cmiMakeCmBlockNull( &sProtocolContext ) != ACI_SUCCESS );
    ACI_TEST_RAISE( cmiAllocCmBlock( &sProtocolContext,
                                     CMI_PROTOCOL_MODULE( RP ),
                                     (cmiLink *)sConnectLink,
                                     aCollector )
                    != ACI_SUCCESS, ERR_ALLOC_CM_BLOCK );
    sState = 2;
        
    /* wake up Peer Sender */
    ACI_TEST( cmiConnectWithoutData( &sProtocolContext,
                                     &sConnectArg,
                                     sTimeout,
                                     SO_REUSEADDR )
              != ACI_SUCCESS );
    sState = 3;

    ACI_TEST( ulaXLogCollectorCheckRemoteVersion( &sProtocolContext,
                                                  sTimeout,
                                                  aErrorMgr,
                                                  sMsg,
                                                  &sMsgLen ) 
              != ACI_SUCCESS );
    
    ulaXLogCollectorGetVersionFromAck( sMsg,
                                       sMsgLen,
                                       &sRemoteVersion );

    ACI_TEST( ulaMetaSendMeta( &sProtocolContext, 
                               aCollector->mXLogSenderName,
                               sRemoteVersion,
                               sFlag,
                               aErrorMgr )
              != ACI_SUCCESS );
    sState = 2;
    
    ACI_TEST_RAISE( cmiShutdownLink( sConnectLink,
                                     CMI_DIRECTION_RDWR )
                    != ACI_SUCCESS, ERR_LINK_SHUTDOWN );

    sState = 1;
    ACI_TEST_RAISE( cmiFreeCmBlock( &sProtocolContext ) 
                    != ACI_SUCCESS, ERR_ALLOC_CM_BLOCK );

    sState = 0;
    ACI_TEST_RAISE(cmiFreeLink(sConnectLink)
                   != ACI_SUCCESS, ERR_LINK_FREE);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_LINK_SHUTDOWN)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_LINK_SHUTDOWN);
    }
    ACI_EXCEPTION( ERR_ALLOC_CM_BLOCK )
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_ALLOC_CM_BLOCK,
                        "wakeupPeer");
    }
    ACI_EXCEPTION(ERR_SOCKET_TYPE_NONE)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_SOCKET_TYPE_NONE);
    }
    ACI_EXCEPTION(ERR_LINK_ALLOC)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_LINK_ALLOC);
    }
    ACI_EXCEPTION(ERR_LINK_FREE)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_LINK_FREE);
    }

    ACI_EXCEPTION_END;

    switch ( sState )
    {
        case 3:
            (void)cmiShutdownLink( sConnectLink, CMI_DIRECTION_RDWR );
        case 2:
            (void)cmiFreeCmBlock( &sProtocolContext );
        case 1:
            (void)cmiFreeLink( sConnectLink );
        default:
            break;
    }
    
    return ACI_FAILURE;
}

/*
 * -----------------------------------------------------------------------------
 *  Control
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogCollectorHandshakeBefore(ulaXLogCollector *aCollector,
                                       ulaErrorMgr      *aErrorMgr)
{
    ulaXLogLinkedList  *sList;
    ulaTransTblNode    *sTmpNode;
    acp_uint32_t        sTableSize;
    acp_uint32_t        sTrIndex;

    /* ACK 관련 수집 정보 초기화 */
    aCollector->mRestartSN          = ULA_SN_NULL;
    aCollector->mLastCommitSN       = 0;    // BUG-17659
    aCollector->mLastArrivedSN      = ULA_SN_NULL;
    aCollector->mLastProcessedSN    = ULA_SN_NULL;
    aCollector->mProcessedXLogCount = 0;
    aCollector->mStopACKArrived     = ACP_FALSE;
    aCollector->mKeepAliveArrived   = ACP_FALSE;

    /* Meta 정보 초기화 */
    ulaMetaDestroy(&aCollector->mMeta);
    ulaMetaInitialize(&aCollector->mMeta);

    /* XLog Queue 초기화 */
    // PROJ-1663
    if (aCollector->mRemainedXLog != NULL)
    {
        ACI_TEST(ulaXLogCollectorFreeXLogMemory(aCollector,
                                                aCollector->mRemainedXLog,
                                                aErrorMgr)
                 != ACI_SUCCESS);
        aCollector->mRemainedXLog = NULL;
    }

    ACI_TEST(ulaXLogCollectorFreeXLogFromLinkedList(aCollector,
                                                    &aCollector->mXLogQueue,
                                                    aErrorMgr)
                   != ACI_SUCCESS);

    ACI_TEST(ulaXLogLinkedListDestroy(&aCollector->mXLogQueue, aErrorMgr)
             != ACI_SUCCESS);

    ACI_TEST(ulaXLogLinkedListInitialize(&aCollector->mXLogQueue,
                                         ACP_TRUE,
                                         aErrorMgr)
             != ACI_SUCCESS);

    /* Transaction Table 초기화 - 1 */
    if (aCollector->mUseCommittedTxBuffer == ACP_TRUE)
    {
        sTableSize = ulaTransTblGetTableSize(&aCollector->mTransTbl);

        for (sTrIndex = 0; sTrIndex < sTableSize; sTrIndex++)
        {
            sTmpNode = ulaTransTblGetTrNodeDirect(&aCollector->mTransTbl,
                                                  sTrIndex);
            ACE_ASSERT(sTmpNode != NULL);
            sList = &(sTmpNode->mCollectionList);
            ACI_TEST(ulaXLogCollectorFreeXLogFromLinkedList(aCollector,
                                                            sList,
                                                            aErrorMgr)
                           != ACI_SUCCESS)
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaXLogCollectorHandshake(ulaXLogCollector *aCollector,
                                 ulaErrorMgr      *aErrorMgr)
{
    acp_rc_t     sRc;
    cmiLink    * sListenLink;
    cmiListenArg sListenArg;
    acp_bool_t   sListenLinkAlloc = ACP_FALSE;
    acp_time_t   sTimeout;
    acp_char_t   sBuffer[ULA_ACK_MSG_LEN];
    acp_bool_t   sAuthInfoLock = ACP_FALSE;
    acp_bool_t   sSendLock = ACP_FALSE;
    acp_bool_t   sReceiveLock = ACP_FALSE;
    acp_uint32_t sTableSize;
    acp_time_t   sTotalWaitTime = 0;
    acp_time_t   sWaitTime = acpTimeFromSec( 3 ); /* 3 Sec */

    acp_uint32_t sIndex;
    acp_char_t   sXLogSenderIP[ULA_IP_LEN] = { 0, };
    ulaVersion   sVersion;

    ACI_TEST_RAISE(aCollector->mSocketType == ULA_SOCKET_TYPE_NONE,
                   ERR_SOCKET_TYPE_NONE);

    sTimeout = acpTimeFromSec( aCollector->mHandshakeTimeoutSec );

    acpMemSet(sBuffer, 0x00, ULA_ACK_MSG_LEN);

    /* 기존의 연결을 종료 */
    ACI_TEST(ulaXLogCollectorFinishNetwork(aCollector,
                                           aErrorMgr) != ACI_SUCCESS);

    sRc = acpThrMutexLock(&aCollector->mSendMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_SEND_LOCK);
    sSendLock = ACP_TRUE;
    sRc = acpThrMutexLock(&aCollector->mReceiveMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_RECEIVE_LOCK);
    sReceiveLock = ACP_TRUE;

    /* Listen Link 할당 및 설정 */
    if (aCollector->mSocketType == ULA_SOCKET_TYPE_TCP)
    {
        ACI_TEST_RAISE(cmiAllocLink(&sListenLink,
                                    CMI_LINK_TYPE_LISTEN,
                                    CMI_LINK_IMPL_TCP) != ACI_SUCCESS,
                       ERR_LINK_ALLOC);
        sListenLinkAlloc = ACP_TRUE;

        sListenArg.mTCP.mPort      = aCollector->mTCPPort;
        sListenArg.mTCP.mMaxListen = 1;
        sListenArg.mTCP.mIPv6      = aCollector->mTCPIpStack;
    }
    else if (aCollector->mSocketType == ULA_SOCKET_TYPE_UNIX)
    {
        ACI_TEST_RAISE(cmiAllocLink(&sListenLink,
                                    CMI_LINK_TYPE_LISTEN,
                                    CMI_LINK_IMPL_UNIX) != ACI_SUCCESS,
                       ERR_LINK_ALLOC);
        sListenLinkAlloc = ACP_TRUE;

        sListenArg.mUNIX.mFilePath  = aCollector->mUNIXSocketFile;
        sListenArg.mUNIX.mMaxListen = 1;
    }
    else
    {
        ACI_RAISE(ERR_SOCKET_TYPE_NOT_SUPPORT);
    }

    ACI_TEST_RAISE(cmiListenLink(sListenLink, &sListenArg)
                   != ACI_SUCCESS, ERR_LINK_LISTEN);

    while ( 1 ) 
    {
        sRc = acpThrMutexLock( &aCollector->mAuthInfoMutex );
        ACI_TEST_RAISE( ACP_RC_NOT_SUCCESS( sRc ), ERR_MUTEX_AUTH_INFO_LOCK );
        sAuthInfoLock = ACP_TRUE;

        for ( sIndex = 0; sIndex < aCollector->mXLogSenderIPCount; sIndex++ )
        {
            /* 히든 프로퍼티라 ACI_TEST 로 에러를 알려준다.
             * AuthInfo 가 두개 이상일 때 하나라도 실패한다면
             * 전체가 실패할 것이다. */
            if ( ( aCollector->mXLogSenderIP[sIndex][0] != '\0' ) &&
                 ( aCollector->mXLogSenderPort[sIndex] > 0 ) )
            {
                ACI_TEST( ulaWakeupPeerSender( aCollector, 
                                               aErrorMgr, 
                                               aCollector->mXLogSenderIP[sIndex],
                                               aCollector->mXLogSenderPort[sIndex] )
                          != ACI_SUCCESS );
            }
            else
            {
                /* nothing to do */
            }
        }
        sAuthInfoLock = ACP_FALSE;
        sRc = acpThrMutexUnlock( &aCollector->mAuthInfoMutex );
        ACI_TEST_RAISE( ACP_RC_NOT_SUCCESS( sRc ), ERR_MUTEX_AUTH_INFO_UNLOCK );

        /* 접속 대기 및 Peer Link 생성 */
        if ( cmiWaitLink( sListenLink, sWaitTime ) == ACI_SUCCESS )
        {
            break;
        }
        else
        {
            ACI_TEST_RAISE( aciGetErrorCode() != cmERR_ABORT_TIMED_OUT,
                            ERR_LINK_WAIT );

            sTotalWaitTime += sWaitTime;
            ACI_TEST_RAISE( sTotalWaitTime >= sTimeout, ERR_TIMEOUT );
        }
    } 

    ACI_TEST_RAISE(cmiAcceptLink(sListenLink, &aCollector->mPeerLink)
                    != ACI_SUCCESS, ERR_LINK_ACCEPT);

    /* Initialize Protocol Context & Alloc CM Block */
    ACI_TEST( cmiMakeCmBlockNull( &(aCollector->mProtocolContext) ) != ACI_SUCCESS );
    ACI_TEST_RAISE( cmiAllocCmBlock( &(aCollector->mProtocolContext),
                                     CMI_PROTOCOL_MODULE( RP ),
                                     (cmiLink *)aCollector->mPeerLink,
                                     aCollector )
                    != ACI_SUCCESS, ERR_ALLOC_CM_BLOCK );
    aCollector->mSessionValid    = ACP_TRUE;
    aCollector->mNetworkExitFlag = ACP_FALSE;

    /* Authentication Information 검사 */
    if (aCollector->mSocketType == ULA_SOCKET_TYPE_TCP)
    {
        sRc = acpThrMutexLock(&aCollector->mAuthInfoMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_AUTH_INFO_LOCK);
        sAuthInfoLock = ACP_TRUE;

        // Authentication Information 얻기
        acpMemSet(sXLogSenderIP, 0x00, ULA_IP_LEN);
        (void)cmiGetLinkInfo(aCollector->mPeerLink, sXLogSenderIP, ULA_IP_LEN,
                             CMI_LINK_INFO_REMOTE_IP_ADDRESS);

        // 비교
        for (sIndex = 0; sIndex < aCollector->mXLogSenderIPCount; sIndex++)
        {
            if (acpCStrCaseCmp(aCollector->mXLogSenderIP[sIndex],
                               sXLogSenderIP,
                               ULA_IP_LEN) == 0)
            {
                break;
            }
        }
        ACI_TEST_RAISE(sIndex >= aCollector->mXLogSenderIPCount,
                       ERR_AUTH_FAILURE);

        sAuthInfoLock = ACP_FALSE;
        sRc = acpThrMutexUnlock(&aCollector->mAuthInfoMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_AUTH_INFO_UNLOCK);
    }
    /* Protocol Version 검사 및 Handshake ACK 전송 */
    ACI_TEST(ulaXLogCollectorCheckProtocol(aCollector,
                                           &sVersion,
                                           aErrorMgr)
             != ACI_SUCCESS);

    /* Meta 정보 수신 및 Handshake ACK 전송 */
    ACI_TEST_RAISE(ulaMetaRecvMeta( &aCollector->mMeta,
                                    &(aCollector->mProtocolContext),
                                    aCollector->mHandshakeTimeoutSec,
                                    sVersion,
                                    aCollector->mXLogSenderName,
                                    &sTableSize,
                                    aErrorMgr )
                   != ACI_SUCCESS, ERR_META_RECEIVE);

    if ( needToProcessProtocolOperation( ULA_META_COMPRESSTYPE,
                                         aCollector->mMeta.mReplication.mSenderVersion )
         == ACP_TRUE )
    {
        cmiDisableCompress( &(aCollector->mProtocolContext) );
        cmiSetDecompressType( &(aCollector->mProtocolContext), aCollector->mMeta.mReplication.mCompressType ); 
    }
    else
    {
        aCollector->mMeta.mReplication.mCompressType = CMI_COMPRESS_LZO;
    }        
        
    (void)ulaLogTrace( (ulaErrorMgr *)aErrorMgr, 
                       ULA_TRC_I_COMPRESS_TYPE, 
                       aCollector->mMeta.mReplication.mCompressType );
    
    ACI_TEST(ulaMetaSortMeta(&aCollector->mMeta, aErrorMgr)
             != ACI_SUCCESS);

    ACI_TEST(ulaCommSendHandshakeAck( &(aCollector->mProtocolContext),
                                      ULA_MSG_OK,
                                      sBuffer,
                                      aErrorMgr )
             != ACI_SUCCESS);

    sListenLinkAlloc = ACP_FALSE;
    ACI_TEST_RAISE(cmiFreeLink(sListenLink)
                   != ACI_SUCCESS, ERR_LINK_FREE);

    // PROJ-1663 : IMPLICIT SAVEPOINT SET
    ACE_DASSERT(aCollector->mRemainedXLog == NULL);
    aCollector->mRemainedXLog = NULL;

    /* Transaction Table 초기화 - 2 */
    ACI_TEST(ulaTransTblDestroy(&aCollector->mTransTbl, aErrorMgr)
             != ACI_SUCCESS);

    ACI_TEST(ulaTransTblInitialize(&aCollector->mTransTbl,
                                   sTableSize,
                                   aCollector->mUseCommittedTxBuffer,
                                   aErrorMgr)
             != ACI_SUCCESS);

    sReceiveLock = ACP_FALSE;
    sRc = acpThrMutexUnlock(&aCollector->mReceiveMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_RECEIVE_UNLOCK);

    sSendLock = ACP_FALSE;
    sRc = acpThrMutexUnlock(&aCollector->mSendMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_SEND_UNLOCK);

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_ALLOC_CM_BLOCK )
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_ALLOC_CM_BLOCK,
                        "handshake");
    }
    ACI_EXCEPTION(ERR_MUTEX_AUTH_INFO_LOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_AUTH_INFO_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_AUTH_INFO_UNLOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_AUTH_INFO_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_SEND_LOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_SEND_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_SEND_UNLOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_SEND_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_RECEIVE_LOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_RECEIVE_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_RECEIVE_UNLOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_RECEIVE_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_SOCKET_TYPE_NONE)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_SOCKET_TYPE_NONE);
    }
    ACI_EXCEPTION(ERR_SOCKET_TYPE_NOT_SUPPORT)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_SOCKET_TYPE_NOT_SUPPORT);
    }
    ACI_EXCEPTION(ERR_LINK_ALLOC)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_LINK_ALLOC);
    }
    ACI_EXCEPTION(ERR_LINK_WAIT)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_LINK_WAIT);
    }
    ACI_EXCEPTION(ERR_LINK_FREE)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_LINK_FREE);
    }
    ACI_EXCEPTION(ERR_LINK_LISTEN)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_LINK_LISTEN);
    }
    ACI_EXCEPTION(ERR_LINK_ACCEPT)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_LINK_ACCEPT);
    }
    ACI_EXCEPTION(ERR_META_RECEIVE)
    {
        // 이미 ulaSetErrorCode() 수행

        (void)acpSnprintf(sBuffer, ULA_ACK_MSG_LEN, "%s",
                          ulaGetErrorMessage(aErrorMgr));

        (void)ulaCommSendHandshakeAck( &(aCollector->mProtocolContext),
                                       ULA_MSG_META_DIFF,
                                       sBuffer,
                                       NULL );
    }
    ACI_EXCEPTION(ERR_TIMEOUT)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_NET_TIMEOUT,
                        "Dispatcher Select");
    }
    ACI_EXCEPTION(ERR_AUTH_FAILURE)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_AUTH_FAIL);
    }
    ACI_EXCEPTION_END;

    if (sListenLinkAlloc == ACP_TRUE)
    {
        (void)cmiFreeLink(sListenLink);
    }

    // 실패 시, Meta 정보 초기화
    ulaMetaDestroy(&aCollector->mMeta);
    ulaMetaInitialize(&aCollector->mMeta);

    if (sAuthInfoLock == ACP_TRUE)
    {
        (void)acpThrMutexUnlock(&aCollector->mAuthInfoMutex);
    }
    if (sReceiveLock == ACP_TRUE)
    {
        (void)acpThrMutexUnlock(&aCollector->mReceiveMutex);
    }
    if (sSendLock == ACP_TRUE)
    {
        (void)acpThrMutexUnlock(&aCollector->mSendMutex);
    }

    // 실패 시, 연결을 종료
    (void)ulaXLogCollectorFinishNetwork(aCollector, NULL);

    return ACI_FAILURE;
}

ACI_RC ulaXLogCollectorCheckRemoteVersion( cmiProtocolContext * aProtocolContext,
                                           acp_uint32_t         aTimeOut,
                                           ulaErrorMgr        * aErrorMgr,
                                           acp_char_t         * aMsg,
                                           acp_uint32_t       * aMsgLen )
{
    acp_uint32_t sResult = ULA_MSG_DISCONNECT; 
    acp_bool_t   sExitFlag = ACP_FALSE; /* Dummy */
    
    ACI_TEST_RAISE( ulaCommSendVersion( aProtocolContext, aErrorMgr ) 
                    != ACI_SUCCESS, ERR_SEND_VERSION );

    ACI_TEST_RAISE( ulaCommRecvHandshakeAck( aProtocolContext, 
                                             &( sExitFlag ),
                                             &sResult,
                                             aMsg,
                                             aMsgLen,
                                             aTimeOut,
                                             aErrorMgr ) 
                    != ACI_SUCCESS, ERR_RECV_HANDSHAKE );

    switch ( sResult )
    {
        case ULA_MSG_OK :
            break;

        case ULA_MSG_PROTOCOL_DIFF :
            ACI_RAISE( ERR_PROTOCOL_DIFF );
            break;

        default:
            ACI_RAISE( ERR_UNEXPECTED_PROTOCOL );
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_PROTOCOL_DIFF )
    {
        ulaSetErrorCode( aErrorMgr, ulaERR_ABORT_PROTOCOL_DIFF );
    }
    ACI_EXCEPTION( ERR_UNEXPECTED_PROTOCOL )
    {
        ulaSetErrorCode( aErrorMgr, ulaERR_ABORT_NET_UNEXPECTED_PROTOCOL );
    }
    ACI_EXCEPTION( ERR_SEND_VERSION );
    {
         ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_NET_WRITE );
    }
    ACI_EXCEPTION( ERR_RECV_HANDSHAKE );
    {
         ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_NET_READ );
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaXLogCollectorCheckProtocol(ulaXLogCollector *aCollector,
                                     ulaVersion       *aVersion,
                                     ulaErrorMgr      *aErrorMgr)
{
    acp_char_t *sInformation = NULL;
    acp_char_t  sBuffer[ULA_ACK_MSG_LEN];
    acp_bool_t  sDummy = ACP_FALSE;

    /* Protocol Version 수신 및 검사 */
    ACI_TEST(ulaCommRecvVersion( &(aCollector->mProtocolContext),
                                 aVersion,
                                 &sDummy,
                                 aCollector->mHandshakeTimeoutSec,
                                 aErrorMgr )
             != ACI_SUCCESS);

    // Check Replication Protocol Version
    ACI_TEST_RAISE(ULA_GET_MAJOR_VERSION(aVersion->mVersion)
                   != REPLICATION_MAJOR_VERSION, ERR_PROTOCOL);

    ACI_TEST_RAISE(ULA_GET_MINOR_VERSION(aVersion->mVersion)
                   != REPLICATION_MINOR_VERSION, ERR_PROTOCOL);

    /* Handshake ACK 전송 */
    acpMemSet(sBuffer, 0x00, ULA_ACK_MSG_LEN);
    ACI_TEST(ulaCommSendHandshakeAck( &(aCollector->mProtocolContext),
                                      ULA_MSG_OK,
                                      sBuffer,
                                      aErrorMgr )
             != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PROTOCOL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_PROTOCOL_DIFF);

        switch (REPLICATION_ENDIAN_64BIT)
        {
            case 1:
                sInformation = (acp_char_t *)"Big Endian, 64 Bit";
                break;

            case 2:
                sInformation = (acp_char_t *)"Big Endian, 32 Bit";
                break;

            case 3:
                sInformation = (acp_char_t *)"Little Endian, 64 Bit";
                break;

            case 4:
                sInformation = (acp_char_t *)"Little Endian, 32 Bit";
                break;

            default :
                sInformation = (acp_char_t *)"";
                break;
        }

        acpMemSet(sBuffer, 0x00, ULA_ACK_MSG_LEN);
        (void)acpSnprintf(sBuffer, ULA_ACK_MSG_LEN,
                          "Peer Replication Protocol Version is "
                          "[%d.%d.%d] "
                          "%s",
                          REPLICATION_MAJOR_VERSION,
                          REPLICATION_MINOR_VERSION,
                          REPLICATION_FIX_VERSION,
                          sInformation);

        (void)ulaCommSendHandshakeAck( &(aCollector->mProtocolContext),
                                       ULA_MSG_PROTOCOL_DIFF,
                                       sBuffer,
                                       NULL );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static acp_bool_t ulaXLogCollectorIsTransRelatedXLog(ulaXLogType aType)
{
    acp_bool_t sResult = ACP_FALSE;

    switch (aType)
    {
        case ULA_X_BEGIN :              // Not Used
        case ULA_X_COMMIT :
        case ULA_X_ABORT :
        case ULA_X_INSERT :
        case ULA_X_UPDATE :
        case ULA_X_DELETE :
        case ULA_X_IMPL_SP_SET :        // Not Used
        case ULA_X_SP_SET :
        case ULA_X_SP_ABORT :
        case ULA_X_STMT_BEGIN :         // Not Used
        case ULA_X_STMT_END :           // Not Used
        case ULA_X_CURSOR_OPEN :        // Not Used
        case ULA_X_CURSOR_CLOSE :       // Not Used
        case ULA_X_LOB_CURSOR_OPEN :
        case ULA_X_LOB_CURSOR_CLOSE :
        case ULA_X_LOB_PREPARE4WRITE :
        case ULA_X_LOB_PARTIAL_WRITE :
        case ULA_X_LOB_FINISH2WRITE :
            sResult = ACP_TRUE;
            break;

        default :
            break;
    }

    return sResult;
}

/*
 * -----------------------------------------------------------------------------
 *  Collecting Committed Transactions
 * -----------------------------------------------------------------------------
 */
// PROJ-1663 : BEGIN 패킷 미사용
static ACI_RC ulaXLogCollectorProcessCommittedTransBegin
                                        (ulaXLogCollector *aCollector,
                                         ulaXLog          *aXLog,
                                         acp_bool_t       *aOutNeedXLogFree,
                                         ulaErrorMgr      *aErrorMgr)
{
    ulaXLogLinkedList *sList;
    ulaXLog           *sListXLog = NULL;
    acp_bool_t         sMutexLock = ACP_FALSE;

    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutNeedXLogFree == NULL, ERR_PARAMETER_NULL);

    *aOutNeedXLogFree = ACP_TRUE;

    ACI_TEST(ulaTransTblLockCollectionList(&aCollector->mTransTbl, aErrorMgr)
             != ACI_SUCCESS);
    sMutexLock = ACP_TRUE;

    sList = ulaTransTblGetCollectionList(&aCollector->mTransTbl,
                                         aXLog->mHeader.mTID);
    ACI_TEST_RAISE(sList == NULL, ERR_GET_COLLECTION_LIST);

    ACI_TEST(ulaXLogLinkedListPeepHead(sList, &sListXLog, aErrorMgr)
             != ACI_SUCCESS);

    /* 수집 Linked List에 XLog가 있으면, 치명적인 오류로 처리합니다. */
    ACI_TEST_RAISE(sListXLog != NULL, ERR_XLOG_ALREADY_EXIST);

    /* 수집 Linked List의 마지막에 추가합니다. */
    ACI_TEST(ulaXLogLinkedListInsertToTail(sList, aXLog, aErrorMgr)
             != ACI_SUCCESS);
    *aOutNeedXLogFree = ACP_FALSE;

    sMutexLock = ACP_FALSE;
    ACI_TEST(ulaTransTblUnlockCollectionList(&aCollector->mTransTbl,
                                             aErrorMgr)
             != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_GET_COLLECTION_LIST)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_NOT_INIT_TXTABLE);
    }
    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Committed Transaction BEGIN");
    }
    ACI_EXCEPTION(ERR_XLOG_ALREADY_EXIST)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_TX_ALREADY_BEGIN,
                        sListXLog->mHeader.mTID,
                        aXLog->mHeader.mTID);

        ACE_DASSERT(sListXLog->mHeader.mType == ULA_X_BEGIN);
    }
    ACI_EXCEPTION_END;

    if (sMutexLock == ACP_TRUE)
    {
        (void)ulaTransTblUnlockCollectionList(&aCollector->mTransTbl,
                                              aErrorMgr);
    }

    return ACI_FAILURE;
}

static ACI_RC ulaXLogCollectorProcessCommittedTransAbortToSP
                                        (ulaXLogCollector *aCollector,
                                         ulaXLog          *aXLog,
                                         acp_bool_t       *aOutNeedXLogFree,
                                         ulaErrorMgr      *aErrorMgr)
{
    ulaXLogLinkedList *sList;
    ulaXLog           *sListXLog = NULL;
    acp_bool_t         sMutexLock = ACP_FALSE;

    ulaXLogLinkedList  sTempList;
    acp_bool_t         sPsmSvpLog = ACP_FALSE;
    acp_bool_t         sTempListInit = ACP_FALSE;


    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutNeedXLogFree == NULL, ERR_PARAMETER_NULL);

    /* BUG-28988 앞에서 헤더타입이 ULA_X_SP_ABORT임을 확인했으므로
     * mSPName은 NULL이 되면 안된다.*/
    ACI_TEST_RAISE(aXLog->mSavepoint.mSPName == NULL, ERR_PARAMETER_INVALID);

    *aOutNeedXLogFree = ACP_TRUE;

    ACI_TEST(ulaTransTblLockCollectionList(&aCollector->mTransTbl, aErrorMgr)
                   != ACI_SUCCESS);
    sMutexLock = ACP_TRUE;

    sList = ulaTransTblGetCollectionList(&aCollector->mTransTbl, 
                                         aXLog->mHeader.mTID);
    ACI_TEST_RAISE(sList == NULL, ERR_GET_COLLECTION_LIST);

    ACI_TEST(ulaXLogLinkedListPeepHead(sList, &sListXLog, aErrorMgr)
             != ACI_SUCCESS);

    //임시 리스트 초기화
    ACI_TEST(ulaXLogLinkedListInitialize(&sTempList, ACP_FALSE, aErrorMgr)
             != ACI_SUCCESS);
    sTempListInit = ACP_TRUE;

    if (sListXLog != NULL)
    {
        /* 수집 Linked List의 최초 XLog TID와 TID가 같으면, */
        if (aXLog->mHeader.mTID == sListXLog->mHeader.mTID)
        {
            /* 수집 Linked List의 마지막에서 해당 Savepoint까지
             * XLog를 XLog Pool에 반환합니다.
             */
            if (acpCStrCaseCmp(ULA_PSM_SAVEPOINT_NAME,
                               aXLog->mSavepoint.mSPName,
                               9) == 0)
            {
                sPsmSvpLog = ACP_TRUE;
            }

            do
            {
                ACI_TEST(ulaXLogLinkedListRemoveFromTail(sList,
                                                         &sListXLog,
                                                         aErrorMgr)
                         != ACI_SUCCESS);

                if (sListXLog != NULL)
                {
                    if ((sListXLog->mHeader.mType == ULA_X_IMPL_SP_SET) ||
                        (sListXLog->mHeader.mType == ULA_X_SP_SET))
                    {
                        /* BUG-21920 : PSM ROLLBACK처리시 만약에 프로시져 
                         * 내부에 Savepoint들이 있을 경우 지우지 않고 
                         * 임시 리스트에 저장 시킨다 (if문)
                         * 일반 Explicit Rollback시에 PSM Savepoint가 있을시에도
                         * 임시 리스트에 저장시킨다  (else문)
                         */
                        if (sPsmSvpLog == ACP_TRUE)
                        {
                            if (acpCStrCaseCmp(ULA_PSM_SAVEPOINT_NAME,
                                               sListXLog->mSavepoint.mSPName,
                                               9) != 0)
                            {
                                // PSM_SP가 아닌 Savepoint는 
                                // 임시 리스트에 저장 시킨다.
                                ACI_TEST(ulaXLogLinkedListInsertToHead
                                                                (&sTempList,
                                                                 sListXLog,
                                                                 aErrorMgr)
                                         != ACI_SUCCESS);
                                continue;
                            }
                            else
                            {
                                ACI_TEST(ulaXLogLinkedListInsertToTail
                                                                (sList,
                                                                 sListXLog,
                                                                 aErrorMgr)
                                         != ACI_SUCCESS);
                                break;
                            }
                        }
                        else
                        {
                            if (acpCStrCaseCmp(sListXLog->mSavepoint.mSPName,
                                               aXLog->mSavepoint.mSPName,
                                               40) == 0)
                            {
                                /* BUG-21805 : Savepoint로 Rollback시, 
                                 * Savepoiint까지 지우는 문제
                                 *
                                 * Savepoint까지 Rollback시 List에서 
                                 * 역순으로 지우는데 자기가 원하는 
                                 * Savepoint일 경우에는 InsertToTail을 통해서
                                 * 다시 List에 넣어준다
                                 */
                                ACI_TEST(ulaXLogLinkedListInsertToTail
                                                                (sList,
                                                                 sListXLog,
                                                                 aErrorMgr)
                                         != ACI_SUCCESS);
                                break;

                            }
                            else if (acpCStrCaseCmp
                                            (ULA_PSM_SAVEPOINT_NAME,
                                             sListXLog->mSavepoint.mSPName,
                                             9) == 0)
                            {
                                // PSM Saveponit는 지우지 않고 
                                // 임시 리스트에 저장한다
                                ACI_TEST(ulaXLogLinkedListInsertToHead
                                                                (&sTempList,
                                                                 sListXLog,
                                                                 aErrorMgr)
                                         != ACI_SUCCESS);
                                continue;
                            }
                        }
                    }

                    ACI_TEST(ulaXLogCollectorFreeXLogMemory(aCollector,
                                                            sListXLog,
                                                            aErrorMgr)
                             != ACI_SUCCESS);
                }
            } while (sListXLog != NULL);

            /* BUG-21920 : 임시 저장 리스트에 Savepoint들이 있으면
             * 기존 리스트의  맨 뒤에 다시 저장
             */
            do
            {
                ACI_TEST(ulaXLogLinkedListRemoveFromHead(&sTempList,
                                                         &sListXLog,
                                                         aErrorMgr)
                               != ACI_SUCCESS);

                if (sListXLog != NULL)
                {
                    ACI_TEST(ulaXLogLinkedListInsertToTail(sList,
                                                           sListXLog,
                                                           aErrorMgr)
                             != ACI_SUCCESS);
                }
            }
            while (sListXLog != NULL);
        }
    }

    //임시 리스트 해제
    sTempListInit = ACP_FALSE;
    ACI_TEST(ulaXLogLinkedListDestroy(&sTempList, aErrorMgr) != ACI_SUCCESS);

    sMutexLock = ACP_FALSE;
    ACI_TEST(ulaTransTblUnlockCollectionList(&aCollector->mTransTbl,
                                             aErrorMgr)
             != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_GET_COLLECTION_LIST)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_NOT_INIT_TXTABLE);
    }
    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Committed Transaction ABORT_TO_SP");
    }
    ACI_EXCEPTION(ERR_PARAMETER_INVALID)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_INVALID,
                        "Committed Transaction ABORT_TO_SP : "
                        "Savepoint name is empty.");
    }
    ACI_EXCEPTION_END;

    if (sTempListInit == ACP_TRUE)
    {
        (void)ulaXLogLinkedListDestroy(&sTempList, NULL);
    }
    if (sMutexLock == ACP_TRUE)
    {
        (void)ulaTransTblUnlockCollectionList(&aCollector->mTransTbl, NULL);
    }

    return ACI_FAILURE;
}

static ACI_RC ulaXLogCollectorProcessCommittedTransAbort
                                           (ulaXLogCollector *aCollector,
                                            ulaXLog          *aXLog,
                                            acp_bool_t       *aOutNeedXLogFree,
                                            ulaErrorMgr      *aErrorMgr)
{
    ulaXLogLinkedList *sList;
    ulaXLog           *sListXLog = NULL;
    acp_bool_t         sMutexLock = ACP_FALSE;

    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutNeedXLogFree == NULL, ERR_PARAMETER_NULL);

    *aOutNeedXLogFree = ACP_TRUE;

    ACI_TEST(ulaTransTblLockCollectionList(&aCollector->mTransTbl, aErrorMgr)
             != ACI_SUCCESS);
    sMutexLock = ACP_TRUE;

    sList = ulaTransTblGetCollectionList(&aCollector->mTransTbl,
                                         aXLog->mHeader.mTID);
    ACI_TEST_RAISE(sList == NULL, ERR_GET_COLLECTION_LIST);

    ACI_TEST(ulaXLogLinkedListPeepHead(sList, &sListXLog, aErrorMgr)
             != ACI_SUCCESS);

    if (sListXLog != NULL)
    {
        /* 수집 Linked List의 최초 XLog TID와 TID가 같으면, */
        if (aXLog->mHeader.mTID == sListXLog->mHeader.mTID)
        {
            /* 수집 Linked List의 모든 XLog를 XLog Pool에 반환합니다. */
            ACI_TEST(ulaXLogCollectorFreeXLogFromLinkedList(aCollector,
                                                            sList,
                                                            aErrorMgr)
                     != ACI_SUCCESS);
        }
    }

    sMutexLock = ACP_FALSE;
    ACI_TEST(ulaTransTblUnlockCollectionList(&aCollector->mTransTbl,
                                             aErrorMgr)
             != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_GET_COLLECTION_LIST)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_NOT_INIT_TXTABLE);
    }
    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Committed Transaction ABORT");
    }
    ACI_EXCEPTION_END;

    if (sMutexLock == ACP_TRUE)
    {
        (void)ulaTransTblUnlockCollectionList(&aCollector->mTransTbl, NULL);
    }

    return ACI_FAILURE;
}

static ACI_RC ulaXLogCollectorProcessCommittedTransCommit
                                         (ulaXLogCollector *aCollector,
                                          ulaXLog          *aXLog,
                                          acp_bool_t       *aOutNeedXLogFree,
                                          ulaErrorMgr      *aErrorMgr)
{
    ulaXLogLinkedList *sList;
    ulaXLog           *sListXLog = NULL;
    acp_bool_t         sMutexLock = ACP_FALSE;

    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutNeedXLogFree == NULL, ERR_PARAMETER_NULL);

    *aOutNeedXLogFree = ACP_TRUE;

    ACI_TEST(ulaTransTblLockCollectionList(&aCollector->mTransTbl,
                                           aErrorMgr)
             != ACI_SUCCESS);
    sMutexLock = ACP_TRUE;

    sList = ulaTransTblGetCollectionList(&aCollector->mTransTbl,
                                         aXLog->mHeader.mTID);
    ACI_TEST_RAISE(sList == NULL, ERR_GET_COLLECTION_LIST);

    ACI_TEST(ulaXLogLinkedListPeepHead(sList, &sListXLog, aErrorMgr)
             != ACI_SUCCESS);

    if (sListXLog != NULL)
    {
        /* 수집 Linked List의 최초 XLog TID와 TID가 같으면, */
        if (aXLog->mHeader.mTID == sListXLog->mHeader.mTID)
        {
            /* 수집 Linked List의 마지막에 추가합니다. */
            ACI_TEST(ulaXLogLinkedListInsertToTail(sList,
                                                   aXLog,
                                                   aErrorMgr)
                     != ACI_SUCCESS);
            *aOutNeedXLogFree = ACP_FALSE;

            do
            {
                ACI_TEST(ulaXLogLinkedListRemoveFromHead(sList,
                                                         &sListXLog, 
                                                         aErrorMgr)
                               != ACI_SUCCESS);

                if (sListXLog != NULL)
                {
                    ACE_DASSERT(sListXLog->mHeader.mType != ULA_X_SP_ABORT);

                    /* Savepoint 관련 XLog를 XLog Pool에 반환합니다. */
                    if ((sListXLog->mHeader.mType == ULA_X_IMPL_SP_SET) ||
                        (sListXLog->mHeader.mType == ULA_X_SP_SET))
                    {
                        ACI_TEST(ulaXLogCollectorFreeXLogMemory(aCollector,
                                                                sListXLog,
                                                                aErrorMgr)
                                 != ACI_SUCCESS);
                    }
                    /* 수집 Linked List의 XLog를 XLog Queue로 이동시킵니다. */
                    else
                    {
                        ACI_TEST_RAISE(ulaXLogLinkedListInsertToTail
                                                    (&aCollector->mXLogQueue,
                                                     sListXLog,
                                                     aErrorMgr)
                                       != ACI_SUCCESS,
                                       ERR_XLOG_QUEUE_INSERT2);
                    }
                }
            } while (sListXLog != NULL);
        }
    }

    sMutexLock = ACP_FALSE;
    ACI_TEST(ulaTransTblUnlockCollectionList(&aCollector->mTransTbl,
                                             aErrorMgr)
             != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_GET_COLLECTION_LIST)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_NOT_INIT_TXTABLE);
    }
    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Committed Transaction COMMIT");
    }
    ACI_EXCEPTION(ERR_XLOG_QUEUE_INSERT2)
    {
        // 이미 ulaSetErrorCode() 수행

        (void)ulaXLogCollectorFreeXLogMemory(aCollector, sListXLog, NULL);
    }
    ACI_EXCEPTION_END;

    if (sMutexLock == ACP_TRUE)
    {
        (void)ulaTransTblUnlockCollectionList(&aCollector->mTransTbl, NULL);
    }

    return ACI_FAILURE;
}

static ACI_RC ulaXLogCollectorProcessCommittedTransOthers
                                            (ulaXLogCollector *aCollector,
                                             ulaXLog          *aXLog,
                                             acp_bool_t       *aOutNeedXLogFree,
                                             ulaErrorMgr      *aErrorMgr)
{
    ulaXLogLinkedList *sList;
    ulaXLog           *sListXLog = NULL;
    acp_bool_t         sMutexLock = ACP_FALSE;

    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);
    ACI_TEST_RAISE(aOutNeedXLogFree == NULL, ERR_PARAMETER_NULL);

    *aOutNeedXLogFree = ACP_TRUE;

    ACI_TEST(ulaTransTblLockCollectionList(&aCollector->mTransTbl,
                                           aErrorMgr)
             != ACI_SUCCESS);
    sMutexLock = ACP_TRUE;

    sList = ulaTransTblGetCollectionList(&aCollector->mTransTbl,
                                         aXLog->mHeader.mTID);
    ACI_TEST_RAISE(sList == NULL, ERR_GET_COLLECTION_LIST);

    ACI_TEST(ulaXLogLinkedListPeepHead(sList, &sListXLog, aErrorMgr)
             != ACI_SUCCESS);

    if (sListXLog != NULL)
    {
        /* 수집 Linked List의 최초 XLog TID와 TID가 같으면, */
        if (aXLog->mHeader.mTID == sListXLog->mHeader.mTID)
        {
            /* 수집 Linked List의 마지막에 추가합니다. */
            ACI_TEST(ulaXLogLinkedListInsertToTail(sList, aXLog, aErrorMgr)
                           != ACI_SUCCESS);
            *aOutNeedXLogFree = ACP_FALSE;
        }
    }
    else    // PROJ-1663 : BEGIN 패킷 미사용 처리
    {
        /* 수집 Linked List의 마지막에 추가합니다. */
        ACI_TEST(ulaXLogLinkedListInsertToTail(sList, aXLog, aErrorMgr)
                 != ACI_SUCCESS);
        *aOutNeedXLogFree = ACP_FALSE;
    }

    sMutexLock = ACP_FALSE;
    ACI_TEST(ulaTransTblUnlockCollectionList(&aCollector->mTransTbl, aErrorMgr)
             != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_GET_COLLECTION_LIST)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_NOT_INIT_TXTABLE);
    }
    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Committed Transaction Others");
    }
    ACI_EXCEPTION_END;

    if (sMutexLock == ACP_TRUE)
    {
        (void)ulaTransTblUnlockCollectionList(&aCollector->mTransTbl, NULL);
    }

    return ACI_FAILURE;
}

static ACI_RC ulaXLogConvertLOBColumn( ulaXLogCollector *aCollector,
                                       ulaXLog          *aXLog,
                                       ulaErrorMgr      *aErrorMgr) 
{
    acp_uint32_t    i = 0;
    acp_uint64_t    sTableOID;
    acp_uint32_t    sColumnCount = 0;
    ulaTable        *sTable = NULL;
    ulaColumn       *sColumnInfo = NULL;
    ulaXLogColumn   *sColumn = NULL;

    sTableOID = aXLog->mHeader.mTableOID;
    sColumnCount = aXLog->mColumn.mColCnt;
    sColumn = &(aXLog->mColumn);

    ACI_TEST(ulaMetaGetTableInfo( &aCollector->mMeta, 
                                  sTableOID,
                                  &sTable,
                                  aErrorMgr )
             != ACI_SUCCESS);
    ACI_TEST_RAISE( sTable == NULL, ERR_TABLE_NOT_FOUND );

    for ( i = 0; i < sColumnCount; i++ )
    {
        ACI_TEST( ulaMetaGetColumnInfo(sTable,
                                       sColumn->mCIDArray[i],
                                       &sColumnInfo,
                                       aErrorMgr )
                  != ACI_SUCCESS );
        ACI_TEST_RAISE( sColumnInfo == NULL, ERR_COLUMN_NOT_FOUND );

        if ( ( sColumnInfo->mDataType == MTD_BLOB_ID ) ||
             ( sColumnInfo->mDataType == MTD_CLOB_ID ) )
        {
            /* make correct real size about lob data*/
            if ( sColumn->mBColArray != NULL )
            {
                if ( sColumn->mBColArray[i].length > 0 )
                {
                    sColumn->mBColArray[i].length -= ULA_LOB_DUMMY_HEADER_LEN;
                    sColumn->mBColArray[i].value = 
                        (acp_char_t *)sColumn->mBColArray[i].value + ULA_LOB_DUMMY_HEADER_LEN;
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

            if ( sColumn->mAColArray != NULL )
            {
                if ( sColumn->mAColArray[i].length > 0 )
                {
                    sColumn->mAColArray[i].length -= ULA_LOB_DUMMY_HEADER_LEN;
                    sColumn->mAColArray[i].value = 
                        (acp_char_t *)sColumn->mAColArray[i].value + ULA_LOB_DUMMY_HEADER_LEN;
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
        }
        else
        {
            /* do nothing */
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_TABLE_NOT_FOUND )
    {
        ulaSetErrorCode( aErrorMgr, ulaERR_ABORT_TABLE_NOT_FOUND,
                         "ulaXLogConvertLOBColumn",
                         aXLog->mHeader.mTableOID );
    }
    ACI_EXCEPTION( ERR_COLUMN_NOT_FOUND )
    {
        ulaSetErrorCode( aErrorMgr, ulaERR_ABORT_COLUMN_NOT_FOUND,
                         "ulaXLogConvertLOBColumn",
                         aXLog->mColumn.mCIDArray[i] );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
/*
 * -----------------------------------------------------------------------------
 *  XLog Communication
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogCollectorReceiveXLog(ulaXLogCollector *aCollector,
                                   acp_bool_t       *aOutInsertXLogInQueue,
                                   ulaErrorMgr      *aErrorMgr)
{
    acp_rc_t      sRc;

    ulaXLog      *sXLog = NULL;
    acp_sint32_t  sXLogFreeCount;
    acp_uint32_t  sErrorCode;
    acp_bool_t    sNeedXLogFree = ACP_FALSE;

    acp_bool_t    sReceiveLock = ACP_FALSE;
    acp_bool_t    sAckLock = ACP_FALSE;

    ACI_TEST_RAISE(aOutInsertXLogInQueue == NULL, ERR_PARAMETER_NULL);
    *aOutInsertXLogInQueue = ACP_FALSE;

    sRc = acpThrMutexLock(&aCollector->mReceiveMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_RECEIVE_LOCK);
    sReceiveLock = ACP_TRUE;

    ACI_TEST_RAISE(aCollector->mNetworkExitFlag == ACP_TRUE, ERR_NET_INVALID);

    /* XLog Pool에서 XLog를 얻습니다. */
    ACI_TEST(ulaXLogCollectorAllocXLogMemory(aCollector,
                                             &sXLog,
                                             &sXLogFreeCount,
                                             aErrorMgr)
             != ACI_SUCCESS);
    sNeedXLogFree = ACP_TRUE;

    ACI_TEST_RAISE(sXLog == NULL, ERR_XLOG_POOL_EMPTY);
    ACE_ASSERT(sXLogFreeCount >= 0);

process_next_xlog :

    /* 이전에 수신한 XLog를 처리합니다. */
    if (aCollector->mRemainedXLog != NULL)
    {
        sXLog         = aCollector->mRemainedXLog;
        sNeedXLogFree = ACP_TRUE;

        aCollector->mRemainedXLog = NULL;
    }
    /* XLog를 수신합니다. */
    else if (ulaCommRecvXLog(&aCollector->mProtocolContext,
                             &aCollector->mNetworkExitFlag,
                             sXLog,
                             aCollector->mMemAllocator,
                             aCollector->mReceiveXLogTimeoutSec,
                             aErrorMgr)
             != ACI_SUCCESS)
    {
        sErrorCode = ulaGetErrorCode(aErrorMgr);
        ACI_TEST((sErrorCode == ulaERR_ABORT_NET_EXIT) ||
                 (sErrorCode == ulaERR_IGNORE_NET_TIMEOUT));

        ACI_RAISE(ERR_NET_READ);
    }

    // PROJ-1663 : IMPLICIT SAVEPOINT SET 패킷 처리
    if ((sXLog->mSavepoint.mSPName != NULL) &&
        (sXLog->mHeader.mType != ULA_X_SP_SET) &&
        (sXLog->mHeader.mType != ULA_X_IMPL_SP_SET) &&
        (sXLog->mHeader.mType != ULA_X_SP_ABORT))
    {
        // 다음에 처리할 XLog를 설정합니다.
        aCollector->mRemainedXLog = sXLog;
        sXLog                     = NULL;
        sNeedXLogFree             = ACP_FALSE;

        // XLog Pool에서 XLog를 얻습니다.
        ACI_TEST(ulaXLogCollectorAllocXLogMemory(aCollector,
                                                 &sXLog,
                                                 &sXLogFreeCount,
                                                 aErrorMgr)
                 != ACI_SUCCESS);
        sNeedXLogFree = ACP_TRUE;

        ACI_TEST_RAISE(sXLog == NULL, ERR_XLOG_POOL_EMPTY);
        ACE_ASSERT(sXLogFreeCount >= 0);

        // SP_SET XLog를 설정합니다.
        sXLog->mHeader.mType   = ULA_X_SP_SET;
        sXLog->mHeader.mTID    = aCollector->mRemainedXLog->mHeader.mTID;
        sXLog->mHeader.mSN     = aCollector->mRemainedXLog->mHeader.mSN;
        sXLog->mHeader.mSyncSN = aCollector->mRemainedXLog->mHeader.mSyncSN;

        sXLog->mSavepoint.mSPName    
                        = aCollector->mRemainedXLog->mSavepoint.mSPName;
        sXLog->mSavepoint.mSPNameLen 
                        = aCollector->mRemainedXLog->mSavepoint.mSPNameLen;

        // 다음에 처리할 XLog에서 IMPLICIT SAVEPOINT를 제거합니다.
        aCollector->mRemainedXLog->mSavepoint.mSPName    = NULL;
        aCollector->mRemainedXLog->mSavepoint.mSPNameLen = 0;
    }

    /* Last Arrived SN을 설정합니다. */
    sRc = acpThrMutexLock(&aCollector->mAckMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_ACK_LOCK);
    sAckLock = ACP_TRUE;

    if (sXLog->mHeader.mSN != ULA_SN_NULL)
    {
        aCollector->mLastArrivedSN = sXLog->mHeader.mSN;
    }

    sAckLock = ACP_FALSE;
    sRc = acpThrMutexUnlock(&aCollector->mAckMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_ACK_UNLOCK);

    if ( ( sXLog->mHeader.mType == ULA_X_INSERT ) ||
         ( sXLog->mHeader.mType == ULA_X_UPDATE ) || 
         ( sXLog->mHeader.mType == ULA_X_DELETE ) )
    {
        ACI_TEST( ulaXLogConvertLOBColumn( aCollector,
                                           sXLog,
                                           aErrorMgr )
                  != ACI_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    /* XLog를 XLog Queue나 수집 Linked List에 추가합니다. */
    // Committed Transaction 미수집를 처리합니다.
    if (aCollector->mUseCommittedTxBuffer != ACP_TRUE)
    {
        ACI_TEST(ulaXLogLinkedListInsertToTail(&aCollector->mXLogQueue,
                                               sXLog,
                                               aErrorMgr)
                 != ACI_SUCCESS);
        *aOutInsertXLogInQueue = ACP_TRUE;
    }
    // Transaction 관련 XLog가 아닌 경우를 처리합니다.
    else if (ulaXLogCollectorIsTransRelatedXLog(sXLog->mHeader.mType)
             != ACP_TRUE)
    {
        ACI_TEST(ulaXLogLinkedListInsertToTail(&aCollector->mXLogQueue,
                                               sXLog,
                                               aErrorMgr)
                       != ACI_SUCCESS);
        *aOutInsertXLogInQueue = ACP_TRUE;
    }
    // Transaction 관련 XLog를 처리합니다. (Committed Transaction 수집)
    else
    {
        switch (sXLog->mHeader.mType)
        {
            // PROJ-1663 : BEGIN 패킷 미사용
            case ULA_X_BEGIN :
                ACI_TEST(ulaXLogCollectorProcessCommittedTransBegin
                                                             (aCollector,
                                                              sXLog,
                                                              &sNeedXLogFree,
                                                              aErrorMgr)
                         != ACI_SUCCESS);
                break;

            case ULA_X_SP_ABORT :
                ACI_TEST(ulaXLogCollectorProcessCommittedTransAbortToSP
                                                            (aCollector,
                                                             sXLog,
                                                             &sNeedXLogFree,
                                                             aErrorMgr)
                         != ACI_SUCCESS);
                break;

            case ULA_X_ABORT :
                ACI_TEST(ulaXLogCollectorProcessCommittedTransAbort
                                                            (aCollector,
                                                             sXLog,
                                                             &sNeedXLogFree,
                                                             aErrorMgr)
                         != ACI_SUCCESS);
                break;

            case ULA_X_COMMIT :
                ACI_TEST(ulaXLogCollectorProcessCommittedTransCommit
                                                            (aCollector,
                                                             sXLog,
                                                             &sNeedXLogFree,
                                                             aErrorMgr)
                         != ACI_SUCCESS);
                if (sNeedXLogFree != ACP_TRUE)
                {
                    *aOutInsertXLogInQueue = ACP_TRUE;
                }
                break;

            default :
                ACI_TEST(ulaXLogCollectorProcessCommittedTransOthers
                                                            (aCollector,
                                                             sXLog,
                                                             &sNeedXLogFree,
                                                             aErrorMgr)
                         != ACI_SUCCESS);
                break;
        }

        // XLog Queue나 수집 Linked List에 추가하지 않은 XLog를 
        // XLog Pool에 반환합니다.
        if (sNeedXLogFree != ACP_FALSE)
        {
            ACI_TEST(ulaXLogCollectorFreeXLogMemory(aCollector,
                                                    sXLog,
                                                    aErrorMgr)
                     != ACI_SUCCESS);
        }
    }
    sNeedXLogFree = ACP_FALSE;

    if (aCollector->mRemainedXLog != NULL)
    {
        goto process_next_xlog;
    }

    sReceiveLock = ACP_FALSE;
    sRc = acpThrMutexUnlock(&aCollector->mReceiveMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_RECEIVE_UNLOCK);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_MUTEX_RECEIVE_LOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_RECEIVE_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_RECEIVE_UNLOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_RECEIVE_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_ACK_LOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_ACK_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_ACK_UNLOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_ACK_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "XLog Receive Check");
    }
    ACI_EXCEPTION(ERR_XLOG_POOL_EMPTY)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_INSUFFICIENT_XLOG_POOL );
    }
    ACI_EXCEPTION(ERR_NET_INVALID)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_NET_EXIT,
                        "XLog Receive Check");
    }
    ACI_EXCEPTION(ERR_NET_READ)
    {
        // 이미 ulaSetErrorCode() 수행

        aCollector->mNetworkExitFlag = ACP_TRUE;
    }
    ACI_EXCEPTION_END;

    if ((sXLog != NULL) && (sNeedXLogFree != ACP_FALSE))
    {
        (void)ulaXLogCollectorFreeXLogMemory(aCollector, sXLog, NULL);
    }

    if (sAckLock == ACP_TRUE)
    {
        (void)acpThrMutexUnlock(&aCollector->mAckMutex);
    }
    if (sReceiveLock == ACP_TRUE)
    {
        (void)acpThrMutexUnlock(&aCollector->mReceiveMutex);
    }

    return ACI_FAILURE;
}

/*
 * -----------------------------------------------------------------------------
 *  Endian Conversion
 * -----------------------------------------------------------------------------
 */
static ACI_RC ulaXLogCollectorConvertEndianPK(ulaXLogCollector *aCollector,
                                              ulaXLog          *aXLog,
                                              ulaErrorMgr      *aErrorMgr)
{
    acp_uint32_t   i;
    ulaTable      *sTable = NULL;
    ulaColumn     *sColumn = NULL;

    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);

    ACI_TEST(ulaMetaGetTableInfo(&aCollector->mMeta,
                                 aXLog->mHeader.mTableOID,
                                 &sTable,
                                 aErrorMgr)
             != ACI_SUCCESS);
    ACI_TEST_RAISE(sTable == NULL, ERR_TABLE_NOT_FOUND);

    for (i = 0; i < aXLog->mPrimaryKey.mPKColCnt; i++)
    {
        sColumn = sTable->mPKColumnArray[i];
        ACI_TEST_RAISE(sColumn == NULL, ERR_PK_COLUMN_NOT_FOUND);

        if (aXLog->mPrimaryKey.mPKColArray[i].value != NULL)
        {
            ACI_TEST(ulaConvertEndianValue
                                    (sColumn->mDataType,
                                     sColumn->mEncrypt,
                                     aXLog->mPrimaryKey.mPKColArray[i].value,
                                     aErrorMgr)
                     != ACI_SUCCESS);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Endian Convert (Delete)");
    }
    ACI_EXCEPTION(ERR_TABLE_NOT_FOUND)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_TABLE_NOT_FOUND,
                        "Endian Convert (Delete)",
                        aXLog->mHeader.mTableOID);
    }
    ACI_EXCEPTION(ERR_PK_COLUMN_NOT_FOUND)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_COLUMN_NOT_FOUND,
                        "Endian Convert (Delete)",
                        sTable->mPKColumnArray[i]->mColumnID);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulaXLogCollectorConvertEndianUpdate(ulaXLogCollector *aCollector,
                                                  ulaXLog          *aXLog,
                                                  ulaErrorMgr      *aErrorMgr)
{
    acp_uint32_t  i;
    ulaTable     *sTable = NULL;
    ulaColumn    *sColumn = NULL;

    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);

    ACI_TEST(ulaMetaGetTableInfo(&aCollector->mMeta,
                                 aXLog->mHeader.mTableOID,
                                 &sTable,
                                 aErrorMgr)
             != ACI_SUCCESS);
    ACI_TEST_RAISE(sTable == NULL, ERR_TABLE_NOT_FOUND);

    for (i = 0; i < aXLog->mPrimaryKey.mPKColCnt; i++)
    {
        sColumn = sTable->mPKColumnArray[i];
        ACI_TEST_RAISE(sColumn == NULL, ERR_PK_COLUMN_NOT_FOUND);

        if (aXLog->mPrimaryKey.mPKColArray[i].value != NULL)
        {
            ACI_TEST(ulaConvertEndianValue
                                (sColumn->mDataType,
                                 sColumn->mEncrypt,
                                 aXLog->mPrimaryKey.mPKColArray[i].value,
                                 aErrorMgr)
                     != ACI_SUCCESS);
        }
    }

    for (i = 0; i < aXLog->mColumn.mColCnt; i++)
    {
        ACI_TEST(ulaMetaGetColumnInfo(sTable,
                                      aXLog->mColumn.mCIDArray[i],
                                      &sColumn,
                                      aErrorMgr)
                 != ACI_SUCCESS);
        ACI_TEST_RAISE(sColumn == NULL, ERR_COLUMN_NOT_FOUND);

        if (aXLog->mColumn.mBColArray[i].value != NULL)
        {
            ACI_TEST(ulaConvertEndianValue(sColumn->mDataType,
                                           sColumn->mEncrypt,
                                           aXLog->mColumn.mBColArray[i].value,
                                           aErrorMgr)
                     != ACI_SUCCESS);
        }

        if (aXLog->mColumn.mAColArray[i].value != NULL)
        {
            ACI_TEST(ulaConvertEndianValue(sColumn->mDataType,
                                           sColumn->mEncrypt,
                                           aXLog->mColumn.mAColArray[i].value,
                                           aErrorMgr)
                     != ACI_SUCCESS);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Endian Convert (Update)");
    }
    ACI_EXCEPTION(ERR_TABLE_NOT_FOUND)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_TABLE_NOT_FOUND,
                        "Endian Convert (Update)",
                        aXLog->mHeader.mTableOID);
    }
    ACI_EXCEPTION(ERR_PK_COLUMN_NOT_FOUND)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_COLUMN_NOT_FOUND,
                        "Endian Convert (Update)",
                        sTable->mPKColumnArray[i]->mColumnID);
    }
    ACI_EXCEPTION(ERR_COLUMN_NOT_FOUND)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_COLUMN_NOT_FOUND,
                        "Endian Convert (Update)",
                        aXLog->mColumn.mCIDArray[i]);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulaXLogCollectorConvertEndianInsert(ulaXLogCollector *aCollector,
                                                  ulaXLog          *aXLog,
                                                  ulaErrorMgr      *aErrorMgr)
{
    acp_uint32_t  i;
    ulaTable     *sTable = NULL;
    ulaColumn    *sColumn = NULL;

    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);

    ACI_TEST(ulaMetaGetTableInfo(&aCollector->mMeta,
                                 aXLog->mHeader.mTableOID,
                                 &sTable,
                                 aErrorMgr)
             != ACI_SUCCESS);
    ACI_TEST_RAISE(sTable == NULL, ERR_TABLE_NOT_FOUND);

    for (i = 0; i < aXLog->mColumn.mColCnt; i++)
    {
        ACI_TEST(ulaMetaGetColumnInfo(sTable,
                                      aXLog->mColumn.mCIDArray[i],
                                      &sColumn,
                                      aErrorMgr)
                 != ACI_SUCCESS);
        ACI_TEST_RAISE(sColumn == NULL, ERR_COLUMN_NOT_FOUND);

        if (aXLog->mColumn.mAColArray[i].value != NULL)
        {
            ACI_TEST(ulaConvertEndianValue(sColumn->mDataType,
                                           sColumn->mEncrypt,
                                           aXLog->mColumn.mAColArray[i].value,
                                           aErrorMgr)
                     != ACI_SUCCESS);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Endian Convert (Insert)");
    }
    ACI_EXCEPTION(ERR_TABLE_NOT_FOUND)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_TABLE_NOT_FOUND,
                        "Endian Convert (Insert)",
                        aXLog->mHeader.mTableOID);
    }
    ACI_EXCEPTION(ERR_COLUMN_NOT_FOUND)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_COLUMN_NOT_FOUND,
                        "Endian Convert (Insert)",
                        aXLog->mColumn.mCIDArray[i]);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulaXLogCollectorConvertEndian(ulaXLogCollector *aCollector,
                                            ulaXLog          *aXLog,
                                            ulaErrorMgr      *aErrorMgr)
{
    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);

    switch (aXLog->mHeader.mType)
    {
        case ULA_X_INSERT:
            ACI_TEST(ulaXLogCollectorConvertEndianInsert(aCollector,
                                                         aXLog,
                                                         aErrorMgr)
                     != ACI_SUCCESS);
            break;

        case ULA_X_UPDATE:
            ACI_TEST(ulaXLogCollectorConvertEndianUpdate(aCollector,
                                                         aXLog,
                                                         aErrorMgr)
                     != ACI_SUCCESS);
            break;

        case ULA_X_DELETE:
        case ULA_X_LOB_CURSOR_OPEN:      // BUG-24526
            ACI_TEST(ulaXLogCollectorConvertEndianPK(aCollector,
                                                     aXLog,
                                                     aErrorMgr)
                     != ACI_SUCCESS);
            break;

        default:
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Endian Convert");
    }
    ACI_EXCEPTION_END;

    // 이미 ulaSetErrorCode() 수행

    return ACI_FAILURE;
}

/*
 * -----------------------------------------------------------------------------
 *  Decrypt Values
 * -----------------------------------------------------------------------------
 */
static ACI_RC ulaXLogCollectorDecryptValuesInsert(ulaXLogCollector *aCollector,
                                                  ulaXLog          *aXLog,
                                                  ulaErrorMgr      *aErrorMgr)
{
    acp_uint32_t  sIndex;
    ulaTable     *sTable = NULL;
    ulaColumn    *sColumn = NULL;

    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);

    ACI_TEST(ulaMetaGetTableInfo(&aCollector->mMeta,
                                 aXLog->mHeader.mTableOID,
                                 &sTable,
                                 aErrorMgr)
             != ACI_SUCCESS);
    ACI_TEST_RAISE(sTable == NULL, ERR_TABLE_NOT_FOUND);

    for (sIndex = 0; sIndex < aXLog->mColumn.mColCnt; sIndex++)
    {
        ACI_TEST(ulaMetaGetColumnInfo(sTable,
                                      aXLog->mColumn.mCIDArray[sIndex],
                                      &sColumn,
                                      aErrorMgr)
                 != ACI_SUCCESS);
        ACI_TEST_RAISE(sColumn == NULL, ERR_COLUMN_NOT_FOUND);

        if ((sColumn->mEncrypt == ACP_TRUE) &&
            (aXLog->mColumn.mAColArray[sIndex].value != NULL))
        {
            // PROJ-2002 Column Security
            // 암호화된 값을 복호화한다. 그러나 ALA가 보안 모듈 연동을
            // 지원하지 않아 복호화를 수행할 수 없다. 임시로
            // 사용자가 암호 데이터를 볼 수 없도록 null로 변경한다.
            aXLog->mColumn.mAColArray[sIndex].length = 0;
            (void)aclMemFree( aCollector->mMemAllocator,
                              aXLog->mColumn.mAColArray[sIndex].value );
            aXLog->mColumn.mAColArray[sIndex].value = NULL;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Remove Encrypt Values (Insert)");
    }
    ACI_EXCEPTION(ERR_TABLE_NOT_FOUND)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_TABLE_NOT_FOUND,
                        "Remove Encrypt Values (Insert)",
                        aXLog->mHeader.mTableOID);
    }
    ACI_EXCEPTION(ERR_COLUMN_NOT_FOUND)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_COLUMN_NOT_FOUND,
                        "Remove Encrypt Values (Insert)",
                        aXLog->mColumn.mCIDArray[sIndex]);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulaXLogCollectorDecryptValuesUpdate(ulaXLogCollector *aCollector,
                                                  ulaXLog     * aXLog,
                                                  ulaErrorMgr * aErrorMgr)
{
    acp_uint32_t   sIndex;
    ulaTable      *sTable = NULL;
    ulaColumn     *sColumn = NULL;

    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);

    ACI_TEST(ulaMetaGetTableInfo(&aCollector->mMeta,
                                 aXLog->mHeader.mTableOID,
                                 &sTable,
                                 aErrorMgr)
             != ACI_SUCCESS);
    ACI_TEST_RAISE(sTable == NULL, ERR_TABLE_NOT_FOUND);

    for (sIndex = 0; sIndex < aXLog->mPrimaryKey.mPKColCnt; sIndex++)
    {
        sColumn = sTable->mPKColumnArray[sIndex];
        ACI_TEST_RAISE(sColumn == NULL, ERR_PK_COLUMN_NOT_FOUND);

        if ((sColumn->mEncrypt == ACP_TRUE) &&
            (aXLog->mPrimaryKey.mPKColArray[sIndex].value != NULL))
        {
            // PROJ-2002 Column Security
            // 암호화된 값을 복호화한다. 그러나 ALA가 보안 모듈 연동을
            // 지원하지 않아 복호화를 수행할 수 없다. 임시로
            // 사용자가 암호 데이터를 볼 수 없도록 null로 변경한다.
            aXLog->mPrimaryKey.mPKColArray[sIndex].length = 0;
            (void)aclMemFree( aCollector->mMemAllocator,
                              aXLog->mPrimaryKey.mPKColArray[sIndex].value );
            aXLog->mPrimaryKey.mPKColArray[sIndex].value = NULL;
        }
    }

    for (sIndex = 0; sIndex < aXLog->mColumn.mColCnt; sIndex++)
    {
        ACI_TEST(ulaMetaGetColumnInfo(sTable,
                                      aXLog->mColumn.mCIDArray[sIndex],
                                      &sColumn,
                                      aErrorMgr)
                 != ACI_SUCCESS);
        ACI_TEST_RAISE(sColumn == NULL, ERR_COLUMN_NOT_FOUND);

        if (sColumn->mEncrypt == ACP_TRUE)
        {
            if (aXLog->mColumn.mBColArray[sIndex].value != NULL)
            {
                aXLog->mColumn.mBColArray[sIndex].length = 0;
                (void)aclMemFree( aCollector->mMemAllocator,
                                  aXLog->mColumn.mBColArray[sIndex].value );
                aXLog->mColumn.mBColArray[sIndex].value = NULL;
            }

            if (aXLog->mColumn.mAColArray[sIndex].value != NULL)
            {
                aXLog->mColumn.mAColArray[sIndex].length = 0;
                (void)aclMemFree( aCollector->mMemAllocator,
                                  aXLog->mColumn.mAColArray[sIndex].value );
                aXLog->mColumn.mAColArray[sIndex].value = NULL;
            }
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Remove Encrypt Values (Update)");
    }
    ACI_EXCEPTION(ERR_TABLE_NOT_FOUND)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_TABLE_NOT_FOUND,
                        "Remove Encrypt Values (Update)",
                        aXLog->mHeader.mTableOID);
    }
    ACI_EXCEPTION(ERR_PK_COLUMN_NOT_FOUND)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_COLUMN_NOT_FOUND,
                        "Remove Encrypt Values (Update)",
                        sTable->mPKColumnArray[sIndex]->mColumnID);
    }
    ACI_EXCEPTION(ERR_COLUMN_NOT_FOUND)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_COLUMN_NOT_FOUND,
                        "Remove Encrypt Values (Update)",
                        aXLog->mColumn.mCIDArray[sIndex]);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulaXLogCollectorDecryptValuesPK(ulaXLogCollector *aCollector,
                                              ulaXLog     * aXLog,
                                              ulaErrorMgr * aErrorMgr)
{
    acp_uint32_t  sIndex;
    ulaTable     *sTable = NULL;
    ulaColumn    *sColumn = NULL;

    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);

    ACI_TEST(ulaMetaGetTableInfo(&aCollector->mMeta,
                                 aXLog->mHeader.mTableOID,
                                 &sTable,
                                 aErrorMgr)
             != ACI_SUCCESS);
    ACI_TEST_RAISE(sTable == NULL, ERR_TABLE_NOT_FOUND);

    for (sIndex = 0; sIndex < aXLog->mPrimaryKey.mPKColCnt; sIndex++)
    {
        sColumn = sTable->mPKColumnArray[sIndex];
        ACI_TEST_RAISE(sColumn == NULL, ERR_PK_COLUMN_NOT_FOUND);

        if ((sColumn->mEncrypt == ACP_TRUE) &&
            (aXLog->mPrimaryKey.mPKColArray[sIndex].value != NULL))
        {
            // PROJ-2002 Column Security
            // 암호화된 값을 복호화한다. 그러나 ALA가 보안 모듈 연동을
            // 지원하지 않아 복호화를 수행할 수 없다. 임시로
            // 사용자가 암호 데이터를 볼 수 없도록 null로 변경한다.
            aXLog->mPrimaryKey.mPKColArray[sIndex].length = 0;
            (void)aclMemFree( aCollector->mMemAllocator,
                              aXLog->mPrimaryKey.mPKColArray[sIndex].value );
            aXLog->mPrimaryKey.mPKColArray[sIndex].value = NULL;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Remove Encrypt Values (Delete)");
    }
    ACI_EXCEPTION(ERR_TABLE_NOT_FOUND)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_TABLE_NOT_FOUND,
                        "Remove Encrypt Values (Delete)",
                        aXLog->mHeader.mTableOID);
    }
    ACI_EXCEPTION(ERR_PK_COLUMN_NOT_FOUND)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_COLUMN_NOT_FOUND,
                        "Remove Encrypt Values (Delete)",
                        sTable->mPKColumnArray[sIndex]->mColumnID);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulaXLogCollectorDecryptValues(ulaXLogCollector *aCollector,
                                            ulaXLog          *aXLog,
                                            ulaErrorMgr      *aErrorMgr)
{
    ACI_TEST_RAISE(aXLog == NULL, ERR_PARAMETER_NULL);

    switch (aXLog->mHeader.mType)
    {
        case ULA_X_INSERT:
            ACI_TEST(ulaXLogCollectorDecryptValuesInsert(aCollector,
                                                         aXLog,
                                                         aErrorMgr)
                     != ACI_SUCCESS);
            break;

        case ULA_X_UPDATE:
            ACI_TEST(ulaXLogCollectorDecryptValuesUpdate(aCollector,
                                                         aXLog,
                                                         aErrorMgr)
                     != ACI_SUCCESS);
            break;

        case ULA_X_DELETE:
        case ULA_X_LOB_CURSOR_OPEN:      // BUG-24526
            ACI_TEST(ulaXLogCollectorDecryptValuesPK(aCollector,
                                                     aXLog,
                                                     aErrorMgr)
                     != ACI_SUCCESS);
            break;

        default:
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Remove Encrypt Values");
    }
    ACI_EXCEPTION_END;

    // 이미 ulaSetErrorCode() 수행

    return ACI_FAILURE;
}

ACI_RC ulaXLogCollectorGetXLog(ulaXLogCollector  *aCollector,
                               ulaXLog          **aOutXLog,
                               ulaErrorMgr       *aErrorMgr)
{
    acp_rc_t     sRc;
    ulaXLog     *sXLog = NULL;
    acp_bool_t   sAckLock = ACP_FALSE;

    ACI_TEST_RAISE(aOutXLog == NULL, ERR_PARAMETER_NULL);

    /* XLog Queue에서 XLog를 꺼냅니다. */
    ACI_TEST(ulaXLogLinkedListRemoveFromHead(&aCollector->mXLogQueue,
                                             &sXLog,
                                             aErrorMgr)
             != ACI_SUCCESS);
    *aOutXLog = sXLog;

    if (sXLog != NULL)
    {
        /* Endian DIFF를 처리합니다. */
        if (ulaMetaIsEndianDiff(&aCollector->mMeta) != ACP_FALSE)
        {
            ACI_TEST(ulaXLogCollectorConvertEndian(aCollector, sXLog, aErrorMgr)
                     != ACI_SUCCESS);
        }

        // PROJ-2002 Column Security
        // 보안 컬럼에 대하여 암호화된 데이터를 복호화한다.
        ACI_TEST(ulaXLogCollectorDecryptValues(aCollector, sXLog, aErrorMgr)
                 != ACI_SUCCESS);

        /* Transaction Table에 XLog를 반영합니다. */
        switch (sXLog->mHeader.mType)
        {
            // PROJ-1663 : BEGIN 패킷을 암시적으로 처리
            case ULA_X_INSERT :
            case ULA_X_UPDATE :
            case ULA_X_DELETE :
            case ULA_X_SP_SET :
            case ULA_X_LOB_CURSOR_OPEN :
                if (ulaTransTblIsATrans(&aCollector->mTransTbl,
                                        sXLog->mHeader.mTID) != ACP_TRUE)
                {
                    ACI_TEST(ulaTransTblInsertTrans(&aCollector->mTransTbl,
                                                    sXLog->mHeader.mTID,
                                                    sXLog->mHeader.mSN,
                                                    aErrorMgr)
                             != ACI_SUCCESS);
                }
                break;

            case ULA_X_ABORT :
                ACI_TEST(ulaTransTblRemoveTrans(&aCollector->mTransTbl,
                                                sXLog->mHeader.mTID,
                                                aErrorMgr)
                         != ACI_SUCCESS);
                break;

            case ULA_X_COMMIT :
                // BUG-23339
                if ((aCollector->mUseCommittedTxBuffer == ACP_TRUE) &&
                    (ulaTransTblIsATrans(&aCollector->mTransTbl,
                                         sXLog->mHeader.mTID) != ACP_TRUE))
                {
                    break;
                }

                ACI_TEST(ulaTransTblRemoveTrans(&aCollector->mTransTbl,
                                                sXLog->mHeader.mTID,
                                                aErrorMgr)
                         != ACI_SUCCESS);
                break;

            default :
                break;
        }

        /* ACK 관련 유지 정보를 수집합니다. */
        sRc = acpThrMutexLock(&aCollector->mAckMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_ACK_LOCK);
        sAckLock = ACP_TRUE;

        aCollector->mProcessedXLogCount++;
        aCollector->mLastProcessedSN = sXLog->mHeader.mSN;

        if ((sXLog->mHeader.mType == ULA_X_COMMIT) ||
            (sXLog->mHeader.mType == ULA_X_ABORT) ||
            (sXLog->mHeader.mType == ULA_X_KEEP_ALIVE))
        {
            aCollector->mLastCommitSN = sXLog->mHeader.mSN;
        }

        // Keep Alive, Replication Stop을 처리합니다.
        if (sXLog->mHeader.mType == ULA_X_KEEP_ALIVE)
        {
            aCollector->mKeepAliveArrived = ACP_TRUE;
            aCollector->mRestartSN = sXLog->mHeader.mRestartSN; // BUG-17789
        }
        else if (sXLog->mHeader.mType == ULA_X_REPL_STOP)
        {
            aCollector->mStopACKArrived = ACP_TRUE;
            aCollector->mRestartSN = sXLog->mHeader.mRestartSN; // BUG-17789
        }

        sAckLock = ACP_FALSE;
        sRc = acpThrMutexUnlock(&aCollector->mAckMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_ACK_UNLOCK);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_MUTEX_ACK_LOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_ACK_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_ACK_UNLOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_ACK_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "XLog Get");
    }
    ACI_EXCEPTION_END;

    if (sAckLock == ACP_TRUE)
    {
        (void)acpThrMutexUnlock(&aCollector->mAckMutex);
    }

    return ACI_FAILURE;
}

ACI_RC ulaXLogCollectorSendACK(ulaXLogCollector *aCollector,
                               ulaErrorMgr      *aErrorMgr)
{
    acp_rc_t   sRc;
    ulaXLogAck sAck;
    acp_bool_t sSendACKFlag     = ACP_FALSE;
    acp_bool_t sNetworkExitFlag = ACP_FALSE;

    acp_bool_t sSendLock = ACP_FALSE;
    acp_bool_t sAckLock = ACP_FALSE;

    sRc = acpThrMutexLock(&aCollector->mSendMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_SEND_LOCK);
    sSendLock = ACP_TRUE;

    ACI_TEST_RAISE(aCollector->mNetworkExitFlag == ACP_TRUE, ERR_NET_INVALID);

    sRc = acpThrMutexLock(&aCollector->mAckMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_ACK_LOCK);
    sAckLock = ACP_TRUE;

    /* ACK, STOP ACK의 공통 부분을 구성합니다. */
    if ((aCollector->mStopACKArrived == ACP_TRUE) ||
        (aCollector->mKeepAliveArrived == ACP_TRUE) ||
        (aCollector->mProcessedXLogCount >= 
                            (acp_uint64_t)aCollector->mACKPerXLogCount))
    {
        acpMemSet(&sAck, 0x00, ACI_SIZEOF(ulaXLogAck));

        // ACK Type을 지정합니다.
        if (aCollector->mStopACKArrived == ACP_TRUE)
        {
            sAck.mAckType = ULA_X_STOP_ACK;

            // ACK 전송 이후에 네트워크를 사용할 수 없음을 표시합니다.
            sNetworkExitFlag = ACP_TRUE;
        }
        else
        {
            sAck.mAckType = ULA_X_ACK;
        }

        // LAZY MODE만 지원하므로, Abort/Clear Tx List는 없습니다.
        sAck.mAbortTxCount = 0;
        sAck.mClearTxCount = 0;
        sAck.mAbortTxList  = NULL;
        sAck.mClearTxList  = NULL;

        // ACK 관련 SN을 설정합니다.
        sAck.mRestartSN       = aCollector->mRestartSN;
        sAck.mLastCommitSN    = aCollector->mLastCommitSN;
        sAck.mLastArrivedSN   = aCollector->mLastArrivedSN;
        sAck.mLastProcessedSN = aCollector->mLastProcessedSN;

        // ACK 전송 가능함을 표시합니다.
        sSendACKFlag = ACP_TRUE;

        // ACK 전송 조건을 초기화합니다.
        aCollector->mKeepAliveArrived   = ACP_FALSE;
        aCollector->mStopACKArrived     = ACP_FALSE;
        aCollector->mProcessedXLogCount = 0;
    }

    sAckLock = ACP_FALSE;
    sRc = acpThrMutexUnlock(&aCollector->mAckMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_ACK_UNLOCK);

    /* ACK 전송 가능 여부를 검사하고 전송합니다. */
    if (sSendACKFlag == ACP_TRUE)
    {
        // ACK를 전송합니다.
        ACI_TEST_RAISE(ulaCommSendAck( &(aCollector->mProtocolContext), sAck, aErrorMgr )
                       != ACI_SUCCESS, ERR_ACK_SEND);

        if (sNetworkExitFlag == ACP_TRUE)
        {
            // 네트워크 연결이 끊어진 것으로 표시합니다.
            aCollector->mNetworkExitFlag = ACP_TRUE;
        }
    }

    sSendLock = ACP_FALSE;
    sRc = acpThrMutexUnlock(&aCollector->mSendMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_SEND_UNLOCK);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_MUTEX_SEND_LOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_SEND_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_SEND_UNLOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_SEND_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_ACK_LOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_ACK_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_ACK_UNLOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_ACK_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_NET_INVALID)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_ABORT_NET_EXIT,
                        "ACK Send Check");
    }
    ACI_EXCEPTION(ERR_ACK_SEND)
    {
        // 이미 ulaSetErrorCode() 수행

        aCollector->mNetworkExitFlag = ACP_TRUE;
    }
    ACI_EXCEPTION_END;

    if (sAckLock == ACP_TRUE)
    {
        (void)acpThrMutexUnlock(&aCollector->mAckMutex);
    }
    if (sSendLock == ACP_TRUE)
    {
        (void)acpThrMutexUnlock(&aCollector->mSendMutex);
    }

    return ACI_FAILURE;
}


/*
 * -----------------------------------------------------------------------------
 *  Monitoring API
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogCollectorGetXLogCollectorStatus
                        (ulaXLogCollector       *aCollector,
                         ulaXLogCollectorStatus *aOutXLogCollectorStatus,
                         ulaErrorMgr            *aErrorMgr)
{
    acp_rc_t   sRc;
    acp_char_t sPort[ULA_PORT_LEN];

    acp_bool_t sSendLock = ACP_FALSE;
    acp_bool_t sReceiveLock = ACP_FALSE;
    acp_bool_t sAckLock = ACP_FALSE;

    ACI_TEST_RAISE(aOutXLogCollectorStatus == NULL, ERR_PARAMETER_NULL);

    acpMemSet(aOutXLogCollectorStatus, 0x00, ACI_SIZEOF(ulaXLogCollectorStatus));

    /* 네트워크 정보 설정 */
    sRc = acpThrMutexLock(&aCollector->mSendMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_SEND_LOCK);
    sSendLock = ACP_TRUE;

    sRc = acpThrMutexLock(&aCollector->mReceiveMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_RECEIVE_LOCK);
    sReceiveLock = ACP_TRUE;

    if ((aCollector->mSessionValid == ACP_TRUE) && 
        (aCollector->mNetworkExitFlag != ACP_TRUE))
    {
        aOutXLogCollectorStatus->mNetworkValid = ACP_TRUE;

        if (aCollector->mSocketType == ULA_SOCKET_TYPE_TCP)
        {
            cmiGetLinkInfo(aCollector->mPeerLink,
                           aOutXLogCollectorStatus->mMyIP,
                           ULA_IP_LEN,
                           CMI_LINK_INFO_LOCAL_IP_ADDRESS);

            cmiGetLinkInfo(aCollector->mPeerLink,
                           aOutXLogCollectorStatus->mPeerIP,
                           ULA_IP_LEN,
                           CMI_LINK_INFO_REMOTE_IP_ADDRESS);

            acpMemSet(sPort, 0x00, ULA_PORT_LEN);
            cmiGetLinkInfo(aCollector->mPeerLink,
                           sPort,
                           ULA_PORT_LEN,
                           CMI_LINK_INFO_LOCAL_PORT);
            (void)ulaStrToSInt32(sPort,
                                 ULA_PORT_LEN,
                                 &aOutXLogCollectorStatus->mMyPort);

            acpMemSet(sPort, 0x00, ULA_PORT_LEN);
            cmiGetLinkInfo(aCollector->mPeerLink,
                           sPort,
                           ULA_PORT_LEN,
                           CMI_LINK_INFO_REMOTE_PORT);
            (void)ulaStrToSInt32(sPort,
                                 ULA_PORT_LEN,
                                 &aOutXLogCollectorStatus->mPeerPort);
        }
        else if (aCollector->mSocketType == ULA_SOCKET_TYPE_UNIX)
        {
            acpMemCpy(aOutXLogCollectorStatus->mSocketFile,
                      aCollector->mUNIXSocketFile,
                      ULA_SOCK_NAME_LEN);
        }
    }
    else
    {
        aOutXLogCollectorStatus->mNetworkValid = ACP_FALSE;
    }

    sReceiveLock = ACP_FALSE;
    sRc = acpThrMutexUnlock(&aCollector->mReceiveMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_RECEIVE_UNLOCK);

    sSendLock = ACP_FALSE;
    sRc = acpThrMutexUnlock(&aCollector->mSendMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_SEND_UNLOCK);

    /* XLog Pool 정보 설정 */
    aOutXLogCollectorStatus->mXLogCountInPool = aCollector->mXLogFreeCount;

    /* GAP 정보 설정 */
    sRc = acpThrMutexLock(&aCollector->mAckMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_ACK_LOCK);
    sAckLock = ACP_TRUE;

    aOutXLogCollectorStatus->mLastArrivedSN   = aCollector->mLastArrivedSN;
    aOutXLogCollectorStatus->mLastProcessedSN = aCollector->mLastProcessedSN;

    sAckLock = ACP_FALSE;
    sRc = acpThrMutexUnlock(&aCollector->mAckMutex);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_ACK_UNLOCK);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_MUTEX_SEND_LOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_SEND_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_SEND_UNLOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_SEND_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_RECEIVE_LOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_RECEIVE_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_RECEIVE_UNLOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_RECEIVE_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_ACK_LOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_ACK_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_ACK_UNLOCK)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_ACK_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "XLog Collector Status");
    }
    ACI_EXCEPTION_END;

    if (sAckLock == ACP_TRUE)
    {
        (void)acpThrMutexUnlock(&aCollector->mAckMutex);
    }

    if (sReceiveLock == ACP_TRUE)
    {
        (void)acpThrMutexUnlock(&aCollector->mReceiveMutex);
    }
    if (sSendLock == ACP_TRUE)
    {
        (void)acpThrMutexUnlock(&aCollector->mSendMutex);
    }

    return ACI_FAILURE;
}

ulaMeta *ulaXLogCollectorGetMeta(ulaXLogCollector *aCollector)
{
    return &aCollector->mMeta;
}

void ulaXLogCollectorGetVersionFromAck( acp_char_t     *aMsg,
                                        acp_uint32_t    aMsgLen,
                                        acp_uint64_t   *aVersion )
{
    if ( aMsgLen == ACI_SIZEOF( acp_uint64_t ) ) /* If version information exists in aMsg, get it.  */
    {
        *aVersion = ulaXLogCollectorNtoh ( *(acp_uint64_t*)aMsg );
    }
    else /* otherwise remote version is lower than 7_4_1 version */
    {
        *aVersion = RP_MAKE_VERSION( 7, 4, 1, 0 );
    }
}

acp_uint64_t ulaXLogCollectorNtoh( acp_uint64_t Value )
{
    return ( ( ( Value & ACP_UINT64_LITERAL( 0xFF00000000000000 ) ) >> 56 ) |
             ( ( Value & ACP_UINT64_LITERAL( 0x00FF000000000000 ) ) >> 40 ) |
             ( ( Value & ACP_UINT64_LITERAL( 0x0000FF0000000000 ) ) >> 24 ) |
             ( ( Value & ACP_UINT64_LITERAL( 0x000000FF00000000 ) ) >> 8  ) |
             ( ( Value & ACP_UINT64_LITERAL( 0x00000000FF000000 ) ) << 8  ) |
             ( ( Value & ACP_UINT64_LITERAL( 0x0000000000FF0000 ) ) << 24 ) |
             ( ( Value & ACP_UINT64_LITERAL( 0x000000000000FF00 ) ) << 40 ) |
             ( ( Value & ACP_UINT64_LITERAL( 0x00000000000000FF ) ) << 56 ) );
}

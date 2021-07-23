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
 * $Id: ulaMeta.c 91310 2021-07-22 05:10:54Z donghyun1 $
 **********************************************************************/

#include <acp.h>
#include <acl.h>
#include <ace.h>

#include <ula.h>
#include <ulaMeta.h>
#include <ulaComm.h>

typedef struct processProtocolOperationType
{
    ULA_PROTOCOL_OP_CODE        mOpCode;
    acp_uint32_t                mMajorVersion;
    acp_uint32_t                mMinorVersion;
    acp_uint32_t                mFixVersion;
    acp_uint32_t                mEdianBit;
} processProtocolOperationType;

processProtocolOperationType gProcessProtocolOperationType[ULA_META_MAX] =
{
    { ULA_META_DICTTABLECOUNT, 7, 4, 2, 0 },
    { ULA_META_PARTITIONCOUNT, 7, 4, 4, 0 },
    { ULA_META_XSN           , 7, 4, 5, 0 },
    { ULA_META_SRID          , 7, 4, 6, 0 },
    { ULA_META_COMPRESSTYPE  , 7, 4, 8, 0 }    
};

/*
 * -----------------------------------------------------------------------------
 *  Compare functions for qsort
 * -----------------------------------------------------------------------------
 */
static acp_sint32_t ulaMetaCompareTableOID(const void *aElem1,
                                           const void *aElem2)
{
    if ((*(ulaTable **)aElem1)->mTableOID > (*(ulaTable **)aElem2)->mTableOID)
    {
        return 1;
    }
    else if ((*(ulaTable **)aElem1)->mTableOID <
             (*(ulaTable **)aElem2)->mTableOID)
    {
        return -1;
    }
    return 0;
}

static acp_sint32_t ulaMetaCompareTableName(const void *aElem1,
                                            const void *aElem2)
{
    acp_sint32_t sComp;

    sComp = acpCStrCaseCmp((*(ulaTable **)aElem1)->mFromUserName,
                           (*(ulaTable **)aElem2)->mFromUserName,
                           ULA_NAME_LEN);
    if (sComp != 0)
    {
        return sComp;
    }

    return acpCStrCaseCmp((*(ulaTable **)aElem1)->mFromTableName,
                          (*(ulaTable **)aElem2)->mFromTableName,
                          ULA_NAME_LEN);
}

static acp_sint32_t ulaMetaCompareColumn(const void *aElem1,
                                         const void *aElem2)
{
    if (((ulaColumn *)aElem1)->mColumnID > ((ulaColumn *)aElem2)->mColumnID)
    {
        return 1;
    }
    else if (((ulaColumn *)aElem1)->mColumnID < 
                                ((ulaColumn *)aElem2)->mColumnID)
    {
        return -1;
    }
    return 0;
}

static acp_sint32_t ulaMetaCompareIndex(const void *aElem1,
                                        const void *aElem2)
{
    if (((ulaIndex *)aElem1)->mIndexID > ((ulaIndex *)aElem2)->mIndexID)
    {
        return 1;
    }
    else if (((ulaIndex *)aElem1)->mIndexID < ((ulaIndex *)aElem2)->mIndexID)
    {
        return -1;
    }
    return 0;
}

/*
 * -----------------------------------------------------------------------------
 *  Convenience function for freeing memory of a meta
 * -----------------------------------------------------------------------------
 */
static void ulaMetaFreeMetaMemory(ulaMeta *aMeta)
{
    acp_uint32_t       sTC;     // Table Count
    acp_uint32_t       sIC;     // Index Count
    ulaTable * sTable;

    /* Meta 정보 메모리 해제 */
    if (aMeta->mReplication.mTableArray != NULL)
    {
        for (sTC = 0; sTC < aMeta->mReplication.mTableCount; sTC++)
        {
            sTable = &(aMeta->mReplication.mTableArray[sTC]);

            // PK Column
            if (sTable->mPKColumnArray != NULL)
            {
                acpMemFree(sTable->mPKColumnArray);
            }

            // Column
            if (sTable->mColumnArray != NULL)
            {
                acpMemFree(sTable->mColumnArray);
            }

            // Index
            if (sTable->mIndexArray != NULL)
            {
                for (sIC = 0; sIC < sTable->mIndexCount; sIC++)
                {
                    if (sTable->mIndexArray[sIC].mColumnIDArray != NULL)
                    {
                        acpMemFree(sTable->mIndexArray[sIC].mColumnIDArray);
                    }
                }
                acpMemFree(sTable->mIndexArray);
            }

            /* check constraint */
            if ( sTable->mChecks != NULL )
            {
                for ( sIC = 0; sIC < sTable->mCheckCount; sIC++)
                {
                    if ( sTable->mChecks[sIC].mCheckCondition != NULL )
                    {
                        acpMemFree( sTable->mChecks[sIC].mCheckCondition );
                        sTable->mChecks[sIC].mCheckCondition = NULL;
                    }
                    else
                    {
                        /* do nothing */
                    }
                }

                acpMemFree( sTable->mChecks );
                sTable->mChecks = NULL;
                sTable->mCheckCount = 0;
            }
            else
            {
                /* do nothing */
            }
        }

        acpMemFree(aMeta->mReplication.mTableArray);
        aMeta->mReplication.mTableArray = NULL;
    }

    /* Table OID, Table Name 정렬 정보 메모리 해제 */
    if (aMeta->mItemOrderByTableOID != NULL)
    {
        acpMemFree(aMeta->mItemOrderByTableOID);
        aMeta->mItemOrderByTableOID = NULL;
    }

    if (aMeta->mItemOrderByTableName != NULL)
    {
        acpMemFree(aMeta->mItemOrderByTableName);
        aMeta->mItemOrderByTableName = NULL;
    }
}

/*
 * -----------------------------------------------------------------------------
 *  ulaMeta Interfaces
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaMetaSendMeta( cmiProtocolContext * aProtocolContext,
                        acp_char_t         * aRepName,
                        acp_uint64_t         aRemoteVersion,
                        acp_uint32_t         aFlag,
                        ulaErrorMgr        * aOutErrorMgr )
{
    ACI_TEST( ulaCommSendMetaRepl( aProtocolContext,
                                   aRepName,
                                   aFlag,
                                   aRemoteVersion,
                                   aOutErrorMgr )
              != ACI_SUCCESS );

    if ( needToProcessProtocolOperation( ULA_META_DICTTABLECOUNT,
                                         aRemoteVersion )
         == ACP_TRUE )
    {
        ACI_TEST( ulaCommSendMetaDictTableCount( aProtocolContext,
                                                 aOutErrorMgr )
                  != ACI_SUCCESS );
    }

    if ( needToProcessProtocolOperation( ULA_META_XSN,
                                         aRemoteVersion )
         == ACP_TRUE )
    {
        ACI_TEST( ulaCommSendMetaInitFlag( aProtocolContext,
                                           aOutErrorMgr )
                  != ACI_SUCCESS );
    }

     return ACI_SUCCESS;
     
     ACI_EXCEPTION_END;

     return ACI_FAILURE;
}

ACI_RC ulaMetaRecvMeta(ulaMeta            * aMeta,
                       cmiProtocolContext * aProtocolContext,
                       acp_uint32_t         aTimeoutSec,
                       ulaVersion           aOutReplVersion,
                       acp_char_t         * aXLogSenderName,
                       acp_uint32_t       * aOutTransTableSize,
                       ulaErrorMgr        * aOutErrorMgr)
{
    acp_rc_t         sRc;
    acp_uint32_t     sTC;               // Table Count
    acp_uint32_t     sCC;               // Column Count
    acp_uint32_t     sIC;               // Index Count
    acp_bool_t       sDummy = ACP_FALSE; // Exit Flag Dummy
    ulaReplTempInfo  sTempInfo;
    ulaTable        *sTable;

    ACI_TEST_RAISE(aOutTransTableSize == NULL, ERR_PARAMETER_NULL);

    aMeta->mReplication.mSenderVersion = aOutReplVersion.mVersion;
    
    /* Replication 정보를 받는다. */
    ACI_TEST_RAISE( ulaCommRecvMetaRepl( aProtocolContext,
                                         &sDummy,
                                         &aMeta->mReplication,
                                         &sTempInfo,
                                         aTimeoutSec,
                                         aOutErrorMgr )
                    != ACI_SUCCESS, ERR_NETWORK );

    aMeta->mEndianDiff = ACP_FALSE;
#if defined(ENDIAN_IS_BIG_ENDIAN)
    if ((sTempInfo.mFlags & ULA_ENDIAN_MASK) == ULA_LITTLE_ENDIAN)
    {
        aMeta->mEndianDiff = ACP_TRUE;
    }
#else
    if ((sTempInfo.mFlags & ULA_ENDIAN_MASK) == ULA_BIG_ENDIAN)
    {
        aMeta->mEndianDiff = ACP_TRUE;
    }
#endif

    /* Transaction Table Size를 얻는다. */
    *aOutTransTableSize = sTempInfo.mTransTableSize;

    /* Table 개수에 따라서, Table 메모리를 할당한다. */
    sRc = acpMemAlloc((void **)&aMeta->mReplication.mTableArray,
                      aMeta->mReplication.mTableCount * ACI_SIZEOF(ulaTable));
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ALA_ERR_MEM_ALLOC);
    acpMemSet(aMeta->mReplication.mTableArray,
              0x00,
              aMeta->mReplication.mTableCount * ACI_SIZEOF(ulaTable));

    /* Table 개수만큼 반복하며 Meta를 받는다. */
    for (sTC = 0; sTC < aMeta->mReplication.mTableCount; sTC++)
    {
        sTable = &(aMeta->mReplication.mTableArray[sTC]);

        ACI_TEST_RAISE( ulaCommRecvMetaReplTbl( aProtocolContext,
                                                &sDummy,
                                                sTable,
                                                aTimeoutSec,
                                                aOutErrorMgr )
                        != ACI_SUCCESS, ERR_NETWORK );

        if ( needToProcessProtocolOperation( ULA_META_PARTITIONCOUNT,
                                             aMeta->mReplication.mSenderVersion )
             == ACP_TRUE)
        {
            ACI_TEST_RAISE( ulaCommRecvMetaPartitionCount( aProtocolContext,
                                                           &sDummy,
                                                           aTimeoutSec,
                                                           aOutErrorMgr )
                            != ACI_SUCCESS, ERR_NETWORK );
        }

        sRc = acpMemAlloc((void **)&sTable->mColumnArray,
                          sTable->mColumnCount * ACI_SIZEOF(ulaColumn));
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ALA_ERR_MEM_ALLOC);
        acpMemSet(sTable->mColumnArray,
                  0x00,
                  sTable->mColumnCount * ACI_SIZEOF(ulaColumn));

        /* Column 개수만큼 반복하며, 각 Column의 정보를 받는다. */
        for (sCC = 0; sCC < sTable->mColumnCount; sCC++)
        {
            ACI_TEST_RAISE( ulaCommRecvMetaReplCol( aProtocolContext,
                                                    &sDummy,
                                                    &(sTable->mColumnArray[sCC]),
                                                    aTimeoutSec,
                                                    aOutErrorMgr )
                            != ACI_SUCCESS, ERR_NETWORK );
        }

        sRc = acpMemAlloc((void **)&sTable->mIndexArray,
                          sTable->mIndexCount * ACI_SIZEOF(ulaIndex));
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ALA_ERR_MEM_ALLOC);
        acpMemSet(sTable->mIndexArray,
                  0x00,
                  sTable->mIndexCount * ACI_SIZEOF(ulaIndex));

        /* Index 개수만큼 반복하며, 각 Index의 정보를 받는다. */
        for (sIC = 0; sIC < sTable->mIndexCount; sIC++)
        {
            ACI_TEST_RAISE( ulaCommRecvMetaReplIdx( aProtocolContext,
                                                    &sDummy,
                                                    &(sTable->mIndexArray[sIC]),
                                                    aTimeoutSec,
                                                    aOutErrorMgr )
                            != ACI_SUCCESS, ERR_NETWORK );

            sRc = acpMemAlloc
                    ((void **)&sTable->mIndexArray[sIC].mColumnIDArray,
                     sTable->mIndexArray[sIC].mColumnCount *
                                                    ACI_SIZEOF(acp_uint32_t));
            ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ALA_ERR_MEM_ALLOC);
            acpMemSet(sTable->mIndexArray[sIC].mColumnIDArray,
                      0x00,
                      sTable->mIndexArray[sIC].mColumnCount *
                                                    ACI_SIZEOF(acp_uint32_t));

            for (sCC = 0; sCC < sTable->mIndexArray[sIC].mColumnCount; sCC++)
            {
                ACI_TEST_RAISE( ulaCommRecvMetaReplIdxCol
                                ( aProtocolContext,
                                  &sDummy,
                                  &sTable->mIndexArray[sIC].mColumnIDArray[sCC],
                                  aTimeoutSec,
                                  aOutErrorMgr ) != ACI_SUCCESS, ERR_NETWORK );
            }
        }

        if ( sTable->mCheckCount != 0 )
        {
            sRc = acpMemAlloc( (void **)&(sTable->mChecks), 
                               (sTable->mCheckCount) * ACI_SIZEOF(ulaCheck) );
            ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);

            acpMemSet( sTable->mChecks,
                       0x00,
                       (sTable->mCheckCount) * ACI_SIZEOF(ulaCheck) );

            for ( sIC = 0; sIC < sTable->mCheckCount; sIC++ )
            {
                ACI_TEST( ulaCommRecvMetaReplCheck( aProtocolContext,
                                                    &sDummy,
                                                    &(sTable->mChecks[sIC]),
                                                    aTimeoutSec,
                                                    aOutErrorMgr ) != ACI_SUCCESS );
            }
        }
        else
        {
            /* do nothing */
        }
    }

    if ( needToProcessProtocolOperation( ULA_META_DICTTABLECOUNT,
                                         aMeta->mReplication.mSenderVersion )
         == ACP_TRUE )
    {
        ACI_TEST( ulaCommRecvMetaDictTableCount( aProtocolContext,
                                                 &sDummy,
                                                 aTimeoutSec,
                                                 aOutErrorMgr )
                  != ACI_SUCCESS );
    }

    if ( needToProcessProtocolOperation( ULA_META_XSN,
                                         aMeta->mReplication.mSenderVersion )
         == ACP_TRUE )
    {
        ACI_TEST( ulaCommRecvMetaInitFlag( aProtocolContext,
                                           &sDummy,
                                           aTimeoutSec,
                                           aOutErrorMgr )
                  != ACI_SUCCESS );
    }
    
    /* XLog Sender Name을 확인한다. */
    ACI_TEST_RAISE(acpCStrCaseCmp(aMeta->mReplication.mXLogSenderName,
                                  aXLogSenderName,
                                  ULA_NAME_LEN) != 0, ERR_XLOG_SENDER_NAME);
    /* ROLE을 확인한다. */
    ACI_TEST_RAISE( ( sTempInfo.mRole != ULA_ROLE_ANALYSIS ) && 
                    ( sTempInfo.mRole != ULA_ROLE_ANALYSIS_PROPAGATION ), ERR_ROLE);

    /* Replication Flags를 확인한다. */
    ACI_TEST_RAISE((sTempInfo.mFlags & ULA_WAKEUP_PEER_SENDER_MASK)
                   == ULA_WAKEUP_PEER_SENDER_FLAG_SET, ERR_WAKEUP_PEER_SENDER);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Meta Receive");
    }
    ACI_EXCEPTION(ALA_ERR_MEM_ALLOC)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_MEMORY_ALLOC);
    }
    ACI_EXCEPTION(ERR_NETWORK)
    {
        // 이미 ulaSetErrorCode() 수행
    }
    ACI_EXCEPTION(ERR_XLOG_SENDER_NAME)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_XLOG_SENDER_NAME_DIFF,
                        aXLogSenderName,
                        aMeta->mReplication.mXLogSenderName);
    }
    ACI_EXCEPTION(ERR_ROLE)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_INVALID_ROLE,
                        sTempInfo.mRole);
    }
    ACI_EXCEPTION(ERR_WAKEUP_PEER_SENDER)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_INVALID_REPLICATION_FLAGS,
                        sTempInfo.mFlags);
    }

    ACI_EXCEPTION(ULA_ERR_MEM_ALLOC)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_MEMORY_ALLOC);
    }

    ACI_EXCEPTION_END;

    ulaMetaFreeMetaMemory(aMeta);

    return ACI_FAILURE;
}

ACI_RC ulaMetaSortMeta(ulaMeta *aMeta, ulaErrorMgr *aOutErrorMgr)
{
    acp_rc_t         sRc;
    acp_uint32_t     sTC;               // Table Count
    acp_uint32_t     sCC;               // Column Count
    ulaTable        *sTable;
    ulaColumn       *sColumn;
    ulaIndex        *sIndex;

    ACI_TEST_RAISE((aMeta->mReplication.mTableArray == NULL) ||
                   (aMeta->mReplication.mTableCount == 0), ERR_META_NOT_EXIST);

    /* Table OID, Table Name 정렬 메모리 할당 및 초기화 */
    sRc = acpMemAlloc((void **)&aMeta->mItemOrderByTableOID,
                      aMeta->mReplication.mTableCount * ACI_SIZEOF(ulaTable *));
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ALA_ERR_MEM_ALLOC);

    sRc = acpMemAlloc((void **)&aMeta->mItemOrderByTableName,
                      aMeta->mReplication.mTableCount * ACI_SIZEOF(ulaTable *));
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ALA_ERR_MEM_ALLOC);

    for (sTC = 0; sTC < aMeta->mReplication.mTableCount; sTC++)
    {
        aMeta->mItemOrderByTableOID[sTC]  =
                        &(aMeta->mReplication.mTableArray[sTC]);
        aMeta->mItemOrderByTableName[sTC] =
                        &(aMeta->mReplication.mTableArray[sTC]);
    }

    /* Table OID, Table Name 정렬 */
    acpSortQuickSort(aMeta->mItemOrderByTableOID,
                     aMeta->mReplication.mTableCount,
                     ACI_SIZEOF(ulaTable *),
                     ulaMetaCompareTableOID);

    acpSortQuickSort(aMeta->mItemOrderByTableName,
                     aMeta->mReplication.mTableCount,
                     ACI_SIZEOF(ulaTable *),
                     ulaMetaCompareTableName);

    /* Column, Index 정렬 */
    for (sTC = 0; sTC < aMeta->mReplication.mTableCount; sTC++)
    {
        sTable = &(aMeta->mReplication.mTableArray[sTC]);

        acpSortQuickSort(sTable->mColumnArray,
                         sTable->mColumnCount,
                         ACI_SIZEOF(ulaColumn),
                         ulaMetaCompareColumn);

        acpSortQuickSort(sTable->mIndexArray,
                         sTable->mIndexCount,
                         ACI_SIZEOF(ulaIndex),
                         ulaMetaCompareIndex);
    }

    /* PK Column 작성 */
    for (sTC = 0; sTC < aMeta->mReplication.mTableCount; sTC++)
    {
        sTable = &(aMeta->mReplication.mTableArray[sTC]);

        ACI_TEST_RAISE(ulaMetaGetIndexInfo(sTable,
                                           sTable->mPKIndexID,
                                           &sIndex,
                                           aOutErrorMgr)
                       != ACI_SUCCESS, ERR_GET_INDEX_INFO);
        ACI_TEST_RAISE(sIndex == NULL, ERR_META_NOT_EXIST);

        sTable->mPKColumnCount = sIndex->mColumnCount;

        sRc = acpMemAlloc((void **)&sTable->mPKColumnArray,
                          sTable->mPKColumnCount * ACI_SIZEOF(ulaColumn *));
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ALA_ERR_MEM_ALLOC);

        for (sCC = 0; sCC < sIndex->mColumnCount; sCC++)
        {
            ACI_TEST_RAISE(ulaMetaGetColumnInfo(sTable,
                                                sIndex->mColumnIDArray[sCC],
                                                &sColumn,
                                                aOutErrorMgr)
                           != ACI_SUCCESS, ERR_GET_COLUMN_INFO);

            sTable->mPKColumnArray[sCC] = sColumn;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_META_NOT_EXIST)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_META_NOT_EXIST);
    }
    ACI_EXCEPTION(ALA_ERR_MEM_ALLOC)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_MEMORY_ALLOC);
    }
    ACI_EXCEPTION(ERR_GET_INDEX_INFO)
    {
        // 이미 ulaSetErrorCode() 수행
    }
    ACI_EXCEPTION(ERR_GET_COLUMN_INFO)
    {
        // 이미 ulaSetErrorCode() 수행
    }
    ACI_EXCEPTION_END;

    /* Table OID, Table Name 정렬 정보 메모리 해제 */
    if (aMeta->mItemOrderByTableOID != NULL)
    {
        acpMemFree(aMeta->mItemOrderByTableOID);
        aMeta->mItemOrderByTableOID = NULL;
    }

    if (aMeta->mItemOrderByTableName != NULL)
    {
        acpMemFree(aMeta->mItemOrderByTableName);
        aMeta->mItemOrderByTableName = NULL;
    }

    /* PK Column 메모리 해제 */
    if (aMeta->mReplication.mTableArray != NULL)
    {
        for (sTC = 0; sTC < aMeta->mReplication.mTableCount; sTC++)
        {
            sTable = &(aMeta->mReplication.mTableArray[sTC]);

            if (sTable->mPKColumnArray != NULL)
            {
                acpMemFree(sTable->mPKColumnArray);
                sTable->mPKColumnArray = NULL;
            }
        }
    }

    return ACI_FAILURE;
}

ACI_RC ulaMetaGetReplicationInfo(ulaMeta         *aMeta,
                                 ulaReplication **aOutReplication,
                                 ulaErrorMgr     *aOutErrorMgr)
{
    ACI_TEST_RAISE(aOutReplication == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE((aMeta->mReplication.mTableArray == NULL) ||
                   (aMeta->mReplication.mTableCount == 0), ERR_META_NOT_EXIST);

    *aOutReplication = &aMeta->mReplication;

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Replication");
    }
    ACI_EXCEPTION(ERR_META_NOT_EXIST)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_META_NOT_EXIST);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaMetaGetTableInfo(ulaMeta       *aMeta,
                           acp_uint64_t   aTableOID,
                           ulaTable     **aOutTable,
                           ulaErrorMgr   *aOutErrorMgr)
{
    acp_sint32_t sLow;
    acp_sint32_t sHigh;
    acp_sint32_t sMid;
    acp_sint32_t sComp;

    ACI_TEST_RAISE(aOutTable == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE((aMeta->mReplication.mTableArray == NULL) ||
                   (aMeta->mReplication.mTableCount == 0), ERR_META_NOT_EXIST);

    sLow  = 0;
    sHigh = aMeta->mReplication.mTableCount - 1;
    *aOutTable = NULL;
    while (sLow <= sHigh)
    {
        sMid = (sHigh + sLow) >> 1;
        sComp = aMeta->mItemOrderByTableOID[sMid]->mTableOID - aTableOID;

        if (sComp > 0)
        {
            sHigh = sMid - 1;
        }
        else if (sComp < 0)
        {
            sLow = sMid + 1;
        }
        else
        {
            *aOutTable = aMeta->mItemOrderByTableOID[sMid];
            break;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Table");
    }
    ACI_EXCEPTION(ERR_META_NOT_EXIST)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_META_NOT_EXIST);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaMetaGetTableInfoByName(ulaMeta           *aMeta,
                                 const acp_char_t  *aFromUserName,
                                 const acp_char_t  *aFromTableName,
                                 ulaTable         **aOutTable,
                                 ulaErrorMgr       *aOutErrorMgr)
{
    acp_sint32_t sLow;
    acp_sint32_t sHigh;
    acp_sint32_t sMid;
    acp_sint32_t sComp;

    ACI_TEST_RAISE(aOutTable == NULL, ERR_PARAMETER_NULL);

    ACI_TEST_RAISE((aMeta->mReplication.mTableArray == NULL) ||
                   (aMeta->mReplication.mTableCount == 0), ERR_META_NOT_EXIST);

    sLow  = 0;
    sHigh = aMeta->mReplication.mTableCount - 1;
    *aOutTable = NULL;
    while (sLow <= sHigh)
    {
        sMid = (sHigh + sLow) >> 1;
        sComp = acpCStrCaseCmp
                        (aMeta->mItemOrderByTableName[sMid]->mFromUserName,
                         aFromUserName,
                         ULA_NAME_LEN);
        if (sComp == 0)
        {
            sComp = acpCStrCaseCmp
                        (aMeta->mItemOrderByTableName[sMid]->mFromTableName,
                         aFromTableName,
                         ULA_NAME_LEN);
        }

        if (sComp > 0)
        {
            sHigh = sMid - 1;
        }
        else if (sComp < 0)
        {
            sLow = sMid + 1;
        }
        else
        {
            *aOutTable = aMeta->mItemOrderByTableName[sMid];
            break;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Table");
    }
    ACI_EXCEPTION(ERR_META_NOT_EXIST)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_META_NOT_EXIST);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaMetaGetColumnInfo(ulaTable      *aTable,
                            acp_uint32_t   aColumnID,
                            ulaColumn    **aOutColumn,
                            ulaErrorMgr   *aOutErrorMgr)
{
    acp_sint32_t sLow;
    acp_sint32_t sHigh;
    acp_sint32_t sMid;
    acp_sint32_t sComp;

    ACI_TEST_RAISE((aTable == NULL) || (aOutColumn == NULL),
                   ERR_PARAMETER_NULL);

    sLow  = 0;
    sHigh = aTable->mColumnCount - 1;
    *aOutColumn = NULL;
    while (sLow <= sHigh)
    {
        sMid = (sHigh + sLow) >> 1;
        sComp = aTable->mColumnArray[sMid].mColumnID - aColumnID;

        if (sComp > 0)
        {
            sHigh = sMid - 1;
        }
        else if (sComp < 0)
        {
            sLow = sMid + 1;
        }
        else
        {
            *aOutColumn = &(aTable->mColumnArray[sMid]);
            break;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Table or Column");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaMetaGetIndexInfo(ulaTable      *aTable,
                           acp_uint32_t   aIndexID,
                           ulaIndex     **aOutIndex,
                           ulaErrorMgr   *aOutErrorMgr)
{
    acp_sint32_t sLow;
    acp_sint32_t sHigh;
    acp_sint32_t sMid;
    acp_sint32_t sComp;

    ACI_TEST_RAISE((aTable == NULL) || (aOutIndex == NULL),
                   ERR_PARAMETER_NULL);

    sLow  = 0;
    sHigh = aTable->mIndexCount - 1;
    *aOutIndex = NULL;
    while (sLow <= sHigh)
    {
        sMid = (sHigh + sLow) >> 1;
        sComp = aTable->mIndexArray[sMid].mIndexID - aIndexID;

        if (sComp > 0)
        {
            sHigh = sMid - 1;
        }
        else if (sComp < 0)
        {
            sLow = sMid + 1;
        }
        else
        {
            *aOutIndex = &(aTable->mIndexArray[sMid]);
            break;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Table or Index");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_bool_t ulaMetaIsEndianDiff(ulaMeta *aMeta)
{
    return aMeta->mEndianDiff;
}

ACI_RC ulaMetaGetProtocolVersion(ulaProtocolVersion *aOutProtocolVersion,
                                 ulaErrorMgr        *aOutErrorMgr)
{
    ACI_TEST_RAISE(aOutProtocolVersion == NULL, ERR_PARAMETER_NULL);

    aOutProtocolVersion->mMajor = REPLICATION_MAJOR_VERSION;
    aOutProtocolVersion->mMinor = REPLICATION_MINOR_VERSION;
    aOutProtocolVersion->mFix   = REPLICATION_FIX_VERSION;

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Protocol Version");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

void ulaMetaInitialize(ulaMeta *aMeta)
{
    acpMemSet(&aMeta->mReplication, 0x00, ACI_SIZEOF(ulaReplication));

    aMeta->mItemOrderByTableOID = NULL;
    aMeta->mItemOrderByTableName = NULL;

    aMeta->mEndianDiff = ACP_FALSE;
}

void ulaMetaDestroy(ulaMeta *aMeta)
{
    ulaMetaFreeMetaMemory( aMeta );
}

acp_bool_t ulaMetaIsHiddenColumn( ulaColumn   * aColumn )
{
    acp_bool_t sIsHiddenColumn = ACP_FALSE;

    ACE_DASSERT( aColumn != NULL );

    if( ( aColumn->mQPFlag & ULN_QPFLAG_HIDDEN_COLUMN_MASK )
        == ULN_QPFLAG_HIDDEN_COLUMN_TRUE )
    {
        sIsHiddenColumn = ACP_TRUE;
    }
    else
    {
        sIsHiddenColumn = ACP_FALSE;
    }

    return sIsHiddenColumn;
}

acp_bool_t needToProcessProtocolOperation( ULA_PROTOCOL_OP_CODE aOpCode,
                                           acp_uint64_t         aVersion )
{
    acp_bool_t sReturnValue = ACP_FALSE;

    ACE_DASSERT( aOpCode < ULA_META_MAX );
    ACE_DASSERT( aOpCode == gProcessProtocolOperationType[aOpCode].mOpCode );

    if ( aVersion >= RP_MAKE_VERSION( gProcessProtocolOperationType[aOpCode].mMajorVersion,
                                      gProcessProtocolOperationType[aOpCode].mMinorVersion,
                                      gProcessProtocolOperationType[aOpCode].mFixVersion,
                                      gProcessProtocolOperationType[aOpCode].mEdianBit ) )
    {
        sReturnValue = ACP_TRUE;
    }
    else
    {
        sReturnValue = ACP_FALSE;
    }

    return sReturnValue;
}

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
 * $Id: ulsdDistTxInfo.c 88046 2020-07-14 07:12:07Z donlet $
 **********************************************************************/

#include <ulnPrivate.h>
#include <uln.h>
#include <ulsdDistTxInfo.h>

#define ULSD_DIST_TX_INFO_SYNC_AS 1  /* 1 : Atomic Opearation, 2 : Mutex */
#define ULSD_DIST_TX_INFO_SYNC_ATOMIC_OP_SPIN_CNT 100

/**
 *  Update and Calculate DistTxInfo
 */

void ulsdInitDistTxInfo(ulnDbc *aDbc)
{
    aDbc->mSCN             = 0;
    aDbc->mTxFirstStmtSCN  = 0;
    aDbc->mTxFirstStmtTime = 0;
    aDbc->mDistLevel       = ULSD_DIST_LEVEL_INIT;
}

/* Env의 SCN은 멀티쓰레드 환경에서 동기화가 필요하다. */
void ulsdUpdateSCNToEnv(ulnEnv *aEnv, acp_uint64_t *aSCN)
{
#if (ULSD_DIST_TX_INFO_SYNC_AS == 1)
    acp_uint64_t sEnvSCN  = acpAtomicGet64(&aEnv->mSCN);
    acp_sint32_t sSpinCnt = ULSD_DIST_TX_INFO_SYNC_ATOMIC_OP_SPIN_CNT;

    if (*aSCN > sEnvSCN)
    {
        while ((acp_uint64_t)acpAtomicCas64(&aEnv->mSCN, *aSCN, sEnvSCN) != sEnvSCN)
        {
            /* 실패한 경우 조건이 만족할때까지 재시도한다. */
            sEnvSCN = acpAtomicGet64(&aEnv->mSCN);

            if (sEnvSCN >= *aSCN)
            {
                break;
            }

            if (sSpinCnt > 0)
            {
                sSpinCnt--;
            }
            else
            {
                acpSleepUsec(1);
            }
        }
    }

    /* Double check */
    ACE_DASSERT((acp_uint64_t)acpAtomicGet64(&aEnv->mSCN) >= *aSCN);
#elif (ULSD_DIST_TX_INFO_SYNC_AS == 2)
    if (*aSCN > aEnv->mSCN)
    {
        ULN_OBJECT_LOCK(&aEnv->mObj, ULN_FID_NONE);

        if (*aSCN > aEnv->mSCN)
        {
            aEnv->mSCN = *aSCN;
        }

        ULN_OBJECT_UNLOCK(&aEnv->mObj, ULN_FID_NONE);
    }

    /* Double check */
    ACE_DASSERT(aEnv->mSCN >= *aSCN);
#else
#error Please set AtomicOpeartion or Mutex
#endif
}

void ulsdUpdateSCNToDbc(ulnDbc *aDbc, acp_uint64_t *aSCN)
{
    aDbc->mSCN = *aSCN;
}

/* Session 타입에 따라 업데이트 되는 대상 SCN이 다르다. */
void ulsdUpdateSCN(ulnDbc *aDbc, acp_uint64_t *aSCN)
{
    switch (aDbc->mShardDbcCxt.mShardSessionType)
    {
        case ULSD_SESSION_TYPE_COORD:
            ulsdUpdateSCNToDbc(aDbc, aSCN);
            break;

        case ULSD_SESSION_TYPE_LIB:
            ulsdUpdateSCNToEnv(aDbc->mParentEnv, aSCN);
            break;

        case ULSD_SESSION_TYPE_USER:
            if (aDbc->mShardDbcCxt.mShardClient == ULSD_SHARD_CLIENT_TRUE)
            {
                ulsdUpdateSCNToEnv(aDbc->mParentEnv, aSCN);
            }
            else
            {
                /* USER with CLI는 SD에서 분산정보를 관리하므로 업데이트 할 필요가 없다. */
            }
            break;

        default:  /* Non-reachable */
            ACE_DASSERT(1);
            break;
    }
}

/**
 *  ulsdCalcDistTxInfoForMeta
 *  @aMetaDbc
 *  @aExecuteNodeCnt  : Exeucte를 수행할 Node 수
 *  @aNodeDbcIndexArr : Execute를 수행할 Node의 Index가 저장된 배열
 *
 *  Client-side로 수행되는 경우 분산정보를 계산한다.
 */
void ulsdCalcDistTxInfoForMeta(ulnDbc       *aMetaDbc,
                               acp_uint16_t  aExecuteNodeCnt,
                               acp_uint16_t *aNodeDbcIndexArr)
{
    acp_bool_t   sIsGCTx     = ulsdIsGCTx(ulnDbcGetGlobalTransactionLevel(aMetaDbc));
    acp_uint64_t sMetaEnvSCN = ULSD_NON_GCTX_TX_FIRST_STMT_SCN;

    /* BUG-48109 Non-GCTx */
    if (sIsGCTx == ACP_TRUE)
    {
        sMetaEnvSCN = acpAtomicGet64(&aMetaDbc->mParentEnv->mSCN);
    }

    /* PROJ-2733-DistTxInfo DistTxInfo를 설정한다. */
    if ( aMetaDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF )
    {
        ACE_DASSERT( ( (aMetaDbc->mTxFirstStmtSCN == 0) && (aMetaDbc->mTxFirstStmtTime == 0) ) ||
                     ( (aMetaDbc->mTxFirstStmtSCN != 0) && (aMetaDbc->mTxFirstStmtTime != 0) ) );

        if (aMetaDbc->mTxFirstStmtSCN == 0)
        {
            aMetaDbc->mTxFirstStmtSCN = sMetaEnvSCN;
        }
        if (aMetaDbc->mTxFirstStmtTime == 0)
        {
            aMetaDbc->mTxFirstStmtTime = acpTimeNow() / 1000;
        }
    }
    else  /* SQL_AUTOCOMMIT_ON */
    {
        aMetaDbc->mDistLevel       = ULSD_DIST_LEVEL_INIT;
        aMetaDbc->mTxFirstStmtSCN  = sMetaEnvSCN;
        aMetaDbc->mTxFirstStmtTime = acpTimeNow() / 1000;
    }

    if (aExecuteNodeCnt >= 2)
    {
        aMetaDbc->mDistLevel = ULSD_DIST_LEVEL_PARALLEL;
    }
    else
    {
        /* 노드가 1개인 경우 */
        switch (aMetaDbc->mDistLevel)
        {
            case ULSD_DIST_LEVEL_INIT:
                aMetaDbc->mShardDbcCxt.mBeforeExecutedNodeDbcIndex = aNodeDbcIndexArr[0];
                aMetaDbc->mDistLevel = ULSD_DIST_LEVEL_SINGLE;
                break;

            case ULSD_DIST_LEVEL_SINGLE:
                if (aMetaDbc->mShardDbcCxt.mBeforeExecutedNodeDbcIndex != aNodeDbcIndexArr[0]) 
                {
                    aMetaDbc->mDistLevel = ULSD_DIST_LEVEL_MULTI;
                }
                break;

            case ULSD_DIST_LEVEL_MULTI:
                break;

            case ULSD_DIST_LEVEL_PARALLEL:
                aMetaDbc->mDistLevel = ULSD_DIST_LEVEL_MULTI;
                break;

            default:
                ACE_DASSERT(0);
                aMetaDbc->mDistLevel = ULSD_DIST_LEVEL_MULTI;  /* Release */
                break;
        }
    }

    /* PROJ-2733-DistTxInfo 분산레벨이 PARALLEL일때만 SCN 값을 보낸다. */
    if (sIsGCTx == ACP_TRUE)
    {
        if (aMetaDbc->mDistLevel == ULSD_DIST_LEVEL_PARALLEL)
        {
            aMetaDbc->mSCN = sMetaEnvSCN;
        }
        else
        {
            aMetaDbc->mSCN = 0;
        }
    }
    else
    {
        /* BUG-48109 Non-GCTx */
        aMetaDbc->mSCN = 0;

        ACE_DASSERT((aMetaDbc->mSCN == 0) &&
                    (aMetaDbc->mTxFirstStmtSCN == ULSD_NON_GCTX_TX_FIRST_STMT_SCN));
    }
}

/**
 *  ulsdCalcDistTxInfoForCoord
 *  @aMetaDbc
 *
 *  Server-side로 수행되는 경우 분산정보를 계산한다.
 */
void ulsdCalcDistTxInfoForCoord(ulnDbc *aMetaDbc)
{
    acp_bool_t   sIsGCTx     = ulsdIsGCTx(ulnDbcGetGlobalTransactionLevel(aMetaDbc));
    acp_uint64_t sMetaEnvSCN = ULSD_NON_GCTX_TX_FIRST_STMT_SCN;

    /* BUG-48109 Non-GCTx */
    if (sIsGCTx == ACP_TRUE)
    {
        sMetaEnvSCN = acpAtomicGet64(&aMetaDbc->mParentEnv->mSCN);
    }

    /* PROJ-2733-DistTxInfo DistTxInfo를 설정한다. */
    if ( aMetaDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF )
    {
        ACE_DASSERT( ( (aMetaDbc->mTxFirstStmtSCN == 0) && (aMetaDbc->mTxFirstStmtTime == 0) ) ||
                     ( (aMetaDbc->mTxFirstStmtSCN != 0) && (aMetaDbc->mTxFirstStmtTime != 0) ) );

        if (aMetaDbc->mTxFirstStmtSCN == 0)
        {
            aMetaDbc->mTxFirstStmtSCN = sMetaEnvSCN;
        }
        if (aMetaDbc->mTxFirstStmtTime == 0)
        {
            aMetaDbc->mTxFirstStmtTime = acpTimeNow() / 1000;
        }
    }
    else  /* SQL_AUTOCOMMIT_ON */
    {
        aMetaDbc->mTxFirstStmtSCN  = sMetaEnvSCN;
        aMetaDbc->mTxFirstStmtTime = acpTimeNow() / 1000;
    }

    /* PROJ-2733-DistTxInfo COORD는 항상 DistLevel가 MULTI이다. */
    aMetaDbc->mDistLevel = ULSD_DIST_LEVEL_MULTI;

    /* BUG-48109 Non-GCTx */
    if (sIsGCTx == ACP_TRUE)
    {
        aMetaDbc->mSCN = sMetaEnvSCN;
    }
    else
    {
        aMetaDbc->mSCN = 0;

        ACE_DASSERT( (aMetaDbc->mSCN == 0) &&
                     (aMetaDbc->mTxFirstStmtSCN == ULSD_NON_GCTX_TX_FIRST_STMT_SCN) );
    }
}

/**
 *  ulsdPropagateDistTxInfoToNode
 *  @aNodeStmt
 *  @aMetaDbc
 *
 *  Client-side로 수행되는 경우 계산된 분산정보를 각 노드에 전파한다.
 */
void ulsdPropagateDistTxInfoToNode(ulnDbc *aNodeDbc, ulnDbc *aMetaDbc)
{
    aNodeDbc->mSCN             = aMetaDbc->mSCN;
    aNodeDbc->mTxFirstStmtSCN  = aMetaDbc->mTxFirstStmtSCN;
    aNodeDbc->mTxFirstStmtTime = aMetaDbc->mTxFirstStmtTime;
    aNodeDbc->mDistLevel       = aMetaDbc->mDistLevel;
}

/*
 * Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

package Altibase.jdbc.driver.sharding.core;

import Altibase.jdbc.driver.sharding.core.AltibaseShardingConnection;
import Altibase.jdbc.driver.sharding.core.GlobalTransactionLevel;
import Altibase.jdbc.driver.cm.CmProtocolContext;
import java.util.logging.Level;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;

/**
 * PROJ-2733-DistTxInfo
 * shard 분산정보를 관리하는 클래스
 */
public class DistTxInfo
{
    public enum ShardDistLevel
    {
        DIST_LEVEL_INIT,
        DIST_LEVEL_SINGLE,
        DIST_LEVEL_MULTI,
        DIST_LEVEL_PARALLEL
    }

    private static final byte  NON_GCTX_TX_FIRST_STMT_SCN = 1;

    private long           mSCN;      
    private long           mTxFirstStmtSCN;     
    private long           mTxFirstStmtTime;     
    private ShardDistLevel mDistLevel;   
    private short          mBeforeExecutedNodeId = -1;

    public DistTxInfo()
    {
        initDistTxInfo();
    }

    public void initDistTxInfo()
    {
        mSCN = 0;
        mTxFirstStmtSCN = 0;
        mTxFirstStmtTime = 0;
        mDistLevel = ShardDistLevel.DIST_LEVEL_INIT;
    }

    public void calcDistTxInfoForServerSide(AltibaseShardingConnection aMetaConnection)
    {
        GlobalTransactionLevel sGlobalTransactionLevel = aMetaConnection.getGlobalTransactionLevel();
        boolean sIsAutoCommit = aMetaConnection.getAutoCommit();
        long sSCN = NON_GCTX_TX_FIRST_STMT_SCN;
        
        if (sGlobalTransactionLevel == GlobalTransactionLevel.GCTX)
        {
            sSCN = CmProtocolContext.getSCN();
        }
        
        if (sIsAutoCommit)
        {
            mTxFirstStmtSCN = sSCN;
            mTxFirstStmtTime = System.currentTimeMillis();
        }
        else
        {
            if (!((mTxFirstStmtSCN == 0 && mTxFirstStmtTime == 0) || (mTxFirstStmtSCN != 0 && mTxFirstStmtTime != 0)))
            {
                shard_log(Level.SEVERE, "[1] calcDistTxInfoForServerSide failed.");
            }
            
            if (mTxFirstStmtSCN == 0)
            {
                mTxFirstStmtSCN = sSCN;
            }
            if (mTxFirstStmtTime == 0)
            {
                mTxFirstStmtTime = System.currentTimeMillis();
            }
        }

        if (sGlobalTransactionLevel == GlobalTransactionLevel.GCTX)
        {
            mSCN = sSCN;
        }
        else
        {
            mSCN = 0;
            if (!(mSCN == 0 && mTxFirstStmtSCN == NON_GCTX_TX_FIRST_STMT_SCN))
            {
                shard_log(Level.SEVERE, "[2] calcDistTxInfoForServerSide failed.");
            }
        }
        mDistLevel = ShardDistLevel.DIST_LEVEL_MULTI;
    }
    
    public void calcDistTxInfoForDataNode(AltibaseShardingConnection aMetaConnection, short aExecuteNodeCnt, short aNodeIndex)
    {
        GlobalTransactionLevel sGlobalTransactionLevel = aMetaConnection.getGlobalTransactionLevel();
        boolean sIsAutoCommit = aMetaConnection.getAutoCommit();
        long sSCN = NON_GCTX_TX_FIRST_STMT_SCN;
        
        if (sGlobalTransactionLevel == GlobalTransactionLevel.GCTX)
        {
            sSCN = CmProtocolContext.getSCN();
        }
        
        if (sIsAutoCommit)
        {
            mDistLevel = ShardDistLevel.DIST_LEVEL_INIT;
            mTxFirstStmtSCN = sSCN;
            mTxFirstStmtTime = System.currentTimeMillis();
        }
        else
        {
            if (!((mTxFirstStmtSCN == 0 && mTxFirstStmtTime == 0) || (mTxFirstStmtSCN != 0 && mTxFirstStmtTime != 0)))
            {
                shard_log(Level.SEVERE, "[1] calcDistTxInfoForDataNode failed.");
            }
            
            if (mTxFirstStmtSCN == 0)
            {
                mTxFirstStmtSCN = sSCN;
            }
            if (mTxFirstStmtTime == 0)
            {
                mTxFirstStmtTime = System.currentTimeMillis();
            }
        }
        
        if (aExecuteNodeCnt >= 2)
        {
            mDistLevel = ShardDistLevel.DIST_LEVEL_PARALLEL;
        }
        else
        {
            switch (mDistLevel)
            {
                case DIST_LEVEL_INIT:
                    mBeforeExecutedNodeId = aNodeIndex;
                    mDistLevel = ShardDistLevel.DIST_LEVEL_SINGLE;
                    break;
                case DIST_LEVEL_SINGLE:
                    if (mBeforeExecutedNodeId != aNodeIndex)
                    {
                        mDistLevel = ShardDistLevel.DIST_LEVEL_MULTI;
                    }
                    break;
                case DIST_LEVEL_MULTI:
                    break;
                case DIST_LEVEL_PARALLEL:
                    mDistLevel = ShardDistLevel.DIST_LEVEL_MULTI;
                    break;
                default:
                    // Error
                    mDistLevel = ShardDistLevel.DIST_LEVEL_MULTI;
                    break;
            }
        }
            
        if (sGlobalTransactionLevel == GlobalTransactionLevel.GCTX)
        {
            if (mDistLevel == ShardDistLevel.DIST_LEVEL_PARALLEL)
            {
                mSCN = sSCN;
            }
            else
            {
                mSCN = 0;
            }
        }
        else
        {
            mSCN = 0;
            if (!(mSCN == 0 && mTxFirstStmtSCN == NON_GCTX_TX_FIRST_STMT_SCN))
            {
                shard_log(Level.SEVERE, "[2] calcDistTxInfoForDataNode failed.");
            }
        }
    }
    
    public void propagateDistTxInfoToNode(DistTxInfo aDistTxInfo)
    {
        mSCN = aDistTxInfo.mSCN;      
        mTxFirstStmtSCN = aDistTxInfo.mTxFirstStmtSCN;     
        mTxFirstStmtTime = aDistTxInfo.mTxFirstStmtTime;     
        mDistLevel = aDistTxInfo.mDistLevel;   
    }
    
    public long getSCN()
    {
        return mSCN;
    }

    public long getTxFirstStmtSCN()
    {
        return mTxFirstStmtSCN;
    }

    public long getTxFirstStmtTime()
    {
        return mTxFirstStmtTime;
    }

    public byte getDistLevel()
    {
        return (byte)mDistLevel.ordinal();
    }
}

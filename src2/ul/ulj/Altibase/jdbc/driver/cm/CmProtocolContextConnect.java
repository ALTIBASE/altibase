/*
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

package Altibase.jdbc.driver.cm;

import java.sql.SQLException;
import java.util.ArrayList;

import Altibase.jdbc.driver.datatype.Column;
import Altibase.jdbc.driver.datatype.BigIntColumn;
import Altibase.jdbc.driver.datatype.BooleanColumn;
import Altibase.jdbc.driver.datatype.ColumnFactory;
import Altibase.jdbc.driver.datatype.IntegerColumn;
import Altibase.jdbc.driver.datatype.StringPropertyColumn;
import Altibase.jdbc.driver.datatype.TinyIntColumn;
import Altibase.jdbc.driver.sharding.core.GlobalTransactionLevel;
import Altibase.jdbc.driver.util.AltibaseProperties;

public class CmProtocolContextConnect extends CmProtocolContext
{
    private CmHandshakeResult      mHandshakeResult;
    private CmShardHandshakeResult mShardhandshakeResult;
    private ArrayList              mPropKeyList   = new ArrayList();
    private ArrayList              mPropValueList = new ArrayList();

    public CmProtocolContextConnect(CmChannel aChannel)
    {
        super(aChannel);
        mHandshakeResult = null;
        mShardhandshakeResult = null;
    }

    public CmProtocolContextConnect()
    {
        // PROJ-2690 CmProtocolContextShardConnnect에서 상속하기 때문에 default 생성자 추가
    }

    public CmHandshakeResult getHandshakeResult()
    {
        if (mHandshakeResult == null)
        {
            mHandshakeResult = new CmHandshakeResult();
        }
        return mHandshakeResult;
    }

    public CmShardHandshakeResult getShardHandshakeResult()
    {
        if (mShardhandshakeResult == null)
        {
            mShardhandshakeResult = new CmShardHandshakeResult();
        }
        return mShardhandshakeResult;
    }

    public CmConnectExResult getConnectExResult()
    {
        return (CmConnectExResult)getCmResult(CmConnectExResult.MY_OP);
    }
    
    public CmGetPropertyResult getPropertyResult()
    {
        return (CmGetPropertyResult)getCmResult(CmGetPropertyResult.MY_OP);
    }
    
    public void addProperty(short aKey, String aValue) throws SQLException
    {
        StringPropertyColumn sValue = ColumnFactory.createStringPropertyColumn();
        sValue.setValue(aValue);
        addProperty(aKey, sValue);
    }

    public void addProperty(short aKey, long aValue) throws SQLException
    {
        BigIntColumn sValue = ColumnFactory.createBigintColumn();
        sValue.setValue(aValue);
        addProperty(aKey, sValue);
    }

    public void addProperty(short aKey, int aValue) throws SQLException
    {
        IntegerColumn sValue = ColumnFactory.createIntegerColumn();
        sValue.setValue(aValue);
        addProperty(aKey, sValue);
    }

    public void addProperty(short aKey, byte aValue) throws SQLException
    {
        TinyIntColumn sValue = ColumnFactory.createTinyIntColumn();
        sValue.setValue(aValue);
        addProperty(aKey, sValue);
    }

    public void addProperty(short aKey, boolean aValue) throws SQLException
    {
        BooleanColumn sValue = ColumnFactory.createBooleanColumn();
        sValue.setValue(aValue);
        addProperty(aKey, sValue);
    }

    public void addProperty(short aKey, Column aValue)
    {
        mPropKeyList.add(aKey);
        mPropValueList.add(aValue);
    }

    public void clearProperties()
    {
        mPropKeyList.clear();
        mPropValueList.clear();
    }

    /* BUG-39817 */
    public Column getPropertyColumn(short aKey)
    {
        for (int i = 0; i < mPropKeyList.size(); i++)
        {
            if ((Short)mPropKeyList.get(i) == aKey)
            {
                return (Column) mPropValueList.get(i);
            }
        }
        return null;
    }

    public boolean isSetProperty(short aKey) throws SQLException
    {
        Column sColumn = getPropertyColumn(aKey);
        return sColumn != null;
    }

    public boolean isSetPropertyResult(short aKey) throws SQLException
    {
        Column sColumn = getPropertyResult().getPropertyColumn(aKey);
        return sColumn != null;
    }
    
    public int getLobCacheThreshold() throws NumberFormatException, SQLException
    {
        return Integer.parseInt(getPropertyResult().getProperty(AltibaseProperties.PROP_CODE_LOB_CACHE_THRESHOLD));
    }
    
    public String getCharsetName() throws SQLException
    {
        return getPropertyResult().getProperty(AltibaseProperties.PROP_CODE_NLS_CHARACTERSET);
    }
    
    public String getNCharsetName() throws SQLException
    {
        return getPropertyResult().getProperty(AltibaseProperties.PROP_CODE_NLS_NCHAR_CHARACTERSET);
    }

    public byte getExplainPlanMode() throws SQLException
    {
        Column sColumn = getPropertyResult().getPropertyColumn(AltibaseProperties.PROP_CODE_EXPLAIN_PLAN);
        if (sColumn == null) {
            return -1;
        }
        return sColumn.getByte();
    }

    /* BUG-39817 */
    public int getIsolationLevel() throws NumberFormatException, SQLException
    {
        return Integer.parseInt(getPropertyResult().getProperty(AltibaseProperties.PROP_CODE_ISOLATION_LEVEL));
    }

    public GlobalTransactionLevel getGlobalTransactionLevel() throws NumberFormatException, SQLException
    {
        short sPropValue = Short.parseShort(getPropertyResult().getProperty(AltibaseProperties.PROP_CODE_GLOBAL_TRANSACTION_LEVEL));
        return GlobalTransactionLevel.get(sPropValue);
    }

    public short getShardStatementRetry() throws NumberFormatException, SQLException
    {
        return Short.parseShort(getPropertyResult().getProperty(AltibaseProperties.PROP_CODE_SHARD_STATEMENT_RETRY));
    }

    public int getIndoubtFetchTimeout() throws NumberFormatException, SQLException
    {
        return Integer.parseInt(getPropertyResult().getProperty(AltibaseProperties.PROP_CODE_INDOUBT_FETCH_TIMEOUT));
    }

    public short getIndoubtFetchMethod() throws NumberFormatException, SQLException
    {
        return Short.parseShort(getPropertyResult().getProperty(AltibaseProperties.PROP_CODE_INDOUBT_FETCH_METHOD));
    }

    int getPropertyCount()
    {
        return mPropKeyList.size();
    }
    
    short getPropertyKey(int aIndex)
    {
        return ((Short)mPropKeyList.get(aIndex)).shortValue();
    }
    
    Column getPropertyValue(int aIndex)
    {
        return (Column)mPropValueList.get(aIndex);
    }
    
    public String toString()
    {
        StringBuffer sStr = new StringBuffer();
        
        for (int sIdx=0; sIdx < mPropKeyList.size(); sIdx++)
        {
            Column sColumn = (Column)mPropValueList.get(sIdx);
            String sPropertyValue = "";
            try
            {
                sPropertyValue = sColumn.getString();
            }
            catch (SQLException e)
            {
                e.printStackTrace();
            }
            
            sStr.append("{ID=").append(mPropKeyList.get(sIdx)).append(", value=").append(sPropertyValue).append("}");
        }
        
        return sStr.toString();
    }
}

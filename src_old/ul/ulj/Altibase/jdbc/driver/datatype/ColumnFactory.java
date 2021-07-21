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

package Altibase.jdbc.driver.datatype;

import java.nio.charset.CharsetEncoder;
import java.sql.Types;
import java.util.HashMap;

import Altibase.jdbc.driver.AltibaseTypes;
import Altibase.jdbc.driver.util.AltibaseProperties;
import Altibase.jdbc.driver.util.StringUtils;

public class ColumnFactory
{
    private static HashMap<String, Class> mMTDTypeMap  = new HashMap<String, Class>();
    private static HashMap<String, Class> mJDBCTypeMap = new HashMap<String, Class>();

    private AltibaseProperties mProps;
    private CharsetEncoder mDBEncoder;
    private CharsetEncoder mNCharEncoder;

    static
    {
        // BUGBUG (2012-11-15) MTD Type 또는 JDBC Type이 겹치는 것들은 대표 타입이 나중에 등록되어야 한다.

        // Altibase는 DATE, TIME을 지원하지 않는다. (DATE는 실제론 TIMESTAMP임)
        // 그래서, 아래 세 컬럼은 모두 MTD Type이 같다. TIMESTAMP가 대표.
        register(new DateColumn());
        register(new TimeColumn());
        register(new TimestampColumn());

        register(new BinaryColumn());
        register(new BitColumn());
        register(new VarbitColumn());
        register(new NibbleColumn());
        register(new ByteColumn());
        register(new VarbyteColumn());
        register(new FloatColumn());
        register(new RealColumn());
        register(new NCharColumn());
        register(new CharColumn());
        register(new NVarcharColumn());
        register(new VarcharColumn());
        register(new NumberColumn());
        register(new NumericColumn());
        register(new NullColumn());
        register(new BigIntColumn());
        register(new BlobLocatorColumn());
        register(new BooleanColumn());
        register(new ClobLocatorColumn());
        register(new DoubleColumn());
        register(new IntegerColumn());
        register(new SmallIntColumn());
        register(new IntervalColumn());
    }

    public ColumnFactory()
    {
    }
    
    public void setProperties(AltibaseProperties aProps)
    {
        mProps = aProps;
    }
    
    private static void register(Column aType)
    {
        mMTDTypeMap.put(String.valueOf(aType.getDBColumnType()), aType.getClass());
        for (int sJDBCType : aType.getMappedJDBCTypes())
        {
            mJDBCTypeMap.put(String.valueOf(sJDBCType), aType.getClass());
        }
    }

    public Column getInstance(int aDBType)
    {
        return getRepresentativeColumn(aDBType);
    }

    /**
     * DB Type에 해당하는 Column 객체를 얻는다.
     * 
     * @param aDBType {@link AltibaseTypes}에 정의된 type 상수
     * @return Column 객체
     */
    private Column getRepresentativeColumn(int aDBType)
    {
        try
        {
            Column sColumn = (Column)(mMTDTypeMap.get(String.valueOf(aDBType)).newInstance());
            
            switch(aDBType)
            {
                case ColumnTypes.VARCHAR :
                case ColumnTypes.CHAR :
                case ColumnTypes.NVARCHAR :
                case ColumnTypes.NCHAR :
                    CommonCharVarcharColumn sVarcharColumn = ((CommonCharVarcharColumn)sColumn);
                    // PROJ-2427 getBytes시 사용하기 위한 Encoder를 setter메소드를 이용해 전달한다.
                    sVarcharColumn.setDBEncoder(mDBEncoder);
                    sVarcharColumn.setNCharEncoder(mNCharEncoder);
                    // BUG-43807 ColumnReader를 삭제하고 바로 Column객체에 redundant관련 플래그를 셋팅한다.
                    sVarcharColumn.setRemoveRedundantMode(mProps.isOnRedundantDataTransmission());
                    break;
                case ColumnTypes.TIMESTAMP:
                    // BUG-46513 date관련 컬럼일때는 date_format jdbc 속성값을 셋팅해 준다.
                    String sDateFormatStr = mProps.getDateFormat();
                    if (!StringUtils.isEmpty(sDateFormatStr))
                    {
                        CommonDateTimeColumn sDateColumn = (CommonDateTimeColumn)sColumn;
                        sDateColumn.setDateFormat(sDateFormatStr);
                    }
                    break;
                default :
                    break;
            }
            
            return sColumn;
        }
        catch (Exception e)
        {
            return null;
        }
    }

    /**
     * JDBC Type에 해당하는 Column 객체를 얻는다.
     * 
     * @param aJDBCType {@link java.sql.Types}에 정의된 type 상수
     * @return Column 객체
     */
    public Column getMappedColumn(int aJDBCType)
    {
        if (aJDBCType == Types.OTHER)
        {
            return null;
        }

        try
        {
            Column sColumn = (Column)(mJDBCTypeMap.get(String.valueOf(aJDBCType)).newInstance());
            
            switch(aJDBCType)
            {
                case AltibaseTypes.VARCHAR :
                case AltibaseTypes.CHAR :
                case AltibaseTypes.NVARCHAR :
                case AltibaseTypes.NCHAR :
                    // BUG-43807 ColumnReader를 삭제하고 바로 Column객체에 redundant관련 플래그를 셋팅한다.
                    ((CommonCharVarcharColumn)sColumn).setRemoveRedundantMode(mProps.isOnRedundantDataTransmission());
                    break;
                default :
                    break;
            }
                return sColumn;
        }
        catch (Exception e)
        {
            return null;
        }
    }
    
    public VarcharColumn createVarcharColumn()
    {
        VarcharColumn sColumn = new VarcharColumn();
        boolean isRedundant = ( (mProps != null) && mProps.isOnRedundantDataTransmission() );
        sColumn.setRemoveRedundantMode(isRedundant);
        return sColumn;
    }

    public static BigIntColumn createBigintColumn()
    {
        return new BigIntColumn();
    }
    
    /**
     * BUG-39149 ping 쿼리에서 SmallIntColumn을 수동으로 생성하기 위해 메소드 추가
     */
    public static SmallIntColumn createSmallintColumn()
    {
        return new SmallIntColumn();
    }
    
    public static IntegerColumn createIntegerColumn()
    {
        return new IntegerColumn();
    }

    public static TinyIntColumn createTinyIntColumn()
    {
        return new TinyIntColumn();
    }

    public static BooleanColumn createBooleanColumn()
    {
        return new BooleanColumn();
    }

    public static StringPropertyColumn createStringPropertyColumn()
    {
        return new StringPropertyColumn();
    }

    public void setCharSetEncoder(CharsetEncoder aDBEncoder)
    {
        this.mDBEncoder = aDBEncoder;
    }

    public void setNCharSetEncoder(CharsetEncoder aNCharEncoder)
    {
        this.mNCharEncoder = aNCharEncoder;
    }
}

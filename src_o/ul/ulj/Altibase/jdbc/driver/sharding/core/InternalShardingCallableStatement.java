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

import java.math.BigDecimal;
import java.sql.*;
import java.util.Calendar;

/**
 * ServerSide 또는 ClientSide에서 공통적으로 실행되는 java.sql.CallableStatement 인터페이스 모음
 */
public interface InternalShardingCallableStatement extends InternalShardingPreparedStatement
{
    boolean wasNull() throws SQLException;
    String getString(int aParameterIndex) throws SQLException;
    boolean getBoolean(int aParameterIndex) throws SQLException;
    byte getByte(int aParameterIndex) throws SQLException;
    short getShort(int aParameterIndex) throws SQLException;
    int getInt(int aParameterIndex) throws SQLException;
    long getLong(int aParameterIndex) throws SQLException;
    float getFloat(int aParameterIndex) throws SQLException;
    double getDouble(int aParameterIndex) throws SQLException;
    BigDecimal getBigDecimal(int aParameterIndex, int aScale) throws SQLException;
    byte[] getBytes(int aParameterIndex) throws SQLException;
    Date getDate(int aParameterIndex) throws SQLException;
    Time getTime(int aParameterIndex) throws SQLException;
    Timestamp getTimestamp(int aParameterIndex) throws SQLException;
    Object getObject(int aParameterIndex) throws SQLException;
    BigDecimal getBigDecimal(int aParameterIndex) throws SQLException;
    Blob getBlob(int aParameterIndex) throws SQLException;
    Clob getClob(int aParameterIndex) throws SQLException;
    Date getDate(int aParameterIndex, Calendar aCal) throws SQLException;
    Time getTime(int aParameterIndex, Calendar aCal) throws SQLException;
    Timestamp getTimestamp(int aParameterIndex, Calendar aCal) throws SQLException;
    String getString(String aParameterName) throws SQLException;
    boolean getBoolean(String aParameterName) throws SQLException;
    byte getByte(String aParameterName) throws SQLException;
    short getShort(String aParameterName) throws SQLException;
    int getInt(String aParameterName) throws SQLException;
    long getLong(String aParameterName) throws SQLException;
    float getFloat(String aParameterName) throws SQLException;
    double getDouble(String aParameterName) throws SQLException;
    byte[] getBytes(String aParameterName) throws SQLException;
    Date getDate(String aParameterName) throws SQLException;
    Time getTime(String aParameterName) throws SQLException;
    Timestamp getTimestamp(String aParameterName) throws SQLException;
    Object getObject(String aParameterName) throws SQLException;
    BigDecimal getBigDecimal(String aParameterName) throws SQLException;
    Blob getBlob(String aParameterName) throws SQLException;
    Clob getClob(String aParameterName) throws SQLException;
    Date getDate(String aParameterName, Calendar aCal) throws SQLException;
    Time getTime(String aParameterName, Calendar aCal) throws SQLException;
    Timestamp getTimestamp(String aParameterName, Calendar aCal) throws SQLException;
}

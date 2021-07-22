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

import java.io.InputStream;
import java.io.Reader;
import java.math.BigDecimal;
import java.sql.Blob;
import java.sql.Clob;
import java.sql.Date;
import java.sql.SQLException;
import java.sql.Time;
import java.sql.Timestamp;
import java.util.Calendar;
import java.util.Set;

import Altibase.jdbc.driver.cm.CmBufferWriter;
import Altibase.jdbc.driver.cm.CmChannel;
import Altibase.jdbc.driver.cm.CmFetchResult;
import Altibase.jdbc.driver.util.DynamicArray;

public interface Column
{
    int getDBColumnType();

    Set<Integer> getMappedJDBCTypes();

    boolean isMappedJDBCType(int aSqlType);

    String getDBColumnTypeName();

    String getObjectClassName();

    int getMaxDisplaySize();

    /**
     * SQL/CLI 의 SQL_DESC_OCTET_LENGTH 에 대응되는 column 의 최대 길이를 반환한다.
     */
    int getOctetLength();

    DynamicArray createTypedDynamicArray();

    boolean isArrayCompatible(DynamicArray aArray);

    void storeTo(DynamicArray aArray);

    // PROJ-2368
    /**
     * List Protocol 을 위한 ByteBuffer 에 프로토콜에서 정의한 바이너리 형태로 데이타를 쓴다.
     * 
     * @param aBufferWriter 데이타를 쓸 ByteBuffer Writer
     * @throws SQLException 데이타 전송에 실패했을 경우
     */
    void storeTo(ListBufferHandle aBufferWriter) throws SQLException;

    /**
     * {@link #writeTo(CmBufferWriter)}로 에 쓸 준비를 하고, 쓸 데이타의 길이를 반환한다.
     * 
     * @param aBufferWriter 데이타를 쓸 ByteBuffer Writer
     * @return 쓸 데이타의 바이트 길이
     * @throws SQLException 쓸 데이타를 준비하는데 실패했을 경우
     */
    int prepareToWrite(CmBufferWriter aBufferWriter) throws SQLException;

    /**
     * ByteBuffer에 프로토콜에서 정의한 바이너리 형태로 데이타를 쓴다.
     * 
     * @param aBufferWriter 데이타를 쓸 ByteBuffer Writer
     * @return ByteBuffer에 쓴 데이타 사이즈(byte 단위)
     * @throws SQLException 데이타 전송에 실패했을 경우
     */
    int writeTo(CmBufferWriter aBufferWriter) throws SQLException;

    void getDefaultColumnInfo(ColumnInfo aInfo);

    boolean isNumberType();

    /**
     * 채널로부터 컬럼데이터를 읽어와 해당하는 인덱스의 DynamicArray에 바로 저장한다.
     * @param aChannel 소켓통신을 위한 채널객체
     * @param aFetchResult DynamicArray에 대한 인터페이스를 가지고 있는 객체
     * @throws SQLException 소켓통신시 예외가 발생한 경우
     */
    void readFrom(CmChannel aChannel, CmFetchResult aFetchResult) throws SQLException;

    void readParamsFrom(CmChannel aChannel) throws SQLException;

    void setMaxBinaryLength(int aMaxLength);

    void loadFrom(DynamicArray aArray);

    void setColumnInfo(ColumnInfo aInfo);

    ColumnInfo getColumnInfo();

    boolean isNull();

    void setValue(Object aValue) throws SQLException;

    boolean getBoolean() throws SQLException;

    byte getByte() throws SQLException;

    byte[] getBytes() throws SQLException;

    short getShort() throws SQLException;

    int getInt() throws SQLException;

    long getLong() throws SQLException;

    float getFloat() throws SQLException;

    double getDouble() throws SQLException;

    BigDecimal getBigDecimal() throws SQLException;

    String getString() throws SQLException;

    Date getDate() throws SQLException;

    Date getDate(Calendar aCalendar) throws SQLException;

    Time getTime() throws SQLException;

    Time getTime(Calendar aCalendar) throws SQLException;

    Timestamp getTimestamp() throws SQLException;

    Timestamp getTimestamp(Calendar aCalendar) throws SQLException;

    InputStream getAsciiStream() throws SQLException;

    InputStream getBinaryStream() throws SQLException;

    Reader getCharacterStream() throws SQLException;

    Blob getBlob() throws SQLException;

    Clob getClob() throws SQLException;

    Object getObject() throws SQLException;

    /**
     * 컬럼 인덱스를 셋팅한다.
     * @param aColumnIndex 컬럼 인덱스
     */
    void setColumnIndex(int aColumnIndex);

    /**
     * 컬럼의 인덱스를 가져온다.
     * @return 컬럼 인덱스
     */
    int getColumnIndex();

    /**
     * 컬럼 데이터를 ArrayList에 저장한다.
     */
    void storeTo();

    /**
     * ArrayList로 부터 컬럼데이터를 읽어들인다.
     * @param aLoadIndex
     */
    void loadFrom(int aLoadIndex);

    /**
     * 컬럼데이터가 저장되어 있는 ArrayList를 초기화 한다.
     */
    void clearValues();
}

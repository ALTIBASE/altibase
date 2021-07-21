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

package Altibase.jdbc.driver;

import java.io.IOException;
import java.io.InputStream;
import java.io.Reader;
import java.sql.Blob;
import java.sql.Clob;
import java.sql.SQLException;
import java.util.LinkedList;

import Altibase.jdbc.driver.datatype.BlobLocatorColumn;
import Altibase.jdbc.driver.datatype.ClobLocatorColumn;
import Altibase.jdbc.driver.datatype.Column;
import Altibase.jdbc.driver.datatype.LobLocatorColumn;
import Altibase.jdbc.driver.datatype.DynamicArrayRowHandle;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

class LobUpdator
{
    // LobUpdator의 add method는 Update 작업을 위한 기본값 설정을 담당한다.
    // LobUpdator의 update method에서 그 data를 기반으로 protocol을 수행하여 서버에 전송한다.
    private class LobParams
    {
        Object mTargetLob;
        Object mSource;
        long mLength;

        LobParams(Object aTargetLobLocator, Object aSource, long aLength)
        {
            mTargetLob = aTargetLobLocator;
            mSource = aSource;
            mLength = aLength;
        }
    }

    private LinkedList  mUpdatees   = new LinkedList();
    private AltibasePreparedStatement mPreparedStatement;
    
    LobUpdator(AltibasePreparedStatement aPreparedStmt)
    {
        this.mPreparedStatement = aPreparedStmt;
    }
    
    public void addLobColumn(LobLocatorColumn aTargetLobLocator, Object aSourceValue, long aLength)
    {
        mUpdatees.add(new LobParams(aTargetLobLocator, aSourceValue, aLength));
    }
    
    public void addClobColumn(LobLocatorColumn aTargetLobLocator, Object aSourceValue) throws SQLException
    {
        if(aSourceValue instanceof char[])
        {
            mUpdatees.add(new LobParams(aTargetLobLocator, aSourceValue, ((char[])aSourceValue).length));
        }
        else if(aSourceValue instanceof String)
        {
            mUpdatees.add(new LobParams(aTargetLobLocator, aSourceValue, ((String) aSourceValue).length()));
        }
        else if(aSourceValue instanceof Clob || aSourceValue == null)
        {
            // Clob 객체는 길이를 객체에서 획득할 수 있으므로 0으로 설정한다.
            // BUG-38681 value가 null일때도 mUpdatees에 넣어준다.
            mUpdatees.add(new LobParams(aTargetLobLocator, aSourceValue, 0));
        }
        else
        {
            Error.throwSQLException(ErrorDef.INVALID_TYPE_CONVERSION, aSourceValue.getClass().getName(), "CLOB");
        }
    }
    
    public void addBlobColumn(LobLocatorColumn aTargetLobLocator, Object aSourceValue) throws SQLException
    {
        if(aSourceValue instanceof byte[])
        {
            mUpdatees.add(new LobParams(aTargetLobLocator, aSourceValue, ((byte[])aSourceValue).length));
        }
        else if(aSourceValue instanceof Blob || aSourceValue == null)
        {
            // Blob 객체는 길이를 객체에서 획득할 수 있으므로 0으로 설정한다.
            // BUG-38681 value가 null일때도 mUpdatees에 넣어준다.
            mUpdatees.add(new LobParams(aTargetLobLocator, aSourceValue, 0));
        }
        else
        {
            Error.throwSQLException(ErrorDef.INVALID_TYPE_CONVERSION, aSourceValue.getClass().getName(), "BLOB");
        }
    }
    
    void updateLobColumns() throws SQLException
    {
        int sLobColumnCount = 0;
        int sExecutionCount = 0;
        
        if (mPreparedStatement.mBatchAdded)
        {
            for (int i = 0; i < mPreparedStatement.mBindColumns.size(); i++)
            {
                if ((Column)mPreparedStatement.mBindColumns.get(i) instanceof LobLocatorColumn)
                {
                    sLobColumnCount++;
                }
            }
        }
        
        while (!mUpdatees.isEmpty())
        {
            LobParams sParams = (LobParams)mUpdatees.removeFirst();
         
            // addBatch를 통해 BatchRowHandle의 dynamic array에 데이터는 이미 저장되어 있다. 
            // 최종적으로 executeBatch()를 수행한 경우 이 루틴에 도달한다.
            if(mPreparedStatement.mBatchAdded)
            {
                // 한 row당 LOB Column 갯수만큼 update를 수행한 후 다음 row로 넘어가기 위해 rowhandle을 다음 row로 넘긴다.
                if(sExecutionCount % sLobColumnCount == 0)
                {
                	((DynamicArrayRowHandle)mPreparedStatement.mBatchRowHandle).next();
                }
                
                sExecutionCount++;
            }
            
            // BUG-26327, BUG-37418
            if(((LobLocatorColumn)sParams.mTargetLob).isNullLocator())
            {
                continue;
            }
            
            // Target은 data를 삽입하려는 대상 Table
            // Source는 삽입하려는 data를 의미한다.
            // Column Type에 따라 동작을 달리한다.
            if (sParams.mTargetLob instanceof BlobLocatorColumn)
            {
                AltibaseBlob sBlob = (AltibaseBlob)((BlobLocatorColumn)sParams.mTargetLob).getBlob();
                InputStream sSource = null;
                long sLobLength = 0;
                
                if (sParams.mSource instanceof Blob)
                {
                    // Blob 객체는 BinaryStream으로 변환하여 처리한다.
                    sSource = ((Blob)sParams.mSource).getBinaryStream();
                    sLobLength = ((Blob)sParams.mSource).length();
                    updateBlob(sBlob, sSource, sLobLength);
                }
                else if(sParams.mSource instanceof InputStream)
                {
                    sSource = (InputStream)sParams.mSource;
                    sLobLength = sParams.mLength;
                    updateBlob(sBlob, sSource, sLobLength);
                }
                else if(sParams.mSource instanceof byte[])
                {
                    byte[] sSourceAsByteArr = (byte[])sParams.mSource;
                    sLobLength = sSourceAsByteArr.length;
                    updateBlob(sBlob, sSourceAsByteArr);
                }
                else if (sParams.mSource == null)
                {
                    // BUG-38681 CmProtocol.preparedExecuteBatch를 먼저 호출하면서 해당 값이 null로 업데이트 되기 때문에
                    // 여기서는 null체크만 하고 넘어간다.
                }
                else
                {
                    Error.throwSQLException(ErrorDef.INVALID_DATA_CONVERSION, String.valueOf(sParams.mSource), "BLOB");
                }
                
                // 해당 Column에 데이터 하나를 삽입하면 해당 locator를 해제한다.
                // INSERT의 경우 서버에 대한 LOCATOR를 획득할 방법이 없으므로 해제해도 문제가 없다.
                sBlob.free();
            }
            else if (sParams.mTargetLob instanceof ClobLocatorColumn)
            {
                AltibaseClob sClob = (AltibaseClob)((ClobLocatorColumn)sParams.mTargetLob).getClob();
                
                if (sParams.mSource instanceof Clob)
                {
                    updateClob(sClob, ((Clob)sParams.mSource).getCharacterStream(), 0);
                }
                else if(sParams.mSource instanceof Reader)
                {
                    updateClob(sClob, (Reader)sParams.mSource, sParams.mLength);
                }
                else if(sParams.mSource instanceof InputStream)
                {
                    InputStream sSource = (InputStream)sParams.mSource;
                    // Byte Length
                    updateClob(sClob, sSource, sParams.mLength);
                }
                else if(sParams.mSource instanceof char[])
                {
                    updateClob(sClob, (char[])sParams.mSource);
                }
                else if(sParams.mSource instanceof String)
                {
                    updateClob(sClob, ((String)sParams.mSource).toCharArray());
                }
                else if (sParams.mSource == null)
                {
                    // BUG-38681 CmProtocol.preparedExecuteBatch를 먼저 호출하면서 해당 값이 null로 업데이트 되기 때문에
                    // 여기서는 null체크만 하고 넘어간다.
                }
                else
                {
                    Error.throwSQLException(ErrorDef.INVALID_DATA_CONVERSION, String.valueOf(sParams.mSource), "CLOB");
                }
                
                sClob.free();
            }
        }
    }

    private void updateClob(AltibaseClob aTarget, char[] aSource) throws SQLException
    {
        aTarget.open(mPreparedStatement.mConnection.channel());
        ClobWriter sOut = (ClobWriter) aTarget.setCharacterStream(1);

        try
        {
            sOut.write(aSource);
        } 
        catch (IOException sException)
        {
            Error.throwCommunicationErrorException(sException);
        }
    }

    private void updateBlob(AltibaseBlob aTarget, byte[] sSourceAsByteArr) throws SQLException
    {
        aTarget.open(mPreparedStatement.mConnection.channel());
        aTarget.setBytes(1, sSourceAsByteArr);
    }

    private void updateLob(BlobOutputStream aTarget, InputStream aSource, long aLength) throws SQLException
    {
        try
        {
            // 만약 한 Connection에서 Source와 Target이 주어진다면
            // 같은 channel을 사용하기 때문에 channel의 byte buffer를 연산을 수행할때마다 반드시 다 비워야 한다.
            // 본래는 별도의 buffer를 두어 데이터를 항상 buffer로 복사해 두었으나 이는 복사비용을 불러일으켜 성능을 저하시킨다.
            // Source와 Target이 다른 connection을 사용하고 있다면 buffer copy 비용은 부하이므로
            // 같은 connection을 사용하는 경우에만 별도의 buffer에 일괄 복사하도록 처리한다.
            // 단, 다른 Connection일 때도 가져오려는 데이터 크기가 (실제로는 전체 데이터가 더 클지라도 요청하는 양이 적을 수 있다.)
            // 매우 작을 경우 별도의 buffer를 이용하는 것이 통신량을 줄일 수 있어 효율적이므로 이때는 별도의 buffer를 활용한다.
            
            if(aSource instanceof ConnectionSharable)
            {
                ConnectionSharable sConnSharable = (ConnectionSharable)aSource;
                if(sConnSharable.isSameConnectionWith(mPreparedStatement.mConnection.channel()))
                {
                    ((ConnectionSharable)aSource).setCopyMode();
                    ((ConnectionSharable)aSource).readyToCopy();
                    aTarget.setCopyMode();
                }
            }
            
            aTarget.write(aSource, aLength);
        }
        catch (IOException sException)
        {
            Error.throwCommunicationErrorException(sException);
        }
    }
    
    private void updateBlob(AltibaseBlob aTarget, InputStream aSource, long aLength) throws SQLException
    {
        // 삽입하려는 Column 객체 (aTarget)에 channel 레퍼런스를 전달한다.
        aTarget.open(mPreparedStatement.mConnection.channel());
        // OutputStream을 획득하여 update작업을 수행함
        BlobOutputStream sOut = (BlobOutputStream)aTarget.setBinaryStream(1);
        updateLob(sOut, aSource, aLength);
    }

    private void updateClob(AltibaseClob aTarget, InputStream aSource, long aLength) throws SQLException
    {
        aTarget.open(mPreparedStatement.mConnection.channel());
        BlobOutputStream sOut = (BlobOutputStream)aTarget.setAsciiStream(1);
        updateLob(sOut, aSource, aLength);
    }
    
    private void updateClob(AltibaseClob aTarget, Reader aSource, long aLength) throws SQLException
    {
        aTarget.open(mPreparedStatement.mConnection.channel());
        ClobWriter sOut = (ClobWriter)aTarget.setCharacterStream(1);
        
        try
        {
            if ( (aSource instanceof ConnectionSharable)
              && ((ConnectionSharable) aSource).isSameConnectionWith(mPreparedStatement.mConnection.channel()) )
            {
                ((ConnectionSharable)aSource).setCopyMode();
                ((ConnectionSharable)aSource).readyToCopy();
                sOut.setCopyMode();

                sOut.write(aSource, (int)aLength);

                ((ConnectionSharable)aSource).releaseCopyMode();
                sOut.releaseCopyMode();
            }
            else
            {
                sOut.write(aSource, (int)aLength);
            }
        } 
        catch (IOException sException)
        {
            Error.throwCommunicationErrorException(sException);
        }
    }
}

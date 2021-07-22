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

import java.sql.SQLException;
import java.util.BitSet;

import Altibase.jdbc.driver.AltibaseBitSet;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

abstract class CommonBitColumn extends AbstractBinaryColumn
{
    private static final int NULL_VALUE_LENGTH = 0;

    CommonBitColumn()
    {
        super(LENGTH_SIZE_INT32);
    }

    public CommonBitColumn(int aPrecision)
    {
        super(LENGTH_SIZE_INT32, aPrecision);
    }

    protected int toByteLength(int aBitLength)
    {
        return (aBitLength + 7) >> 3;
    }

    protected int nullLength()
    {
        return NULL_VALUE_LENGTH;
    }

    static interface BitIterationVisitor
    {
        void visitBit(int aIndex, boolean aBit);
    }

    static interface BitStreamIterator
    {
        int getLength();
        boolean get(int aIndex) throws SQLException;
    }
    
    static class StringBitStreamIterator implements BitStreamIterator
    {
        private String mString;
        
        StringBitStreamIterator(String aValue)
        {
            mString = aValue;
        }
        
        public int getLength()
        {
            return mString.length();
        }
        
        public boolean get(int aIndex) throws SQLException
        {
            switch (mString.charAt(aIndex))
            {
                case '1':
                    return true;
                case '0':
                    return false;

                default:
                    Error.throwSQLException(ErrorDef.INVALID_BIT_CHARACTER, String.valueOf(mString.charAt(aIndex)));
                    return false;
            }
        }
    }
    
    static class BitSetStreamIterator implements BitStreamIterator
    {
        private BitSet mBitSet;
        
        BitSetStreamIterator(BitSet aValue)
        {
            mBitSet = aValue;
        }
        
        public int getLength()
        {
            return mBitSet.length();
        }
        
        public boolean get(int aIndex) throws SQLException
        {
            return mBitSet.get(aIndex);
        }
    }
    
    public String getObjectClassName()
    {
        return BitSet.class.getName();
    }

    public int getMaxDisplaySize()
    {
        return getColumnInfo().getPrecision();
    }

    public int getOctetLength()
    {
        return LENGTH_SIZE_INT32 + toByteLength(getColumnInfo().getPrecision());
    }

    private int getReturnLength()
    {
        int sLength = mLength;
        if (getMaxBinaryLength() > 0 && getMaxBinaryLength() < sLength)
        {
            sLength = getMaxBinaryLength();
        }
        return sLength;
    }

    /*
     * 이 메소드의 스펙은 1바이트가 넘을 경우엔 에러를, 1바이트일 경우 그 바이트를 리턴한다.
     */
    protected byte getByteSub() throws SQLException
    {
        byte sResult = 0;
        mByteBuffer.rewind();

        // bit 8개가 1바이트이다.
        // 1바이트가 아니면 byte로 형변환할 수 없다.
        if (mLength != 8)
        {
            Error.throwSQLException(ErrorDef.INVALID_DATA_CONVERSION, getStringSub(), "byte");
        }

        sResult = mByteBuffer.get();
        return sResult;
    }

    protected byte[] getBytesSub() throws SQLException
    {
        mByteBuffer.rewind();
        byte[] sBytes = new byte[mByteBuffer.remaining()];
        mByteBuffer.get(sBytes);
        return sBytes;
    }

    protected String getStringSub() throws SQLException
    {
        class StringGenerator implements BitIterationVisitor
        {
            StringBuffer mStringBuf = new StringBuffer();

            public void visitBit(int aIndex, boolean aBit)
            {
                mStringBuf.append(aBit ? '1' : '0');
            }

            String getString()
            {
                return mStringBuf.toString();
            }
        }
        StringGenerator sBitGen = new StringGenerator();
        iterateBit(sBitGen);
        return sBitGen.getString();
    }

    protected Object getObjectSub() throws SQLException
    {
        class BitSetGenerator implements BitIterationVisitor
        {
            BitSet mResult = new AltibaseBitSet(getReturnLength());

            public void visitBit(int aIndex, boolean aBit)
            {
                mResult.set(aIndex, aBit);
            }

            BitSet getBitSet()
            {
                return mResult;
            }
        }
        BitSetGenerator sBitGen = new BitSetGenerator();
        iterateBit(sBitGen);
        return sBitGen.getBitSet();
    }

    private void setBitStreamValue(BitStreamIterator aValue) throws SQLException
    {
        if (aValue.getLength() > ColumnConst.MAX_BIT_LENGTH)
        {
            Error.throwSQLException(ErrorDef.VALUE_LENGTH_EXCEEDS,
                                    String.valueOf(aValue.getLength()),
                                    String.valueOf(ColumnConst.MAX_BIT_LENGTH));
        }

        ensureAlloc(toByteLength(aValue.getLength()));
        mLength = aValue.getLength();
        int sByte = 0;
        int sBit = 0x80;
        for (int i = 0; i < mLength; i++)
        {            
            if (aValue.get(i))
            {
                sByte |= sBit;
            }
            if (sBit == 1)
            {
                mByteBuffer.put((byte)(sByte));
                sBit = 0x80;
                sByte = 0;
            }
            else
            {
                sBit >>= 1;
            }            
        }
        if (sBit != 0x80)
        {
            mByteBuffer.put((byte)(sByte));
        }
        mByteBuffer.flip();
    }

    protected void setValueSub(Object aValue) throws SQLException
    {
        if (aValue instanceof CommonBitColumn)
        {
            CommonBitColumn sColumn = (CommonBitColumn)aValue;
            sColumn.mByteBuffer.rewind();
            ensureAlloc(sColumn.mByteBuffer.limit());
            mByteBuffer.put(sColumn.mByteBuffer);
            mByteBuffer.flip();
            mLength = sColumn.mLength;
        }
        else if (aValue instanceof String)
        {
            setBitStreamValue(new StringBitStreamIterator((String)aValue));            
        }
        else if (aValue instanceof BitSet)
        {
            setBitStreamValue(new BitSetStreamIterator((BitSet)aValue));
        }
        else if (aValue instanceof Number)
        {
            setBitStreamValue(new StringBitStreamIterator(Long.toBinaryString(((Number)aValue).longValue())));            
        }
        else if (aValue instanceof Boolean)
        {
            boolean sBoolValue = ((Boolean)aValue).booleanValue();
            
            if (sBoolValue) 
            {
                setBitStreamValue(new StringBitStreamIterator("1"));
            }
            else
            {
                setBitStreamValue(new StringBitStreamIterator("0"));
            }
        }
        else
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_TYPE_CONVERSION,
                                    aValue.getClass().getName(), getDBColumnTypeName());
        }

        // 실제 값에 맞게 Precision을 보정. 안그러면 다음과 같은 에러가 날 수 있다:
        // qpERR_ABORT_QCI_INVALID_HOST_DATA_SIZE
        // : ERR-311B8 [08000] Size of data to bind to host variable is invalid
        getColumnInfo().modifyPrecision(mLength);
    }

    private void iterateBit(BitIterationVisitor aVisitor)
    {
        int sIndex = 0;
        mByteBuffer.rewind();
        int sLength = getReturnLength();
        while (mByteBuffer.hasRemaining())
        {
            short sOneByte = mByteBuffer.get();
            for (short i = 0x80; i >= 0x01; i >>= 1)
            {
                if (sIndex >= sLength)
                {
                    // bit length에 도달하면 중단한다.
                    // i는 현재 byte의 끝까지 가기 때문에 중단해줘야 한다.
                    break;
                }
                aVisitor.visitBit(sIndex++, (sOneByte & i) != 0);
            }
        }
    }
}

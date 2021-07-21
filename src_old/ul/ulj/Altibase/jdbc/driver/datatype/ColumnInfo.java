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

import java.sql.Types;

import Altibase.jdbc.driver.AltibaseStatement;
import Altibase.jdbc.driver.AltibaseTypes;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

public class ColumnInfo implements Cloneable
{
    public static final byte IN_OUT_TARGET_TYPE_NONE = 0;
    public static final byte IN_OUT_TARGET_TYPE_IN = 1;
    public static final byte IN_OUT_TARGET_TYPE_INOUT = 2;
    public static final byte IN_OUT_TARGET_TYPE_OUT = 4;
    public static final byte IN_OUT_TARGET_TYPE_TARGET = 5; // select target 절에 사용되는 컬럼

    public static final boolean NON_NULLABLE = false;
    public static final boolean NULLABLE = true;

    private int mDataType;
    private boolean mNullable = true;
    private boolean mUpdatable = false;
    private int mPrecision = 0;
    private int mScale = 0;
    private int mLanguage = 0;
    private byte mArguments = 0;
    private byte mInOutTargetType = IN_OUT_TARGET_TYPE_NONE;
    private String mCatalogName = null;
    private String mTableName = null;
    private String mBaseTableName = null;
    private String mColumnName = null;
    private String mDisplayName = null;
    private String mBaseColumnName = null;
    private String mSchemaName = null;
    private int mCharPrecisionRate = 0;
    /* BUG-45572 columnInfo가 새로 생성되었으므로 setParamInfo를 전송해야 하므로 mChanged를 true로 설정한다. */
    private boolean mChanged = true;
    // BUG-40081 type을 바꿔야 하는지를 나타내는 flag 
    private boolean mShouldChangeType = false;

    public ColumnInfo()
    {
    }
    
	public void setColumnInfo(int aDataType,
                              int aLang,
                              byte aArgs,
                              int aPrecision,
                              int aScale,
                              byte aInOutTargetType,
                              boolean aNullable,
                              boolean aUpdatable,
                              String aCatalogName,
                              String aTableName,
                              String aBaseTableName,
                              String aColName,
                              String aDisplayName,
                              String aBaseColumnName,
                              String aSchemaName,
                              int aBytesPerChar)
    {
        mDataType = aDataType;
        mLanguage = aLang;
        mArguments = aArgs;
        mPrecision = aPrecision;
        mScale = aScale;
        mInOutTargetType = aInOutTargetType;
        mNullable = aNullable;
        mUpdatable = aUpdatable;
        mCatalogName = aCatalogName;
        mTableName = aTableName;
        mBaseTableName = aBaseTableName;
        mColumnName = aColName;
        mDisplayName = aDisplayName;
        mBaseColumnName = aBaseColumnName;
        mSchemaName = aSchemaName;
        mCharPrecisionRate = aBytesPerChar;
        mChanged = true;
    }

    int getCharPrecisionRate()
    {
        return mCharPrecisionRate;
    }

    public void modifyColumnInfo(int aDataType, byte aArgs, int aPrecision, int aScale)
    {
        mDataType = aDataType;
        mArguments = aArgs;
        mScale = aScale;
        mPrecision = aPrecision;
        mChanged = true;
    }

    public void unchange()
    {
        mChanged = false;
    }
    
    public void modifyDataType(int aDataType)
    {
        mDataType = aDataType;
        mChanged = true;
    }
    
    public void modifyScale(int aNewScale)
    {
        mScale = aNewScale;
        mChanged = true;
    }

    public void modifyPrecision(int aPrecision)
    {
        mPrecision = aPrecision;
        mChanged = true;
    }

    public boolean isChanged()
    {
        return mChanged;
    }
    
    public int getDataType()
    {
        return mDataType;
    }
    
    public int getLanguage()
    {
        return mLanguage;
    }

    public byte getArguments()
    {
        return mArguments;
    }

    public int getPrecision()
    {
        return mPrecision;
    }

    public int getScale()
    {
        return mScale;
    }

    public byte getInOutTargetType()
    {
        return mInOutTargetType;
    }

    public boolean getNullable()
    {
        return mNullable;
    }

    public boolean getUpdatable()
    {
        return mUpdatable;
    }
    
    public String getCatalogName()
    {
        return mCatalogName;
    }
    
    public String getTableName()
    {
        return mTableName;
    }
    
    public String getBaseTableName()
    {
        return mBaseTableName;
    }

    public String getColumnName()
    {
        return mColumnName;
    }
    
    public String getDisplayColumnName()
    {
        return mDisplayName;
    }
    
    public String getBaseColumnName()
    {
        return mBaseColumnName;
    }
    
    public String getOwnerName()
    {
        return mSchemaName;
    }
    
    public boolean hasInType()
    {
        return (mInOutTargetType == IN_OUT_TARGET_TYPE_IN ||
                mInOutTargetType == IN_OUT_TARGET_TYPE_INOUT);
    }

    public boolean hasOutType()
    {
        return (mInOutTargetType == IN_OUT_TARGET_TYPE_OUT ||
                mInOutTargetType == IN_OUT_TARGET_TYPE_INOUT);
    }
    
    public void addInType()
    {
        if (mDataType == ColumnTypes.BLOB_LOCATOR ||
            mDataType == ColumnTypes.CLOB_LOCATOR)
        {
            mInOutTargetType = IN_OUT_TARGET_TYPE_OUT;
        }
        else
        {
            if (mInOutTargetType == IN_OUT_TARGET_TYPE_NONE)
            {
                mInOutTargetType = IN_OUT_TARGET_TYPE_IN;
            }
            else if (mInOutTargetType == IN_OUT_TARGET_TYPE_OUT)
            {
                mInOutTargetType = IN_OUT_TARGET_TYPE_INOUT;
            }
        }
    }
    
    public void addOutType()
    {
        if (mInOutTargetType == IN_OUT_TARGET_TYPE_NONE)
        {
            mInOutTargetType = IN_OUT_TARGET_TYPE_OUT;
        }
        else if (mInOutTargetType == IN_OUT_TARGET_TYPE_IN)
        {
            mInOutTargetType = IN_OUT_TARGET_TYPE_INOUT;
        }
    }

    public void modifyInOutType(byte aInOutType)
    {
        mInOutTargetType = aInOutType;
    }

    public boolean shouldChangeType()
    {
        return mShouldChangeType;
    }

    public void setShouldChangeType(boolean aShouldChangeType)
    {
        this.mShouldChangeType = aShouldChangeType;
    }

    public Object clone()
    {
        try
        {
            return super.clone();
        }
        catch (CloneNotSupportedException sEx)
        {
            Error.throwInternalError(ErrorDef.INTERNAL_ASSERTION, sEx);
            return null;
        }
    }

    // BUG-42424 deferred상태일때 ColumnInfo의 기본값을 생성한다.
    public void makeDefaultValues()
    {
        this.setColumnInfo(AltibaseTypes.NULL,
                           0,                                      // language
                           (byte)0,                                // arguments
                           0,                                      // precision
                           0,                                      // scale
                           ColumnInfo.IN_OUT_TARGET_TYPE_NONE,     // in-out type
                           ColumnInfo.NULLABLE,                    // nullable
                           false,                                  // updatable
                           null,                                   // catalog name
                           null,                                   // table name
                           null,                                   // base table name
                           null,                                   // col name
                           null ,                                  // display name
                           null,                                   // base column name
                           null,                                   // schema name
                           AltibaseStatement.BYTES_PER_CHAR);      // bytes per char
    }

    // BUG-42424 dataType에 맞게 argument, scale 등의 값을 보정한다.
    public void setColumnMetaInfos(int aDataType, int aPrecision)
    {
        if (mDataType != aDataType || mPrecision != aPrecision)
        {
            mArguments = 0;
            mPrecision = aPrecision;
            mScale = 0;

            switch (aDataType)
            {
                case Types.CLOB:
                case Types.CHAR:
                case Types.VARCHAR:
                case AltibaseTypes.NCHAR:
                case AltibaseTypes.NVARCHAR:
                case AltibaseTypes.BYTE:
                case AltibaseTypes.NIBBLE:
                case Types.BLOB:
                case Types.BINARY:
                    mArguments = 1;
                    break;
                case Types.FLOAT:
                    // Precision만을 갖는 부동소수점 Data Type
                    mArguments = 1;
                    break;
                case AltibaseTypes.NUMBER:
                case Types.NUMERIC:
                    mScale = 20;
                    mArguments = 2;
                    break;
                case Types.BIT:
                case AltibaseTypes.VARBIT:
                    mArguments = 1;
                    break;
            }

            mDataType = aDataType;
        }
    }
}

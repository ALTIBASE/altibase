/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <idl.h>
#include <mtdTypes.h>
#include <mtd.h>
#include <mmErrorCode.h>
#include <mmcDef.h>
#include <mmcConv.h>
#include <mmcConvFmMT.h>
#include <mmcConvNumeric.h>
#include <mmcSession.h>
#include <mmtServiceThread.h>
#include <mtuProperty.h>

/*  fix BUG-31408,[mm-protocols] It need to describe mmERR_ABORT_INVALID_DATA_SIZE error message in detail.*/
#define  MMC_MT_TYPE_NAME_COUNT (32)
static mmcMtTypeName gMtTypeName[MMC_MT_TYPE_NAME_COUNT] =
{
    {MTD_INTEGER_ID,"MTD_INTEGER" },
    {MTD_NUMBER_ID,"MTD_NUMBER" },
    {MTD_NUMERIC_ID,"MTD_NUMERIC" },
    {MTD_DOUBLE_ID,"MTD_DOUBLE" },
    {MTD_FLOAT_ID,"MTD_FLOAT" },
    {MTD_REAL_ID,"MTD_REAL" },
    {MTD_BIGINT_ID,"MTD_BIGINT" },
    {MTD_SMALLINT_ID,"MTD_SMALLINT"},
    {MTD_VARCHAR_ID,"MTD_VARCHAR" },
    {MTD_CHAR_ID,"MTD_CHAR" },
    {MTD_DATE_ID,"MTD_DATE" },
    {MTD_BINARY_ID,"MTD_BINARY" },
    {MTD_BOOLEAN_ID,"MTD_BOOLEAN" },    
    {MTD_BLOB_ID,"MTD_BLOB" },
    {MTD_BLOB_LOCATOR_ID,"MTD_BLOB_LOCATOR"},
    {MTD_CLOB_ID,"MTD_CLOB" },
    {MTD_CLOB_LOCATOR_ID,"MTD_CLOB_LOCATOR"},
    {MTD_NCHAR_ID,"MTD_NCHAR" },
    {MTD_NVARCHAR_ID,"MTD_NVARCHAR"},
    {MTD_BIT_ID,"MTD_BIT"},
    {MTD_VARBIT_ID,"MTD_VARBIT"},
    {MTD_BYTE_ID,"MTD_BYTE" },
    {MTD_NIBBLE_ID,"MTD_NIBBLE" },
    {MTD_INTERVAL_ID,"MTD_INTERVAL" },
    {MTD_LIST_ID,"MTD_LIST" },
    {MTD_NULL_ID,"MTD_NULL" },
    {MTS_FILETYPE_ID,"MTS_FILETYPE_ID"},
    {MTD_GEOMETRY_ID,"MTD_GEOMETRY_ID"},
    {MTD_ECHAR_ID,"MTD_ECHAR"},
    {MTD_EVARCHAR_ID,"MTD_EVARCHAR" },
    {MTD_NONE_ID,"MTD_NONE" },
    {MTD_VARBYTE_ID,"MTD_VARBYTE" }
};
/*  fix BUG-31408,[mm-protocols] It need to describe mmERR_ABORT_INVALID_DATA_SIZE error message in detail.*/
const SChar * getMtTypeName( const UInt aMtTypeName)
{
    UInt i;
    
    for(i = 0; i < MMC_MT_TYPE_NAME_COUNT; i++)
    {
        if(gMtTypeName[i].mMtType == aMtTypeName)
        {
            break;
        }//
    }//for
    if( i == MMC_MT_TYPE_NAME_COUNT)
    {
        return gMtTypeName[MMC_MT_TYPE_NAME_COUNT -1].mTypeName;
    }
    else
    {
        return gMtTypeName[i].mTypeName;
    }
}

static IDE_RC convertCmNone(qciBindParam * /*aTargetType*/,
                            void         * /*aTarget*/,
                            UInt           /*aTargetSize*/,
                            cmtAny       * /*aSource*/,
                            void         * /*aTemplate*/,
                            mmcSession   * /*aSession*/)

{
    IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));

    return IDE_FAILURE;
}

static IDE_RC convertCmNull(qciBindParam *aTargetType,
                            void         *aTarget,
                            UInt          aTargetSize,
                            cmtAny       * /*aSource*/,
                            void         * /*aTemplate*/,
                            mmcSession   * /*aSession*/)
{
    void *sTarget = aTarget;
    void *sValue;
    UInt  sSize;

    IDE_TEST(mtd::assignNullValueById( aTargetType->type, 
                                       &sValue, 
                                       &sSize ) 
                                     != IDE_SUCCESS);

    IDE_TEST_RAISE( sSize > aTargetSize, InvalidSize );
                    
    idlOS::memcpy(sTarget, sValue, sSize);

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE,aTargetType->id,getMtTypeName(aTargetType->type),sSize, aTargetSize));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmSInt8(qciBindParam *aTargetType,
                             void         *aTarget,
                             UInt          aTargetSize,
                             cmtAny       *aSource,
                             void         * /*aTemplate*/,
                             mmcSession   * /*aSession*/)
{
    void  * sTarget = aTarget;
    UChar  sValue;

    IDE_TEST_RAISE(aTargetType->type != MTD_SMALLINT_ID, InvalidDataType);

    IDE_TEST_RAISE( ID_SIZEOF(mtdSmallintType) > aTargetSize, InvalidSize );

    IDE_TEST(cmtAnyReadUChar(aSource, &sValue) != IDE_SUCCESS);

    *(mtdSmallintType *)sTarget = sValue;

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE,aTargetType->id,getMtTypeName(aTargetType->type), ID_SIZEOF(mtdSmallintType), aTargetSize));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmUInt8(qciBindParam *aTargetType,
                             void         *aTarget,
                             UInt          aTargetSize,
                             cmtAny       *aSource,
                             void         * /*aTemplate*/,
                             mmcSession   * /*aSession*/)
{
    void  * sTarget = aTarget;
    UChar   sValue;
    UInt    sStructSize;

    IDE_TEST(cmtAnyReadUChar(aSource, &sValue) != IDE_SUCCESS);

    switch (aTargetType->type)
    {
        case MTD_BOOLEAN_ID:
            sStructSize = ID_SIZEOF(mtdBooleanType);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );
            
            *(mtdBooleanType *)sTarget = sValue ? MTD_BOOLEAN_TRUE : MTD_BOOLEAN_FALSE;
            break;

        case MTD_SMALLINT_ID:
            sStructSize = ID_SIZEOF(mtdSmallintType);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );
            
            *(mtdSmallintType *)sTarget = sValue;
            break;

        default:
            IDE_RAISE(InvalidDataType);
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), sStructSize, aTargetSize));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmSInt16(qciBindParam *aTargetType,
                              void         *aTarget,
                              UInt          aTargetSize,
                              cmtAny       *aSource,
                              void         * /*aTemplate*/,
                              mmcSession   * /*aSession*/)
{
    void  * sTarget = aTarget;
   
    IDE_TEST_RAISE(aTargetType->type != MTD_SMALLINT_ID, InvalidDataType);
   
    IDE_TEST_RAISE( ID_SIZEOF(mtdSmallintType) > aTargetSize, InvalidSize );
    
    IDE_TEST(cmtAnyReadSShort(aSource, (mtdSmallintType *)sTarget) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), ID_SIZEOF(mtdSmallintType), aTargetSize));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmUInt16(qciBindParam *aTargetType,
                              void         *aTarget,
                              UInt          aTargetSize,
                              cmtAny       *aSource,
                              void         * /*aTemplate*/,
                              mmcSession   * /*aSession*/)
{
    void  * sTarget = aTarget;
    UShort  sValue;

    switch (aTargetType->type)
    {
        case MTD_SMALLINT_ID:
            IDE_TEST_RAISE( ID_SIZEOF(mtdSmallintType) > aTargetSize, InvalidSize );
            IDE_TEST(cmtAnyReadUShort(aSource, &sValue) != IDE_SUCCESS);
            IDE_TEST_RAISE(sValue > MTD_SMALLINT_MAXIMUM, DataOverflow);
            *(mtdSmallintType *)sTarget = sValue;
            break;
        case MTD_INTEGER_ID:
            IDE_TEST_RAISE( ID_SIZEOF(mtdIntegerType) > aTargetSize, InvalidSize );
            IDE_TEST(cmtAnyReadUShort(aSource, &sValue) != IDE_SUCCESS);
            *(mtdIntegerType *)sTarget = sValue;
            break;
        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), ID_SIZEOF(mtdSmallintType), aTargetSize));
    }
    IDE_EXCEPTION(DataOverflow);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_DATA_CONVERSION_OVERFLOW));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmSInt32(qciBindParam *aTargetType,
                              void         *aTarget,
                              UInt          aTargetSize,
                              cmtAny       *aSource,
                              void         * /*aTemplate*/,
                              mmcSession   * /*aSession*/)
{
    void *sTarget = aTarget;
    SInt  sValue;

    switch (aTargetType->type)
    {
        case MTD_SMALLINT_ID:
            IDE_TEST_RAISE( ID_SIZEOF(mtdSmallintType) > aTargetSize, InvalidSize );
            IDE_TEST(cmtAnyReadSInt(aSource, &sValue) != IDE_SUCCESS);
            IDE_TEST_RAISE(sValue > MTD_SMALLINT_MAXIMUM, DataOverflow);
            *(mtdSmallintType *)sTarget = sValue;
            break;
        case MTD_INTEGER_ID:
            IDE_TEST_RAISE( ID_SIZEOF(mtdIntegerType) > aTargetSize, InvalidSize );
            IDE_TEST(cmtAnyReadSInt(aSource, &sValue) != IDE_SUCCESS);
            IDE_TEST_RAISE(sValue > MTD_INTEGER_MAXIMUM, DataOverflow);
            *(mtdIntegerType *)sTarget = sValue;
            break;
        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), ID_SIZEOF(mtdIntegerType), aTargetSize));
    }
    IDE_EXCEPTION(DataOverflow);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_DATA_CONVERSION_OVERFLOW));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmUInt32(qciBindParam *aTargetType,
                              void         *aTarget,
                              UInt          aTargetSize,
                              cmtAny       *aSource,
                              void         * /*aTemplate*/,
                              mmcSession   * /*aSession*/)
{
    void  * sTarget = aTarget;
    UInt    sValue;

    switch (aTargetType->type)
    {
        case MTD_SMALLINT_ID:
            IDE_TEST_RAISE( ID_SIZEOF(mtdSmallintType) > aTargetSize, InvalidSize );
            IDE_TEST(cmtAnyReadUInt(aSource, &sValue) != IDE_SUCCESS);
            IDE_TEST_RAISE(sValue > MTD_SMALLINT_MAXIMUM, DataOverflow);
            *(mtdSmallintType *)sTarget = sValue;
            break;
        case MTD_INTEGER_ID:
            IDE_TEST_RAISE( ID_SIZEOF(mtdIntegerType) > aTargetSize, InvalidSize );
            IDE_TEST(cmtAnyReadUInt(aSource, &sValue) != IDE_SUCCESS);
            IDE_TEST_RAISE(sValue > MTD_INTEGER_MAXIMUM, DataOverflow);
            *(mtdIntegerType *)sTarget = sValue;
            break;
        case MTD_BIGINT_ID:
            IDE_TEST_RAISE( ID_SIZEOF(mtdBigintType) > aTargetSize, InvalidSize );
            IDE_TEST(cmtAnyReadUInt(aSource, &sValue) != IDE_SUCCESS);
            *(mtdBigintType *)sTarget = sValue;
            break;
        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), ID_SIZEOF(mtdIntegerType), aTargetSize));
    }
    IDE_EXCEPTION(DataOverflow);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_DATA_CONVERSION_OVERFLOW));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmSInt64(qciBindParam *aTargetType,
                              void         *aTarget,
                              UInt          aTargetSize,
                              cmtAny       *aSource,
                              void         * /*aTemplate*/,
                              mmcSession   * /*aSession*/)
{
    void  * sTarget = aTarget;
    SLong   sValue;

    switch (aTargetType->type)
    {
        case MTD_SMALLINT_ID:
            IDE_TEST_RAISE( ID_SIZEOF(mtdSmallintType) > aTargetSize, InvalidSize );
            IDE_TEST(cmtAnyReadSLong(aSource, &sValue) != IDE_SUCCESS);
            IDE_TEST_RAISE(sValue > MTD_SMALLINT_MAXIMUM, DataOverflow);
            *(mtdSmallintType *)sTarget = sValue;
            break;
        case MTD_INTEGER_ID:
            IDE_TEST_RAISE( ID_SIZEOF(mtdIntegerType) > aTargetSize, InvalidSize );
            IDE_TEST(cmtAnyReadSLong(aSource, &sValue) != IDE_SUCCESS);
            IDE_TEST_RAISE(sValue > MTD_INTEGER_MAXIMUM, DataOverflow);
            *(mtdIntegerType *)sTarget = sValue;
            break;
        case MTD_BIGINT_ID:
            IDE_TEST_RAISE( ID_SIZEOF(mtdBigintType) > aTargetSize, InvalidSize );
            IDE_TEST(cmtAnyReadSLong(aSource, &sValue) != IDE_SUCCESS);
            IDE_TEST_RAISE(sValue > MTD_BIGINT_MAXIMUM, DataOverflow);
            *(mtdBigintType *)sTarget = sValue;
            break;
        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), ID_SIZEOF(mtdBigintType), aTargetSize));
    }
    IDE_EXCEPTION(DataOverflow);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_DATA_CONVERSION_OVERFLOW));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmUInt64(qciBindParam *aTargetType,
                              void         *aTarget,
                              UInt          aTargetSize,
                              cmtAny       *aSource,
                              void         * /*aTemplate*/,
                              mmcSession   * /*aSession*/)
{
    void  * sTarget = aTarget;
    ULong   sValue;
    UInt    sStructSize = 0;

    switch (aTargetType->type)
    {
        case MTD_BLOB_LOCATOR_ID:
            sStructSize = ID_SIZEOF(mtdBlobLocatorType);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );
            IDE_TEST(cmtAnyReadULong(aSource, (mtdBlobLocatorType *)sTarget) != IDE_SUCCESS);
            break;
        case MTD_CLOB_LOCATOR_ID:
            sStructSize = ID_SIZEOF(mtdClobLocatorType);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );
            IDE_TEST(cmtAnyReadULong(aSource, (mtdClobLocatorType *)sTarget) != IDE_SUCCESS);
            break;
        case MTD_SMALLINT_ID:
            IDE_TEST_RAISE( ID_SIZEOF(mtdSmallintType) > aTargetSize, InvalidSize );
            IDE_TEST(cmtAnyReadULong(aSource, &sValue) != IDE_SUCCESS);
            IDE_TEST_RAISE(sValue > MTD_SMALLINT_MAXIMUM, DataOverflow);
            *(mtdSmallintType *)sTarget = sValue;
            break;
        case MTD_INTEGER_ID:
            IDE_TEST_RAISE( ID_SIZEOF(mtdIntegerType) > aTargetSize, InvalidSize );
            IDE_TEST(cmtAnyReadULong(aSource, &sValue) != IDE_SUCCESS);
            IDE_TEST_RAISE(sValue > MTD_INTEGER_MAXIMUM, DataOverflow);
            *(mtdIntegerType *)sTarget = sValue;
            break;
        case MTD_BIGINT_ID:
            IDE_TEST_RAISE( ID_SIZEOF(mtdBigintType) > aTargetSize, InvalidSize );
            IDE_TEST(cmtAnyReadULong(aSource, &sValue) != IDE_SUCCESS);
            IDE_TEST_RAISE(sValue > MTD_BIGINT_MAXIMUM, DataOverflow);
            *(mtdBigintType *)sTarget = sValue;
            break;
        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), sStructSize, aTargetSize));
    }
    IDE_EXCEPTION(DataOverflow);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_DATA_CONVERSION_OVERFLOW));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmFloat32(qciBindParam *aTargetType,
                               void         *aTarget,
                               UInt          aTargetSize,
                               cmtAny       *aSource,
                               void         * /*aTemplate*/,
                               mmcSession   * /*aSession*/)
{
    void  * sTarget = aTarget;
    
    IDE_TEST_RAISE(aTargetType->type != MTD_REAL_ID, InvalidDataType);

    IDE_TEST_RAISE( ID_SIZEOF(mtdRealType) > aTargetSize, InvalidSize );
    
    IDE_TEST(cmtAnyReadSFloat(aSource, (mtdRealType *)sTarget) != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), ID_SIZEOF(mtdRealType), aTargetSize));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmFloat64(qciBindParam *aTargetType,
                               void         *aTarget,
                               UInt          aTargetSize,
                               cmtAny       *aSource,
                               void         * /*aTemplate*/,
                               mmcSession   * /*aSession*/)
{
    void  * sTarget = aTarget;

    IDE_TEST_RAISE(aTargetType->type != MTD_DOUBLE_ID, InvalidDataType);

    IDE_TEST_RAISE( ID_SIZEOF(mtdDoubleType) > aTargetSize, InvalidSize );
    
    IDE_TEST(cmtAnyReadSDouble(aSource, (mtdDoubleType *) sTarget) != IDE_SUCCESS);
   
    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), ID_SIZEOF(mtdDoubleType), aTargetSize));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmDateTime(qciBindParam *aTargetType,
                                void         *aTarget,
                                UInt          aTargetSize,
                                cmtAny       *aSource,
                                void         * /*aTemplate*/,
                                mmcSession   * /*aSession*/)
{
    void  * sTarget = aTarget;

    cmtDateTime *sDateTime;

    IDE_TEST_RAISE(aTargetType->type != MTD_DATE_ID, InvalidDataType);

    IDE_TEST_RAISE( ID_SIZEOF(mtdDateType) > aTargetSize, InvalidSize );
    
    IDE_TEST(cmtAnyReadDateTime(aSource, &sDateTime) != IDE_SUCCESS);

    IDE_TEST(mtdDateInterface::makeDate((mtdDateType *)sTarget,
                                        sDateTime->mYear,
                                        sDateTime->mMonth,
                                        sDateTime->mDay,
                                        sDateTime->mHour,
                                        sDateTime->mMinute,
                                        sDateTime->mSecond,
                                        sDateTime->mMicroSecond) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), ID_SIZEOF(mtdDateType), aTargetSize));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmInterval(qciBindParam *aTargetType,
                                void         *aTarget,
                                UInt          aTargetSize,
                                cmtAny       *aSource,
                                void         * /*aTemplate*/,
                                mmcSession   * /*aSession*/)
{
    mtdIntervalType *sTarget = (mtdIntervalType *) aTarget;
    cmtInterval     *sInterval;

    IDE_TEST_RAISE(aTargetType->type != MTD_INTERVAL_ID, InvalidDataType);

    IDE_TEST_RAISE( ID_SIZEOF(mtdIntervalType) > aTargetSize, InvalidSize );
    
    IDE_TEST(cmtAnyReadInterval(aSource, &sInterval) != IDE_SUCCESS);

    sTarget->second      = sInterval->mSecond;
    sTarget->microsecond = sInterval->mMicroSecond;

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), ID_SIZEOF(mtdIntervalType), aTargetSize));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmNumeric(qciBindParam *aTargetType,
                               void         *aTarget,
                               UInt          aTargetSize,
                               cmtAny       *aSource,
                               void         * /*aTemplate*/,
                               mmcSession   * aSession)
{
    UChar           sConvBuffer[MMC_CONV_NUMERIC_BUFFER_SIZE];
    UChar          *sConvMantissa;
    mmcConvNumeric  sSrc;
    mmcConvNumeric  sDst;
    cmtNumeric     *sCmNumeric;
    mtdNumericType *sMtNumeric = (mtdNumericType *) aTarget;
    idBool          sIsZero    = ID_TRUE;
    SChar           sSign;
    SInt            sScale     = 0;
    SInt            sPrecision = 0;
    SInt            sSize;
    SInt            i;
    SInt            sSignExponent = 0;
    UInt            sStructSize;

    IDE_TEST_RAISE((aTargetType->type != MTD_FLOAT_ID) &&
                   (aTargetType->type != MTD_NUMBER_ID) &&
                   (aTargetType->type != MTD_NUMERIC_ID),
                   InvalidDataType);

    IDE_TEST(cmtAnyReadNumeric(aSource, &sCmNumeric) != IDE_SUCCESS);

    for (i = 0; i < sCmNumeric->mSize; i++)
    {
        if (sCmNumeric->mData[i] > 0)
        {
            sIsZero = ID_FALSE;
            break;
        }
    }

    if (sIsZero != ID_TRUE)
    {
        sSign  = sCmNumeric->mSign;
        sScale = sCmNumeric->mScale;

        sSrc.initialize(sCmNumeric->mData,
                        sCmNumeric->mSize,
                        sCmNumeric->mSize,
                        256,
                        aSession->getNumericByteOrder());

        sDst.initialize(sConvBuffer,
                        MMC_CONV_NUMERIC_BUFFER_SIZE,
                        0,
                        100,
                        MMC_BYTEORDER_BIG_ENDIAN);

        IDE_TEST(sSrc.convert(&sDst) != IDE_SUCCESS);

        if ((sScale % 2) == 1)
        {
            IDE_TEST(sDst.shiftLeft() != IDE_SUCCESS);

            sScale++;
        }
        else if ((sScale % 2) == -1)
        {
            IDE_TEST(sDst.shiftLeft() != IDE_SUCCESS);
        }

        sConvMantissa = sDst.getBuffer();
        sSize         = sDst.getSize();

        for (i = sSize - 1; (i >= 0) && (sConvMantissa[i] == 0); i--)
        {
            sSize  -= 1;
            sScale -= 2;
        }

        sPrecision = sSize * 2;

        if (sConvMantissa[0] < 10)
        {
            sPrecision--;
        }

        if ((sConvMantissa[sSize - 1] % 10) == 0)
        {
            sPrecision--;
        }

        if (sSign != 1)
        {
            for (i = 0; i < sSize; i++)
            {
                sConvMantissa[i] = 99 - sConvMantissa[i];
            }
        }

        IDE_TEST_RAISE((sPrecision < MTD_NUMERIC_PRECISION_MINIMUM) ||
                       (sPrecision > MTD_NUMERIC_PRECISION_MAXIMUM),
                       PrecisionOverflow);
        // BUG-32571 /TC/Interface/jdbc/common/unitest.sql Diff
        if( aTargetType->type == MTD_FLOAT_ID )
        {
            // TargetType이 FLOAT인 경우 scale이 0이기 때문에 비교할 필요 없음.
        }
        else
        {
            IDE_TEST_RAISE((sScale < MTD_NUMERIC_SCALE_MINIMUM) ||
                           (sScale > MTD_NUMERIC_SCALE_MAXIMUM),
                           ScaleOverflow);
            IDE_TEST_RAISE( sScale > aTargetType->scale, InvalidScaleSize );
        }
        sStructSize = MTD_NUMERIC_SIZE(sPrecision, 0);
        IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

        // BUG-39971 signexponent overflow 처리
        sSignExponent = (sSize - sScale / 2) * (( sSign == 1) ? 1 : -1) + 64;
        IDE_TEST_RAISE( sSignExponent > MTD_NUMERIC_SIGN_EXPONENT_MAXIMUM, ScaleOverflow );

        sMtNumeric->length        = sSize + 1;
        sMtNumeric->signExponent  = (sSign == 1) ? 0x80 : 0;
        sMtNumeric->signExponent |= sSignExponent;

        idlOS::memcpy(sMtNumeric->mantissa, sConvMantissa, sSize);
    }
    else
    {
        sStructSize = MTD_NUMERIC_SIZE(0, 0);
        IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );
        
        sMtNumeric->length       = 1;
        sMtNumeric->signExponent = 128;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), sStructSize, aTargetSize));
    }
    IDE_EXCEPTION( InvalidScaleSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_SCALE_SIZE, sScale, aTargetType->scale));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION(PrecisionOverflow);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NUMERIC_PRECISION_OVERFLOW));
    }
    IDE_EXCEPTION(ScaleOverflow);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NUMERIC_SCALE_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* bug-31397: large sql_byte parameters make buffer overflow.
 * This is used to conv binary, byte, nibble, bit, varbit type.
 * Used in variableGetDataCallback(), convertCmVariable().
 * A new member (mOddSizeFlag) is added. */
typedef struct mmcConvContext
{
    SInt   mType;
    UInt   mOffset;
    UChar *mData;
    idBool mOddSizeFlag; /* for a fragmented packet with an odd size */
} mmcConvContext;

IDE_RC variableGetDataCallback( cmtVariable */*aVariable*/,
                                UInt         /*aOffset*/,
                                UInt         aSize,
                                UChar       *aData,
                                void        *aContext )
{
    mmcConvContext *sContext;

    sContext = (mmcConvContext*)aContext;

    switch( sContext->mType )
    {
        case MTD_BINARY_ID:
            IDE_TEST( mtc::makeBinary( sContext->mData + sContext->mOffset,
                                       aData,
                                       aSize,
                                       &sContext->mOffset,
                                       &sContext->mOddSizeFlag)
                      != IDE_SUCCESS );
            break;

        case MTD_BYTE_ID:
        case MTD_VARBYTE_ID:
            IDE_TEST( mtc::makeByte( sContext->mData + sContext->mOffset,
                                     aData,
                                     aSize,
                                     &sContext->mOffset,
                                     &sContext->mOddSizeFlag)
                      != IDE_SUCCESS );
            break;

        case MTD_NIBBLE_ID:
            IDE_TEST( mtc::makeNibble( sContext->mData + (sContext->mOffset / 2),
                                       sContext->mOffset % 2,
                                       aData,
                                       aSize,
                                       &sContext->mOffset )
                      != IDE_SUCCESS );
            break;

        case MTD_BIT_ID:
        case MTD_VARBIT_ID:
            IDE_TEST( mtc::makeBit( sContext->mData + (sContext->mOffset / 8),
                                    sContext->mOffset % 8,
                                    aData,
                                    aSize,
                                    &sContext->mOffset )
                      != IDE_SUCCESS );
            break;
            
        default:
            IDE_DASSERT(0);
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmVariable(qciBindParam *aTargetType,
                                void         *aTarget,
                                UInt          aTargetSize,
                                cmtAny       *aSource,
                                void         *aTemplate,
                                mmcSession   * /*aSession*/)
{
    void           *sTarget = aTarget;
    cmtVariable    *sVariable;
    UInt            sSize;
    UInt            sStructSize;
    mtdNumericType *sNumeric;
    UChar           sNumericBuffer[MTD_NUMERIC_SIZE_MAXIMUM];
    UChar           sSource[MTC_MAXIMUM_CHAR_SIZE_FOR_NUMBER_GROUP + 1] = { 0, };
    idBool          sCanonized;
    mmcConvContext  sContext;
    // BUG-22609 AIX 최적화 오류 수정
    // switch 에 UInt 형으로 음수값이 2번이상올때 서버 죽음
    SInt    sType   = (SInt)aTargetType->type;

    IDE_TEST(cmtAnyReadVariable(aSource, &sVariable) != IDE_SUCCESS);

    sSize = cmtVariableGetSize(sVariable);

    switch (sType)
    {
        case MTD_BLOB_ID:
            sStructSize = sSize = MTD_BLOB_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );
            
            IDE_TEST(cmtVariableGetData(sVariable,
                                        ((mtdBlobType *)sTarget)->value,
                                        sSize) != IDE_SUCCESS);

            ((mtdBlobType *)aTarget)->length = sSize;

            break;

        case MTD_CLOB_ID:
            sStructSize = sSize = MTD_CLOB_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            IDE_TEST(cmtVariableGetData(sVariable,
                                        ((mtdClobType *)sTarget)->value,
                                        sSize) != IDE_SUCCESS);

            ((mtdClobType *)aTarget)->length = sSize;

            break;

        case MTD_CHAR_ID:
        case MTD_VARCHAR_ID:
            IDE_TEST_RAISE(sSize > ID_USHORT_MAX, InvalidPrecision);
            sStructSize = MTD_CHAR_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            IDE_TEST(cmtVariableGetData(sVariable,
                                        ((mtdCharType *)sTarget)->value,
                                        sSize) != IDE_SUCCESS);

            ((mtdCharType *)aTarget)->length = sSize;

            break;

        case MTD_NCHAR_ID:
        case MTD_NVARCHAR_ID:
            IDE_TEST_RAISE(sSize > ID_USHORT_MAX, InvalidPrecision);
            sStructSize = MTD_CHAR_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            IDE_TEST(cmtVariableGetData(sVariable,
                                        ((mtdNcharType *)sTarget)->value,
                                        sSize) != IDE_SUCCESS);

            ((mtdNcharType *)aTarget)->length = sSize;

            break;

        case MTD_FLOAT_ID:
            IDE_TEST(cmtVariableGetData(sVariable,
                                        sSource,
                                        sSize) != IDE_SUCCESS);

            sNumeric = (mtdNumericType*)sNumericBuffer;
            IDE_TEST( mtc::makeNumeric( sNumeric,
                                        aTargetType->precision,
                                        sSource,
                                        sSize )
                      != IDE_SUCCESS );
            
            idlOS::memcpy( sTarget,
                           sNumeric,
                           ID_SIZEOF(UChar) + sNumeric->length );
            break;
            
        case MTD_NUMBER_ID:
        case MTD_NUMERIC_ID:
            IDE_TEST(cmtVariableGetData(sVariable,
                                        sSource,
                                        sSize) != IDE_SUCCESS);

            sNumeric = (mtdNumericType*)sNumericBuffer;
            IDE_TEST( mtc::makeNumeric( sNumeric,
                                        MTD_NUMERIC_MANTISSA_MAXIMUM,
                                        sSource,
                                        sSize )
                      != IDE_SUCCESS );

            IDE_TEST( mtc::numericCanonize( sNumeric,
                                            (mtdNumericType*)sTarget,
                                            aTargetType->precision,
                                            aTargetType->scale,
                                            &sCanonized )
                      != IDE_SUCCESS );

            if( sCanonized == ID_FALSE )
            {
                idlOS::memcpy( sTarget,
                               sNumeric,
                               ID_SIZEOF(UChar) + sNumeric->length );
            }
            
            break;

        case MTD_SMALLINT_ID:
            IDE_TEST(cmtVariableGetData(sVariable,
                                        sSource,
                                        sSize) != IDE_SUCCESS);

            sSource[sSize] = 0;
            IDE_TEST( mtc::makeSmallint( (mtdSmallintType*)sTarget,
                                         sSource,
                                         sSize )
                      != IDE_SUCCESS );
            break;
            
        case MTD_INTEGER_ID:
            IDE_TEST(cmtVariableGetData(sVariable,
                                        sSource,
                                        sSize) != IDE_SUCCESS);

            sSource[sSize] = 0;
            IDE_TEST( mtc::makeInteger( (mtdIntegerType*)sTarget,
                                        sSource,
                                        sSize )
                      != IDE_SUCCESS );
            break;
            
        case MTD_BIGINT_ID:
            IDE_TEST(cmtVariableGetData(sVariable,
                                        sSource,
                                        sSize) != IDE_SUCCESS);

            sSource[sSize] = 0;
            IDE_TEST( mtc::makeBigint( (mtdBigintType*)sTarget,
                                       sSource,
                                       sSize )
                      != IDE_SUCCESS );
            break;
            
        case MTD_REAL_ID:
            IDE_TEST(cmtVariableGetData(sVariable,
                                        sSource,
                                        sSize) != IDE_SUCCESS);

            sSource[sSize] = 0;
            IDE_TEST( mtc::makeReal( (mtdRealType*)sTarget,
                                     sSource,
                                     sSize )
                      != IDE_SUCCESS );
            break;
            
        case MTD_DOUBLE_ID:
            IDE_TEST(cmtVariableGetData(sVariable,
                                        sSource,
                                        sSize) != IDE_SUCCESS);
            
            sSource[sSize] = 0;
            IDE_TEST( mtc::makeDouble( (mtdDoubleType*)sTarget,
                                       sSource,
                                       sSize )
                      != IDE_SUCCESS );
            break;
            
        case MTD_DATE_ID:
            IDE_TEST(cmtVariableGetData(sVariable,
                                        sSource,
                                        sSize) != IDE_SUCCESS);
            
            sSource[sSize] = 0;
            IDE_TEST( mtdDateInterface::toDate(
                          (mtdDateType*)sTarget,
                          (UChar*)sSource,
                          (UShort)sSize,
                          (UChar*)(((mtcTemplate*)aTemplate)->dateFormat),
                          idlOS::strlen(((mtcTemplate*)aTemplate)->dateFormat) )
                      != IDE_SUCCESS );
            break;
                      
        case MTD_INTERVAL_ID:
            IDE_TEST(cmtVariableGetData(sVariable,
                                        sSource,
                                        sSize) != IDE_SUCCESS);
            
            sSource[sSize] = 0;
            IDE_TEST( mtc::makeInterval( (mtdIntervalType*)sTarget,
                                         sSource,
                                         sSize )
                      != IDE_SUCCESS );
            break;
            
        case MTD_BINARY_ID:
            IDE_TEST_RAISE(sSize > ID_UINT_MAX, InvalidPrecision);
            sStructSize = MTD_BINARY_TYPE_STRUCT_SIZE((sSize/2)+(sSize%2));
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            sContext.mData   = ((mtdBinaryType*)sTarget)->mValue;
            sContext.mOffset = 0;
            sContext.mType   = aTargetType->type;
            sContext.mOddSizeFlag   = ID_FALSE;
            
            IDE_TEST( cmtVariableGetDataWithCallback( sVariable,
                                                      variableGetDataCallback,
                                                      (void*)&sContext )
                      != IDE_SUCCESS );
            ((mtdBinaryType*)sTarget)->mLength = sContext.mOffset;
            
            break;

        case MTD_BYTE_ID:
        case MTD_VARBYTE_ID:
            IDE_TEST_RAISE(sSize > ID_USHORT_MAX, InvalidPrecision);
            sStructSize = MTD_BYTE_TYPE_STRUCT_SIZE((sSize/2)+(sSize%2));
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            sContext.mData   = ((mtdByteType*)sTarget)->value;
            sContext.mOffset = 0;
            sContext.mType   = aTargetType->type;
            sContext.mOddSizeFlag   = ID_FALSE;
            
            IDE_TEST( cmtVariableGetDataWithCallback( sVariable,
                                                      variableGetDataCallback,
                                                      (void*)&sContext )
                      != IDE_SUCCESS );
            ((mtdByteType*)sTarget)->length = sContext.mOffset;

            break;

        case MTD_NIBBLE_ID:
            IDE_TEST_RAISE(sSize > ID_USHORT_MAX, InvalidPrecision);
            sStructSize = MTD_NIBBLE_TYPE_STRUCT_SIZE((sSize/2)+(sSize%2));
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            sContext.mData   = ((mtdNibbleType*)sTarget)->value;
            sContext.mOffset = 0;
            sContext.mType   = aTargetType->type;
            
            IDE_TEST( cmtVariableGetDataWithCallback( sVariable,
                                                      variableGetDataCallback,
                                                      (void*)&sContext )
                      != IDE_SUCCESS );
            ((mtdNibbleType*)sTarget)->length = sContext.mOffset;
            
            break;

        case MTD_BIT_ID:
        case MTD_VARBIT_ID:
            IDE_TEST_RAISE(sSize > ID_USHORT_MAX, InvalidPrecision);
            sStructSize = MTD_BIT_TYPE_STRUCT_SIZE((sSize/8)+(sSize%8));
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            sContext.mData   = ((mtdBitType*)sTarget)->value;
            sContext.mOffset = 0;
            sContext.mType   = aTargetType->type;
            
            IDE_TEST( cmtVariableGetDataWithCallback( sVariable,
                                                      variableGetDataCallback,
                                                      (void*)&sContext )
                      != IDE_SUCCESS );
            ((mtdBitType*)sTarget)->length = sContext.mOffset;
            
            break;

        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), sStructSize, aTargetSize));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION(InvalidPrecision);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_PRECISION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmInVariable(qciBindParam *aTargetType,
                                  void         *aTarget,
                                  UInt          aTargetSize,
                                  cmtAny       *aSource,
                                  void         *aTemplate,
                                  mmcSession   * /*aSession*/)
{
    void           *sTarget = aTarget;
    cmtInVariable  *sInVariable;
    UInt            sSize;
    UInt            sStructSize;
    mtdNumericType *sNumeric;
    UChar           sNumericBuffer[MTD_NUMERIC_SIZE_MAXIMUM];
    idBool          sCanonized;

    /* PROJ-1920 */
    if( aSource->mType == CMT_ID_PTR )
    {
        IDE_TEST(cmtAnyReadPtr(aSource, &sInVariable) != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(cmtAnyReadInVariable(aSource, &sInVariable) != IDE_SUCCESS);
    }

    sSize = sInVariable->mSize;

    switch (aTargetType->type)
    {
        case MTD_BLOB_ID:
            sStructSize = sSize = MTD_BLOB_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );
            
            idlOS::memcpy( ((mtdBlobType *)sTarget)->value,
                           sInVariable->mData,
                           sSize );
                                     
            ((mtdBlobType *)aTarget)->length = sSize;

            break;

        case MTD_CLOB_ID:
            sStructSize = sSize = MTD_CLOB_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );
            
            idlOS::memcpy( ((mtdClobType *)sTarget)->value,
                           sInVariable->mData,
                           sSize );

            ((mtdClobType *)aTarget)->length = sSize;

            break;

        case MTD_CHAR_ID:
        case MTD_VARCHAR_ID:
            IDE_TEST_RAISE(sSize > ID_USHORT_MAX, InvalidPrecision);
            sStructSize = MTD_CHAR_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            idlOS::memcpy( ((mtdCharType *)sTarget)->value,
                           sInVariable->mData,
                           sSize );

            ((mtdCharType *)aTarget)->length = sSize;

            break;

        case MTD_NCHAR_ID:
        case MTD_NVARCHAR_ID:
            IDE_TEST_RAISE(sSize > ID_USHORT_MAX, InvalidPrecision);
            sStructSize = MTD_CHAR_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            idlOS::memcpy( ((mtdNcharType *)sTarget)->value,
                           sInVariable->mData,
                           sSize );

            ((mtdNcharType *)aTarget)->length = sSize;

            break;

        case MTD_FLOAT_ID:
            sNumeric = (mtdNumericType*)sNumericBuffer;
            IDE_TEST( mtc::makeNumeric( sNumeric,
                                        aTargetType->precision,
                                        sInVariable->mData,
                                        sSize )
                      != IDE_SUCCESS );
            
            idlOS::memcpy( sTarget,
                           sNumeric,
                           ID_SIZEOF(UChar) + sNumeric->length );
            break;
            
        case MTD_NUMBER_ID:
        case MTD_NUMERIC_ID:
            sNumeric = (mtdNumericType*)sNumericBuffer;
            IDE_TEST( mtc::makeNumeric( sNumeric,
                                        MTD_NUMERIC_MANTISSA_MAXIMUM,
                                        sInVariable->mData,
                                        sSize )
                      != IDE_SUCCESS );

            IDE_TEST( mtc::numericCanonize( sNumeric,
                                            (mtdNumericType*)sTarget,
                                            aTargetType->precision,
                                            aTargetType->scale,
                                            &sCanonized )
                      != IDE_SUCCESS );

            if( sCanonized == ID_FALSE )
            {
                idlOS::memcpy( sTarget,
                               sNumeric,
                               ID_SIZEOF(UChar) + sNumeric->length );
            }
            break;

        case MTD_SMALLINT_ID:
            IDE_TEST( mtc::makeSmallint( (mtdSmallintType*)sTarget,
                                         sInVariable->mData,
                                         sSize )
                      != IDE_SUCCESS );
            break;
            
        case MTD_INTEGER_ID:
            IDE_TEST( mtc::makeInteger( (mtdIntegerType*)sTarget,
                                        sInVariable->mData,
                                        sSize )
                      != IDE_SUCCESS );
            break;
            
        case MTD_BIGINT_ID:
            IDE_TEST( mtc::makeBigint( (mtdBigintType*)sTarget,
                                       sInVariable->mData,
                                       sSize )
                      != IDE_SUCCESS );
            break;
            
        case MTD_REAL_ID:
            IDE_TEST( mtc::makeReal( (mtdRealType*)sTarget,
                                     sInVariable->mData,
                                     sSize )
                      != IDE_SUCCESS );
            break;
            
        case MTD_DOUBLE_ID:

            IDE_TEST( mtc::makeDouble( (mtdDoubleType*)sTarget,
                                       sInVariable->mData,
                                       sSize )
                      != IDE_SUCCESS );
            break;
            
        case MTD_DATE_ID:
            IDE_TEST( mtdDateInterface::toDate(
                          (mtdDateType*)sTarget,
                          (UChar*)sInVariable->mData,
                          (UShort)sSize,
                          (UChar*)(((mtcTemplate*)aTemplate)->dateFormat),
                          idlOS::strlen(((mtcTemplate*)aTemplate)->dateFormat) )
                      != IDE_SUCCESS );
            break;
            
        case MTD_INTERVAL_ID:
            IDE_TEST( mtc::makeInterval( (mtdIntervalType*)sTarget,
                                         sInVariable->mData,
                                         sSize )
                      != IDE_SUCCESS );
            break;
            
        case MTD_BINARY_ID:
            IDE_TEST_RAISE(sSize > ID_UINT_MAX, InvalidPrecision);
            sStructSize = MTD_BINARY_TYPE_STRUCT_SIZE((sSize/2)+(sSize%2));
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            IDE_TEST( mtc::makeBinary( (mtdBinaryType *)sTarget,
                                       sInVariable->mData,
                                       sSize )
                      != IDE_SUCCESS );

            break;

        case MTD_BYTE_ID:
        case MTD_VARBYTE_ID:
            IDE_TEST_RAISE(sSize > ID_USHORT_MAX, InvalidPrecision);
            sStructSize = MTD_BYTE_TYPE_STRUCT_SIZE((sSize/2)+(sSize%2));
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            IDE_TEST( mtc::makeByte( (mtdByteType *)sTarget,
                                     sInVariable->mData,
                                     sSize )
                      != IDE_SUCCESS );

            break;

        case MTD_NIBBLE_ID:
            IDE_TEST_RAISE(sSize > ID_USHORT_MAX, InvalidPrecision);
            sStructSize = MTD_NIBBLE_TYPE_STRUCT_SIZE((sSize/2)+(sSize%2));
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            IDE_TEST( mtc::makeNibble( (mtdNibbleType *)sTarget,
                                       sInVariable->mData,
                                       sSize )
                      != IDE_SUCCESS );

            break;

        case MTD_BIT_ID:
        case MTD_VARBIT_ID:
            IDE_TEST_RAISE(sSize > ID_USHORT_MAX, InvalidPrecision);
            sStructSize = MTD_BIT_TYPE_STRUCT_SIZE((sSize/8)+(sSize%8));
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            IDE_TEST( mtc::makeBit( (mtdBitType *)sTarget,
                                    sInVariable->mData,
                                    sSize )
                      != IDE_SUCCESS );

            break;

        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id, getMtTypeName(aTargetType->type),sStructSize, aTargetSize));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION(InvalidPrecision);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_PRECISION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmBinary(qciBindParam *aTargetType,
                              void         *aTarget,
                              UInt          aTargetSize,
                              cmtAny       *aSource,
                              void         * /*aTemplate*/,
                              mmcSession   * /*aSession*/)
{
    void           *sTarget = aTarget;
    cmtVariable    *sVariable;
    UInt            sSize;
    UInt            sStructSize;
    
    IDE_TEST(cmtAnyReadBinary(aSource, &sVariable) != IDE_SUCCESS);

    sSize = cmtVariableGetSize(sVariable);

    if (aTargetType->arguments == 1)
    {
        IDE_TEST_RAISE(sSize > (UInt)aTargetType->precision, InvalidPrecision);
    }

    switch (aTargetType->type)
    {
        case MTD_BLOB_ID:
            sStructSize = sSize = MTD_BLOB_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );
            
            IDE_TEST(cmtVariableGetData(sVariable,
                                        ((mtdBlobType *)sTarget)->value,
                                        sSize) != IDE_SUCCESS);

            ((mtdBlobType *)aTarget)->length = sSize;

            break;

        case MTD_CLOB_ID:
            sStructSize = sSize = MTD_CLOB_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            IDE_TEST(cmtVariableGetData(sVariable,
                                        ((mtdClobType *)sTarget)->value,
                                        sSize) != IDE_SUCCESS);

            ((mtdClobType *)aTarget)->length = sSize;

            break;

        case MTD_CHAR_ID:
        case MTD_VARCHAR_ID:
            IDE_TEST_RAISE(sSize > ID_USHORT_MAX, InvalidPrecision);
            sStructSize = MTD_CHAR_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            IDE_TEST(cmtVariableGetData(sVariable,
                                        ((mtdCharType *)sTarget)->value,
                                        sSize) != IDE_SUCCESS);

            ((mtdCharType *)aTarget)->length = sSize;

            break;
            
        case MTD_BINARY_ID:
            IDE_TEST_RAISE(sSize > ID_UINT_MAX, InvalidPrecision);
            sStructSize = MTD_BINARY_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            IDE_TEST(cmtVariableGetData(sVariable,
                                        ((mtdBinaryType *)sTarget)->mValue,
                                        sSize) != IDE_SUCCESS);

            ((mtdBinaryType *)aTarget)->mLength = sSize;

            break;

        case MTD_BYTE_ID:
        case MTD_VARBYTE_ID:
            IDE_TEST_RAISE(sSize > ID_USHORT_MAX, InvalidPrecision);
            sStructSize = MTD_BYTE_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            IDE_TEST(cmtVariableGetData(sVariable,
                                        ((mtdByteType *)sTarget)->value,
                                        sSize) != IDE_SUCCESS);

            ((mtdByteType *)aTarget)->length = sSize;

            break;

        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), sStructSize, aTargetSize));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION(InvalidPrecision);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_PRECISION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmInBinary(qciBindParam *aTargetType,
                                void         *aTarget,
                                UInt          aTargetSize,
                                cmtAny       *aSource,
                                void         * /*aTemplate*/,
                                mmcSession   * /*aSession*/)
{
    void           *sTarget = aTarget;
    cmtInVariable  *sInVariable;
    UInt            sSize;
    UInt            sStructSize;

    IDE_TEST(cmtAnyReadInBinary(aSource, &sInVariable) != IDE_SUCCESS);

    sSize = sInVariable->mSize;

    if (aTargetType->arguments == 1)
    {
        IDE_TEST_RAISE(sSize > (UInt)aTargetType->precision, InvalidPrecision);
    }

    switch (aTargetType->type)
    {
        case MTD_BLOB_ID:
            sStructSize = sSize = MTD_BLOB_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );
            
            idlOS::memcpy( ((mtdBlobType *)sTarget)->value,
                           sInVariable->mData,
                           sSize );
                                     
            ((mtdBlobType *)aTarget)->length = sSize;

            break;

        case MTD_CLOB_ID:
            sStructSize = sSize = MTD_CLOB_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );
            
            idlOS::memcpy( ((mtdClobType *)sTarget)->value,
                           sInVariable->mData,
                           sSize );

            ((mtdClobType *)aTarget)->length = sSize;

            break;

        case MTD_CHAR_ID:
        case MTD_VARCHAR_ID:
            IDE_TEST_RAISE(sSize > ID_USHORT_MAX, InvalidPrecision);
            sStructSize = MTD_CHAR_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            idlOS::memcpy( ((mtdCharType *)sTarget)->value,
                           sInVariable->mData,
                           sSize );

            ((mtdCharType *)aTarget)->length = sSize;

            break;

        case MTD_BINARY_ID:
            IDE_TEST_RAISE(sSize > ID_UINT_MAX, InvalidPrecision);
            sStructSize = MTD_BINARY_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            idlOS::memcpy( ((mtdBinaryType *)sTarget)->mValue,
                           sInVariable->mData,
                           sSize );

            ((mtdBinaryType *)aTarget)->mLength = sSize;

            break;

        case MTD_BYTE_ID:
        case MTD_VARBYTE_ID:
            IDE_TEST_RAISE(sSize > ID_USHORT_MAX, InvalidPrecision);
            sStructSize = MTD_BYTE_TYPE_STRUCT_SIZE(sSize);
            IDE_TEST_RAISE( sStructSize > aTargetSize, InvalidSize );

            idlOS::memcpy( ((mtdByteType *)sTarget)->value,
                           sInVariable->mData,
                           sSize );

            ((mtdByteType *)aTarget)->length = sSize;

            break;

        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), sStructSize, aTargetSize));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION(InvalidPrecision);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_PRECISION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmBit(qciBindParam *aTargetType,
                           void         *aTarget,
                           UInt          aTargetSize,
                           cmtAny       *aSource,
                           void         * /*aTemplate*/,
                           mmcSession   * /*aSession*/)
{
    cmtBit     *sCmBit;
    mtdBitType *sMtBit = (mtdBitType *) aTarget;
    UInt        sSize;

    IDE_TEST_RAISE((aTargetType->type != MTD_BIT_ID) &&
                   (aTargetType->type != MTD_VARBIT_ID), InvalidDataType);

    IDE_TEST(cmtAnyReadBit(aSource, &sCmBit) != IDE_SUCCESS);

    sSize = cmtVariableGetSize(&sCmBit->mData);

    IDE_TEST_RAISE(sSize > (UInt)aTargetType->precision, InvalidPrecision);
    IDE_TEST_RAISE(BIT_TO_BYTE(sCmBit->mPrecision) != sSize, PrecisionMismatch);

    IDE_TEST_RAISE( MTD_BIT_TYPE_STRUCT_SIZE(sCmBit->mPrecision) > aTargetSize, InvalidSize );
    
    IDE_TEST(cmtVariableGetData(&sCmBit->mData, sMtBit->value, sSize) != IDE_SUCCESS);

    sMtBit->length             = sCmBit->mPrecision;

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), MTD_BIT_TYPE_STRUCT_SIZE(sCmBit->mPrecision), aTargetSize));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION(InvalidPrecision);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_PRECISION));
    }
    IDE_EXCEPTION(PrecisionMismatch);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_PRECISION_MISMATCH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmNibble(qciBindParam *aTargetType,
                              void         *aTarget,
                              UInt          aTargetSize,
                              cmtAny       *aSource,
                              void         * /*aTemplate*/,
                              mmcSession   * /*aSession*/)
{
    cmtNibble     *sCmNibble;
    mtdNibbleType *sMtNibble = (mtdNibbleType *) aTarget;
    UInt           sSize;

    IDE_TEST_RAISE(aTargetType->type != MTD_NIBBLE_ID, InvalidDataType);

    IDE_TEST(cmtAnyReadNibble(aSource, &sCmNibble) != IDE_SUCCESS);

    sSize = cmtVariableGetSize(&sCmNibble->mData);

    IDE_TEST_RAISE(sSize > (UInt)aTargetType->precision, InvalidPrecision);
    // BUG-23061 nibble 의 최대길이가 254입니다.
    IDE_TEST_RAISE(sCmNibble->mPrecision > MTD_NIBBLE_PRECISION_MAXIMUM, InvalidPrecision);
    IDE_TEST_RAISE(((sCmNibble->mPrecision + 1) / 2) != sSize, PrecisionMismatch);

    IDE_TEST_RAISE( MTD_NIBBLE_TYPE_STRUCT_SIZE(sCmNibble->mPrecision) > aTargetSize, InvalidSize );
    
    IDE_TEST(cmtVariableGetData(&sCmNibble->mData, sMtNibble->value, sSize) != IDE_SUCCESS);

    sMtNibble->length          = sCmNibble->mPrecision;

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), MTD_NIBBLE_TYPE_STRUCT_SIZE(sCmNibble->mPrecision), aTargetSize));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION(InvalidPrecision);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_PRECISION));
    }
    IDE_EXCEPTION(PrecisionMismatch);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_PRECISION_MISMATCH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmInBit(qciBindParam *aTargetType,
                             void         *aTarget,
                             UInt          aTargetSize,
                             cmtAny       *aSource,
                             void         * /*aTemplate*/,
                             mmcSession   * /*aSession*/)
{
    cmtInBit        *sCmInBit;
    cmtInVariable   *sCmInVariable;
    mtdBitType      *sMtBit = (mtdBitType *) aTarget;
    UInt             sSize;

    IDE_TEST_RAISE((aTargetType->type != MTD_BIT_ID) &&
                   (aTargetType->type != MTD_VARBIT_ID), InvalidDataType);

    IDE_TEST(cmtAnyReadInBit(aSource, &sCmInBit) != IDE_SUCCESS);

    sCmInVariable = &sCmInBit->mData;
    sSize = sCmInVariable->mSize;

    IDE_TEST_RAISE(sSize > (UInt)aTargetType->precision, InvalidPrecision);
    IDE_TEST_RAISE(BIT_TO_BYTE(sCmInBit->mPrecision) != sSize, PrecisionMismatch);

    IDE_TEST_RAISE( MTD_BIT_TYPE_STRUCT_SIZE(sCmInBit->mPrecision) > aTargetSize, InvalidSize );
    
    idlOS::memcpy( sMtBit->value,
                   sCmInVariable->mData,
                   sSize );

    sMtBit->length             = sCmInBit->mPrecision;

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type), MTD_BIT_TYPE_STRUCT_SIZE(sCmInBit->mPrecision), aTargetSize));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION(InvalidPrecision);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_PRECISION));
    }
    IDE_EXCEPTION(PrecisionMismatch);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_PRECISION_MISMATCH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC convertCmInNibble(qciBindParam *aTargetType,
                                void         *aTarget,
                                UInt          aTargetSize,
                                cmtAny       *aSource,
                                void         * /*aTemplate*/,
                                mmcSession   * /*aSession*/)
{
    cmtInNibble     *sCmInNibble;
    cmtInVariable   *sCmInVariable;
    mtdNibbleType   *sMtNibble = (mtdNibbleType *) aTarget;
    UInt             sSize;

    IDE_TEST_RAISE(aTargetType->type != MTD_NIBBLE_ID, InvalidDataType);

    IDE_TEST(cmtAnyReadInNibble(aSource, &sCmInNibble) != IDE_SUCCESS);

    sCmInVariable = &sCmInNibble->mData;
    sSize = sCmInVariable->mSize;

    IDE_TEST_RAISE(sSize > (UInt)aTargetType->precision, InvalidPrecision);

    // BUG-23061 nibble 의 최대길이가 254입니다.
    IDE_TEST_RAISE(sCmInNibble->mPrecision > MTD_NIBBLE_PRECISION_MAXIMUM, InvalidPrecision);
    IDE_TEST_RAISE(((sCmInNibble->mPrecision + 1) / 2) != sSize, PrecisionMismatch);

    IDE_TEST_RAISE( MTD_NIBBLE_TYPE_STRUCT_SIZE(sCmInNibble->mPrecision) > aTargetSize, InvalidSize );
    
    idlOS::memcpy( sMtNibble->value,
                   sCmInVariable->mData,
                   sSize );

    sMtNibble->length = sCmInNibble->mPrecision;

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidSize );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_SIZE, aTargetType->id,getMtTypeName(aTargetType->type),MTD_NIBBLE_TYPE_STRUCT_SIZE(sCmInNibble->mPrecision), aTargetSize));
    }
    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION(InvalidPrecision);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_PRECISION));
    }
    IDE_EXCEPTION(PrecisionMismatch);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_PRECISION_MISMATCH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// fix BUG-17249
IDE_RC mmcConv::convertFromMT(cmiProtocolContext *aProtocolContext,
                              cmtAny             *aTarget,
                              void               *aSource,
                              UInt                aSourceType,
                              mmcSession         *aSession)
{
    // bug-19174
    UInt sLobSize;

    sLobSize      = 0;

    switch(aSourceType)
    {
        case MTD_BLOB_LOCATOR_ID:
        case MTD_CLOB_LOCATOR_ID:
            // bug-26327: null lob locator err when no rows updated.
            // lob data로 parameter bind시 update row가 없는 경우
            // null lob locator가 넘어온다.
            // 이 때는 getLength 호출하면 err 발생하므로 호출 안함.
            // 그러면 client로 보내는 cmtAny.mType 으로 
            // CMT_ID_LOBLOCATOR가 아니라 CMT_ID_NULL이 송신됨.
            // 동시에 client의 ulnBindStoreLobLocator 에서
            // CMT_ID_NULL에 대한 처리를 해주도록 변경함.
            // LOB LOCATOR의 CURSOR가 OPEN 된 상황에서만 LENGTH를 갖고 온다.
            if ((*(smLobLocator *)aSource != MTD_LOCATOR_NULL) &&
                (smiLob::isOpen(*(smLobLocator *)aSource) == ID_TRUE))
            {
                IDE_TEST(qciMisc::lobGetLength( aSession->getStatSQL(),
                                                *(smLobLocator *)aSource,
                                                &sLobSize )
                         != IDE_SUCCESS);
            }
            break;
    }

    return mmcConvFromMT::convert(aProtocolContext,
                                  aTarget,
                                  aSource,
                                  aSourceType,
                                  aSession,
                                  sLobSize);

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * PROJ-1752 LIST 프로토콜 확대 적용
 */
IDE_RC mmcConv::buildAnyFromMT(cmtAny             *aTarget,
                               void               *aSource,
                               UInt                aSourceType,
                               mmcSession         *aSession)
{
    // bug-19174
    UInt sLobSize;

    sLobSize      = 0;

    switch(aSourceType)
    {
        case MTD_BLOB_LOCATOR_ID:
        case MTD_CLOB_LOCATOR_ID:
            // bug-26327: null lob locator err when no rows updated.
            // lob data로 parameter bind시 update row가 없는 경우
            // null lob locator가 넘어온다.
            // 이 때는 getLength 호출하면 err 발생하므로 호출 안함.
            // 그러면 client로 보내는 cmtAny.mType 으로 
            // CMT_ID_LOBLOCATOR가 아니라 CMT_ID_NULL이 송신됨.
            // 동시에 client의 ulnBindStoreLobLocator 에서
            // CMT_ID_NULL에 대한 처리를 해주도록 변경함.
            // LOB LOCATOR의 CURSOR가 OPEN 된 상황에서만 LENGTH를 갖고 온다.
            if ((*(smLobLocator *)aSource != MTD_LOCATOR_NULL) &&
                (smiLob::isOpen(*(smLobLocator *)aSource) == ID_TRUE))
            {
                IDE_TEST(qciMisc::lobGetLength( aSession->getStatSQL(),
                                                *(smLobLocator *)aSource,
                                                &sLobSize )
                         != IDE_SUCCESS);
            }
            break;
    }

    return mmcConvFromMT::buildAny(aTarget,
                                   aSource,
                                   aSourceType,
                                   aSession,
                                   sLobSize);

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

mmcConvFuncFromCM mmcConv::mConvFromCM[CMT_ID_MAX] =
{
    convertCmNone,
    convertCmNull,
    convertCmSInt8,
    convertCmUInt8,
    convertCmSInt16,
    convertCmUInt16,
    convertCmSInt32,
    convertCmUInt32,
    convertCmSInt64,
    convertCmUInt64,
    convertCmFloat32,
    convertCmFloat64,
    convertCmDateTime,
    convertCmInterval,
    convertCmNumeric,
    convertCmVariable,
    convertCmInVariable,
    convertCmBinary,  /* CMT_ID_BINARY    */
    convertCmInBinary,/* CMT_ID_IN_BINARY */
    convertCmBit,
    convertCmInBit,
    convertCmNibble,
    convertCmInNibble,
    NULL,               /* CMT_ID_LOBLOCATOR */
    convertCmInVariable /* CMT_ID_PTR */
};

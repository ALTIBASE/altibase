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
#include <mmcConvNumeric.h>
#include <mmcSession.h>
#include <mmcConvFmMT.h>

//fix BUG-17873
extern mtdModule mtdBigint;
extern mtdModule mtdBinary;
extern mtdModule mtdBit;
extern mtdModule mtdVarbit;
extern mtdModule mtdBlob;
extern mtdModule mtdBoolean;
extern mtdModule mtdChar;
extern mtdModule mtdNchar;
extern mtdModule mtdDate;
extern mtdModule mtdDouble;
extern mtdModule mtdFloat;
extern mtdModule mtdInteger;
extern mtdModule mtdInterval;
extern mtdModule mtdList;
extern mtdModule mtdNull;
extern mtdModule mtdNumber;
extern mtdModule mtdNumeric;
extern mtdModule mtdReal;
extern mtdModule mtdSmallint;
extern mtdModule mtdVarchar;
extern mtdModule mtdNvarchar;
extern mtdModule mtdByte;
extern mtdModule mtdNibble;
extern mtdModule mtdClob;
extern mtdModule mtsFile;
extern mtdModule mtsConnect;
extern mtdModule mtdBlobLocator;
extern mtdModule mtdClobLocator;


//fix BUG-18025.
static IDE_RC convertMtNull(cmtAny *aTarget, void * /*aSource*/)
{
    IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

// PROJ-2256 Communication protocol for efficient query result transmission
static IDE_RC convertMtRedundancy( cmtAny *aAny )
{
    cmtAnySetRedundancy( aAny );

    return IDE_SUCCESS;
}

//fix BUG-18025.
static IDE_RC convertMtBoolean(cmtAny *aTarget, void *aSource)
{
//fix BUG-17873
    if(mtdBoolean.isNull(NULL,
                         aSource ) == ID_TRUE )
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        IDE_TEST(cmtAnyWriteUChar(aTarget,
                                  (*(mtdBooleanType *)aSource == MTD_BOOLEAN_TRUE) ? 1 : 0)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
//fix BUG-18025.
static IDE_RC convertMtSmallInt(cmtAny *aTarget, void *aSource)
{
//fix BUG-17873
    if(mtdSmallint.isNull(NULL,
                          aSource) == ID_TRUE )
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        IDE_TEST(cmtAnyWriteSShort(aTarget, *(mtdSmallintType *)aSource) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
//fix BUG-18025.
static IDE_RC convertMtInteger(cmtAny *aTarget, void *aSource)
{
//fix BUG-17873
    if(mtdInteger.isNull(NULL,
                         aSource) == ID_TRUE )
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        IDE_TEST(cmtAnyWriteSInt(aTarget, *(mtdIntegerType *)aSource) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
//fix BUG-18025.
static IDE_RC convertMtBigInt(cmtAny *aTarget, void *aSource)
{
    //fix BUG-17873
    if( mtdBigint.isNull(NULL,
                         aSource) == ID_TRUE )
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        IDE_TEST(cmtAnyWriteSLong(aTarget, *(mtdBigintType *)aSource) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
//fix BUG-18025.
static IDE_RC convertMtBlobLocator(cmtAny *aTarget, void *aSource, UInt aLobSize)
{
//fix BUG-17873
    if(mtdBlobLocator.isNull(NULL,
                             aSource) == ID_TRUE )
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        // bug-19174
        IDE_TEST(cmtAnyWriteLobLocator(aTarget, *(mtdBlobLocatorType *)aSource, aLobSize) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
//fix BUG-18025
static IDE_RC convertMtClobLocator(cmtAny *aTarget, void *aSource, UInt aLobSize)
{
//fix BUG-17873
    if(mtdClobLocator.isNull(NULL,
                             aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        // bug-19174
        IDE_TEST(cmtAnyWriteLobLocator(aTarget, *(mtdBlobLocatorType *)aSource, aLobSize) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
//fix BUG-18025.
static IDE_RC convertMtReal(cmtAny *aTarget, void *aSource)
{
//fix BUG-17873
    if(mtdReal.isNull(NULL,
                      aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        IDE_TEST(cmtAnyWriteSFloat(aTarget, *(mtdRealType *)aSource) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

//fix BUG-18025.
static IDE_RC convertMtDouble(cmtAny *aTarget, void *aSource)
{
//fix BUG-17873
    if(mtdDouble.isNull(NULL,
                        aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        IDE_TEST(cmtAnyWriteSDouble(aTarget, *(mtdDoubleType *)aSource) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));

    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
//fix BUG-18025.
static IDE_RC convertMtDate(cmtAny *aTarget, void *aSource)
{
    mtdDateType *sMtDate = (mtdDateType *)aSource;
    cmtDateTime *sCmDateTime;

//fix BUG-17873
    if(mtdDate.isNull(NULL,
                      aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        IDE_TEST(cmtAnyGetDateTimeForWrite(aTarget, &sCmDateTime) != IDE_SUCCESS);

        sCmDateTime->mYear        = mtdDateInterface::year(sMtDate);
        sCmDateTime->mMonth       = mtdDateInterface::month(sMtDate);
        sCmDateTime->mDay         = mtdDateInterface::day(sMtDate);
        sCmDateTime->mHour        = mtdDateInterface::hour(sMtDate);
        sCmDateTime->mMinute      = mtdDateInterface::minute(sMtDate);
        sCmDateTime->mSecond      = mtdDateInterface::second(sMtDate);
        sCmDateTime->mMicroSecond = mtdDateInterface::microSecond(sMtDate);
        sCmDateTime->mTimeZone    = 0;
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
//fix BUG-18025.
static IDE_RC convertMtInterval(cmtAny *aTarget, void *aSource)
{
    mtdIntervalType *sMtInterval = (mtdIntervalType *)aSource;
    cmtInterval     *sCmInterval;
//fix BUG-17873
    if(mtdInterval.isNull(NULL,
                          aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        IDE_TEST(cmtAnyGetIntervalForWrite(aTarget, &sCmInterval) != IDE_SUCCESS);

        sCmInterval->mSecond      = sMtInterval->second;
        sCmInterval->mMicroSecond = sMtInterval->microsecond;
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
//fix BUG-18025.
static IDE_RC convertMtNumeric(cmtAny       *aTarget,
                               void         *aSource,
                               mmcByteOrder  aByteOrder)
{
    //fix  BUG-17773.
    UChar           sConvBuffer[MTD_NUMERIC_MANTISSA_MAXIMUM];
    UChar           sMantissaBuffer[MTD_NUMERIC_MANTISSA_MAXIMUM];
    UChar          *sConvMantissa = NULL;
    mmcConvNumeric  sSrc;
    mmcConvNumeric  sDst;
    cmtNumeric     *sCmNumeric = NULL;
    mtdNumericType *sMtNumeric = (mtdNumericType *)aSource;
    SShort          sScale;
    UChar           sPrecision;
    SChar           sSign;
    UInt            sMantissaLen;
    UInt            sSize;
    UInt            i;

    //fix BUG-17873
    if(mtdNumeric.isNull(NULL,
                         aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        sMantissaLen = sMtNumeric->length - 1;
        IDE_TEST_RAISE( sMantissaLen > MTD_NUMERIC_MANTISSA_MAXIMUM,
                        INVALID_MANTISSA_LENGTH);
        sSign        = (sMtNumeric->signExponent & 0x80) ? 1 : 0;
        sScale       = (sMtNumeric->signExponent & 0x7F);
        sPrecision   = sMantissaLen * 2;
        
        
        if ((sScale != 0) && (sMantissaLen > 0))
        {
            sScale = (sMantissaLen - (sScale - 64) * ((sSign == 1) ? 1 : -1)) * 2;
            if (sSign != 1)
            {
                for (i = 0; i < sMantissaLen; i++)
                {
                    //fix  BUG-17773.
                    sMantissaBuffer[i] = 99 - sMtNumeric->mantissa[i];
                }
            }
            else
            {
                //fix  BUG-17773.
                idlOS::memcpy(sMantissaBuffer,sMtNumeric->mantissa,sMantissaLen);
            }
            //fix  BUG-17773.
            sSrc.initialize(sMantissaBuffer,
                            sMantissaLen,
                            sMantissaLen,
                            100,
                            MMC_BYTEORDER_BIG_ENDIAN);
            sDst.initialize(sConvBuffer,
                            MMC_CONV_NUMERIC_BUFFER_SIZE,
                            0,
                            256,
                            aByteOrder);

            if ( (sMantissaBuffer[sMantissaLen - 1] % 10) == 0)
            {
                sSrc.shiftRight();
                sScale--;
            }
            if (sMantissaBuffer[0] == 0)
            {
                sPrecision -= 2;
            }
            else if (sMantissaBuffer[0] < 10)
            {
                sPrecision--;
            }
            IDE_TEST(sSrc.convert(&sDst) != IDE_SUCCESS);
            sConvMantissa = sDst.getBuffer();
            sSize         = sDst.getSize();
        }
        else
        {
            sScale     = 0;
            sPrecision = 0;
            sSize      = 0;
        }

        IDE_TEST(cmtAnyGetNumericForWrite(aTarget, &sCmNumeric, sSize) != IDE_SUCCESS);
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        /*
          if(sScale !=0 ....) 에서 else를 타면 sConvMantisaa가 null이다.
         */
        if(sConvMantissa != NULL)
        {
            
            idlOS::memcpy(sCmNumeric->mData, sConvMantissa, sSize);
        }
        else
        {
            //nothing to do
        }
        sCmNumeric->mSize      = sSize;
        sCmNumeric->mPrecision = sPrecision;
        sCmNumeric->mScale     = sScale;
        sCmNumeric->mSign      = sSign;
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_MANTISSA_LENGTH)
    {

        IDE_SET(ideSetErrorCode(mmERR_ABORT_InvalidMantissaLength,sMantissaLen));
        
    }
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
//fix BUG-18025.
static IDE_RC convertMtChar(cmiProtocolContext *aProtocolContext,
                            cmtAny             *aTarget,
                            void               *aSource)
{
    mtdCharType *sChar = (mtdCharType *)aSource;

    if(mtdChar.isNull(NULL,
                      aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        if( (aProtocolContext != NULL) &&
            (cmiCheckInVariable( aProtocolContext, sChar->length ) == ID_TRUE) )
        {
            IDE_TEST(cmtAnyWriteInVariable(aTarget, sChar->value, sChar->length) != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST(cmtAnyWriteVariable(aTarget, sChar->value, sChar->length) != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC convertMtNchar(cmiProtocolContext *aProtocolContext,
                             cmtAny             *aTarget,
                             void               *aSource)
{
    mtdNcharType *sNchar = (mtdNcharType *)aSource;

    if(mtdNchar.isNull(NULL,
                       aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        if( (aProtocolContext != NULL) &&
            (cmiCheckInVariable( aProtocolContext, sNchar->length ) == ID_TRUE) )
        {
            IDE_TEST(cmtAnyWriteInVariable(aTarget, sNchar->value, sNchar->length) != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST(cmtAnyWriteVariable(aTarget, sNchar->value, sNchar->length) != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC buildAnyMtChar(cmtAny             *aTarget,
                             void               *aSource)
{
    mtdCharType *sChar = (mtdCharType *)aSource;

    if(mtdChar.isNull(NULL,
                      aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        IDE_TEST(cmtAnyWriteInVariable(aTarget, sChar->value, sChar->length) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC buildAnyMtNchar(cmtAny             *aTarget,
                              void               *aSource)
{
    mtdNcharType *sNchar = (mtdNcharType *)aSource;

    if(mtdNchar.isNull(NULL,
                       aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        IDE_TEST(cmtAnyWriteInVariable(aTarget, sNchar->value, sNchar->length) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//fix BUG-18025.
static IDE_RC convertMtBinary(cmiProtocolContext *aProtocolContext,
                              cmtAny             *aTarget,
                              void               *aSource)
{
    mtdBinaryType *sBinary = (mtdBinaryType *)aSource;

    if( mtdBinary.isNull(NULL,
                         aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        if( (aProtocolContext != NULL) &&
            (cmiCheckInBinary( aProtocolContext, sBinary->mLength ) == ID_TRUE) )
        {
            IDE_TEST(cmtAnyWriteInBinary(aTarget, sBinary->mValue, sBinary->mLength) != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST(cmtAnyWriteBinary(aTarget, sBinary->mValue, sBinary->mLength) != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC buildAnyMtBinary(cmtAny             *aTarget,
                               void               *aSource)
{
    mtdBinaryType *sBinary = (mtdBinaryType *)aSource;

    if( mtdBinary.isNull(NULL,
                         aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        IDE_TEST(cmtAnyWriteInBinary(aTarget, sBinary->mValue, sBinary->mLength) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

//fix BUG-18025.
static IDE_RC convertMtByte(cmiProtocolContext *aProtocolContext,
                            cmtAny             *aTarget,
                            void               *aSource)
{
    mtdByteType *sByte = (mtdByteType *)aSource;

    if(mtdByte.isNull(NULL,
                      aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        if( (aProtocolContext != NULL) &&
            (cmiCheckInBinary( aProtocolContext, sByte->length ) == ID_TRUE) )
        {
            IDE_TEST(cmtAnyWriteInBinary(aTarget, sByte->value, sByte->length) != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST(cmtAnyWriteBinary(aTarget, sByte->value, sByte->length) != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC buildAnyMtByte(cmtAny             *aTarget,
                             void               *aSource)
{
    mtdByteType *sByte = (mtdByteType *)aSource;

    if(mtdByte.isNull(NULL,
                      aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        IDE_TEST(cmtAnyWriteInBinary(aTarget, sByte->value, sByte->length) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

//fix BUG-18025.
static IDE_RC convertMtBit(cmiProtocolContext *aProtocolContext,
                           cmtAny             *aTarget,
                           void               *aSource)
{
    mtdBitType *sBit = (mtdBitType *)aSource;

    if(mtdBit.isNull(NULL,
                     aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        if( (aProtocolContext != NULL) &&
            (cmiCheckInBit( aProtocolContext, sBit->length ) == ID_TRUE) )
        {
            IDE_TEST(cmtAnyWriteInBit(aTarget, sBit->value, sBit->length) != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST(cmtAnyWriteBit(aTarget, sBit->value, sBit->length) != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC buildAnyMtBit(cmtAny             *aTarget,
                            void               *aSource)
{
    mtdBitType *sBit = (mtdBitType *)aSource;

    if(mtdBit.isNull(NULL,
                     aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        IDE_TEST(cmtAnyWriteInBit(aTarget, sBit->value, sBit->length) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

//fix BUG-18025.
static IDE_RC convertMtNibble(cmiProtocolContext *aProtocolContext,
                              cmtAny             *aTarget,
                              void               *aSource)
{
    mtdNibbleType *sNibble = (mtdNibbleType *)aSource;

    if(mtdNibble.isNull(NULL,
                        aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        if( (aProtocolContext != NULL) &&
            (cmiCheckInNibble( aProtocolContext, sNibble->length ) == ID_TRUE) )
        {
            IDE_TEST(cmtAnyWriteInNibble(aTarget, sNibble->value, sNibble->length) != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST(cmtAnyWriteNibble(aTarget, sNibble->value, sNibble->length) != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC buildAnyMtNibble(cmtAny             *aTarget,
                               void               *aSource)
{
    mtdNibbleType *sNibble = (mtdNibbleType *)aSource;

    if(mtdNibble.isNull(NULL,
                        aSource) == ID_TRUE)
    {
        IDE_TEST(cmtAnySetNull(aTarget) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        IDE_TEST_RAISE( aSource == NULL,INVALID_SOURCE_DATA);
        IDE_TEST(cmtAnyWriteInNibble(aTarget, sNibble->value, sNibble->length) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(INVALID_SOURCE_DATA)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NullSourceData));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}



//  fix BUG-17249.
IDE_RC mmcConvFromMT::convert(cmiProtocolContext *aProtocolContext,
                              cmtAny             *aTarget,
                              void               *aSource,
                              UInt                aSourceType,
                              mmcSession         *aSession,
                              UInt                aLobSize)
{
    // BUG-22609 AIX 최적화 오류 수정
    // switch 에 UInt 형으로 음수값이 2번이상올때 서버 죽음
    SInt    sType   = (SInt)aSourceType;

    switch (sType)
    {
        //fix BUG-18025.
        case MTD_NULL_ID:
            IDE_TEST(convertMtNull(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_BOOLEAN_ID:
            IDE_TEST(convertMtBoolean(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_SMALLINT_ID:
            IDE_TEST(convertMtSmallInt(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_INTEGER_ID:
            IDE_TEST(convertMtInteger(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_BIGINT_ID:
            IDE_TEST(convertMtBigInt(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_BLOB_LOCATOR_ID:
            // bug-19174
            IDE_TEST(convertMtBlobLocator(aTarget, aSource, aLobSize) != IDE_SUCCESS);
            break;

        case MTD_CLOB_LOCATOR_ID:
            // bug-19174
            IDE_TEST(convertMtClobLocator(aTarget, aSource, aLobSize) != IDE_SUCCESS);
            break;

        case MTD_REAL_ID:
            IDE_TEST(convertMtReal(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_DOUBLE_ID:
            IDE_TEST(convertMtDouble(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_DATE_ID:
            IDE_TEST(convertMtDate(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_INTERVAL_ID:
            IDE_TEST(convertMtInterval(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_FLOAT_ID:
        case MTD_NUMBER_ID:
        case MTD_NUMERIC_ID:
            IDE_TEST(convertMtNumeric(aTarget,
                                      aSource,
                                      aSession->getNumericByteOrder()) != IDE_SUCCESS);
            break;

        case MTD_CHAR_ID:
        case MTD_VARCHAR_ID:
            IDE_TEST(convertMtChar(aProtocolContext, aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_NCHAR_ID:
        case MTD_NVARCHAR_ID:
            IDE_TEST(convertMtNchar(aProtocolContext, aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_BINARY_ID:
            IDE_TEST(convertMtBinary(aProtocolContext, aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_BYTE_ID:
        case MTD_VARBYTE_ID:
            IDE_TEST(convertMtByte(aProtocolContext, aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_BIT_ID:
        case MTD_VARBIT_ID:
            IDE_TEST(convertMtBit(aProtocolContext, aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_NIBBLE_ID:
            IDE_TEST(convertMtNibble(aProtocolContext, aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_BLOB_ID:
        case MTD_CLOB_ID:
        case MTD_LIST_ID:
        case MTD_NONE_ID:
        case MTS_FILETYPE_ID:
        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcConvFromMT::buildAny(cmtAny             *aTarget,
                               void               *aSource,
                               UInt                aSourceType,
                               mmcSession         *aSession,
                               UInt                aLobSize)
{
    // BUG-22609 AIX 최적화 오류 수정
    // switch 에 UInt 형으로 음수값이 2번이상올때 서버 죽음
    SInt    sType   = (SInt)aSourceType;

    switch (sType)
    {
        //fix BUG-18025.
        case MTD_NULL_ID:
            IDE_TEST(convertMtNull(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_BOOLEAN_ID:
            IDE_TEST(convertMtBoolean(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_SMALLINT_ID:
            IDE_TEST(convertMtSmallInt(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_INTEGER_ID:
            IDE_TEST(convertMtInteger(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_BIGINT_ID:
            IDE_TEST(convertMtBigInt(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_BLOB_LOCATOR_ID:
            // bug-19174
            IDE_TEST(convertMtBlobLocator(aTarget, aSource, aLobSize) != IDE_SUCCESS);
            break;

        case MTD_CLOB_LOCATOR_ID:
            // bug-19174
            IDE_TEST(convertMtClobLocator(aTarget, aSource, aLobSize) != IDE_SUCCESS);
            break;

        case MTD_REAL_ID:
            IDE_TEST(convertMtReal(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_DOUBLE_ID:
            IDE_TEST(convertMtDouble(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_DATE_ID:
            IDE_TEST(convertMtDate(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_INTERVAL_ID:
            IDE_TEST(convertMtInterval(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_FLOAT_ID:
        case MTD_NUMBER_ID:
        case MTD_NUMERIC_ID:
            IDE_TEST(convertMtNumeric(aTarget,
                                      aSource,
                                      aSession->getNumericByteOrder()) != IDE_SUCCESS);
            break;

        case MTD_CHAR_ID:
        case MTD_VARCHAR_ID:
            IDE_TEST(buildAnyMtChar(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_NCHAR_ID:
        case MTD_NVARCHAR_ID:
            IDE_TEST(buildAnyMtNchar(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_BINARY_ID:
            IDE_TEST(buildAnyMtBinary(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_BYTE_ID:
        case MTD_VARBYTE_ID:
            IDE_TEST(buildAnyMtByte(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_BIT_ID:
        case MTD_VARBIT_ID:
            IDE_TEST(buildAnyMtBit(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_NIBBLE_ID:
            IDE_TEST(buildAnyMtNibble(aTarget, aSource) != IDE_SUCCESS);
            break;

        case MTD_BLOB_ID:
        case MTD_CLOB_ID:
        case MTD_LIST_ID:
        case MTD_NONE_ID:
        case MTS_FILETYPE_ID:
        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_DATA_CONVERSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2331 */
IDE_RC mmcConvFromMT::compareMtChar( mmcBaseRow    *aBaseRow,
                             qciBindColumn *aMetaData,
                             mtdCharType   *aData )
{
    mtdCharType *sBaseColumn = (mtdCharType *)&( aBaseRow->mBaseRow[aBaseRow->mBaseColumnPos] );
    mtdCharType *sCurColumn  = aData;
    SInt         sIsRedundancy;

    if ( aBaseRow->mIsFirstRow != ID_TRUE )
    {
        if ( sBaseColumn->length == sCurColumn->length )
        {
            sIsRedundancy = idlOS::memcmp( sBaseColumn->value, sCurColumn->value, sCurColumn->length );

            if ( sIsRedundancy == 0 )
            {
                aBaseRow->mIsRedundant = ID_TRUE;
            }
            else
            {
            	aBaseRow->mIsRedundant = ID_FALSE;
            }
        }
        else
        {
            aBaseRow->mIsRedundant = ID_FALSE;
        }
    }

    if ( aBaseRow->mIsRedundant == ID_FALSE )
    {
        idlOS::memcpy( sBaseColumn, sCurColumn, ID_SIZEOF( UShort ) + sCurColumn->length );
    }

    aBaseRow->mBaseColumnPos += aMetaData->mMaxByteSize;

    return IDE_SUCCESS;
}

// PROJ-2256 Communication protocol for efficient query result transmission
IDE_RC mmcConvFromMT::compareMtChar( mmcBaseRow    *aBaseRow,
                                     qciBindColumn *aMetaData,
                                     void          *aData,
                                     cmtAny        *aAny )
{
    mtdCharType *sBaseColumn = (mtdCharType *)&( aBaseRow->mBaseRow[aBaseRow->mBaseColumnPos] );
    mtdCharType *sCurColumn  = (mtdCharType *)aData;
    SInt         sIsRedundancy;

    if ( aBaseRow->mIsFirstRow != ID_TRUE )
    {    
        if ( sBaseColumn->length == sCurColumn->length )
        {    
            sIsRedundancy = idlOS::memcmp( sBaseColumn->value, sCurColumn->value, sCurColumn->length );

            if ( sIsRedundancy == 0 )
            {    
                IDE_TEST( convertMtRedundancy( aAny ) != IDE_SUCCESS );
            }    
        }    
    }    

    if ( aAny->mType != CMT_ID_REDUNDANCY )
    {    
        idlOS::memcpy( sBaseColumn, sCurColumn, ID_SIZEOF( UShort ) + sCurColumn->length );
    }    

    aBaseRow->mBaseColumnPos += aMetaData->mMaxByteSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-2256 Communication protocol for efficient query result transmission
IDE_RC mmcConvFromMT::checkDataRedundancy( mmcBaseRow    *aBaseRow,
                                           qciBindColumn *aMetaData,
                                           void          *aData,
                                           cmtAny        *aAny )
{
    switch( aMetaData->mType )
    {
        case MTD_CHAR_ID:
        case MTD_VARCHAR_ID:
        case MTD_NCHAR_ID:
        case MTD_NVARCHAR_ID:
            IDE_TEST( compareMtChar( aBaseRow, aMetaData, aData, aAny )
                      != IDE_SUCCESS );
            break;

        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

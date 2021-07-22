/** 
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

/***********************************************************************
 * $Id: mtxFromVarbyteTo.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/

#include <mtx.h>

extern mtxModule mtxFromVarbyteTo;

static mtxSerialExecuteFunc mtxFromVarbyteToGetSerialExecute( UInt aMtd1Type,
                                                              UInt /* aMtd2Type */ );

static IDE_RC mtxFromVarbyteToVarchar( mtxEntry ** aEntry );
static IDE_RC mtxFromVarbyteToByte( mtxEntry ** aEntry );

mtxModule mtxFromVarbyteTo = {
    MTX_EXECUTOR_ID_VARBYTE,
    mtx::calculateNA,
    mtxFromVarbyteToGetSerialExecute
};

static mtxSerialExecuteFunc mtxFromVarbyteToGetSerialExecute( UInt aMtd1Type,
                                                              UInt /* aMtd2Type */ )
{
    mtxSerialExecuteFunc sFunc = mtx::calculateNA;

    switch ( aMtd1Type )
    {
        case MTD_VARCHAR_ID:
            sFunc = mtxFromVarbyteToVarchar;

            break;

        case MTD_BYTE_ID:
            sFunc = mtxFromVarbyteToByte;

            break;

        default:
            sFunc = mtx::calculateNA;

            break;
    }

    return sFunc;
}

static IDE_RC mtxFromVarbyteToVarchar( mtxEntry ** aEntry )
{
    mtdCharType * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar         sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdByteType * sArg1Val  = (mtdByteType *)( aEntry[1] + 1 )->mAddress;
    SChar         sFull;
    SChar         sHalf;
    UInt          sCount;
    UInt          sIterator;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_BYTE( sArg1Val ) )
    {
        MTX_SET_NULL_CHAR( sReturn );
    }
    else
    {
        for ( sIterator = 0,
                  sCount = 0;
              sIterator < sArg1Val->length;
              sIterator++,
                  sCount += 2 )
        {
            sFull = sArg1Val->value[ sIterator ];

            sHalf = ( sFull & 0xF0 ) >> 4;

            sReturn->value[ sCount + 0 ] = ( sHalf < 10 ) ? ( sHalf + '0' ) : ( sHalf + 'A' - 10 );

            sHalf = ( sFull & 0x0F );

            sReturn->value[ sCount + 1 ] = ( sHalf < 10 ) ? ( sHalf + '0' ) : ( sHalf + 'A' - 10 );
        }

        sReturn->length = sCount;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromVarbyteToVarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromVarbyteToByte( mtxEntry ** aEntry )
{
    mtdByteType * sReturn   = (mtdByteType *)aEntry[0]->mAddress;
    UChar         sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdByteType * sArg1Val  = (mtdByteType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    sReturn->length = sArg1Val->length;

    idlOS::memcpy( sReturn->value,
                   sArg1Val->value,
                   sArg1Val->length );

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromVarbyteToByte",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

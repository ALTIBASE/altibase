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
 * $Id: mtxFromRealTo.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/

#include <mtx.h>

extern mtxModule mtxFromRealTo;

static mtxSerialExecuteFunc mtxFromRealToGetSerialExecute( UInt aMtd1Type,
                                                           UInt /* aMtd2Type */ );

static IDE_RC mtxFromRealToDouble( mtxEntry ** aEntry );
static IDE_RC mtxFromRealToChar( mtxEntry ** aEntry );
static IDE_RC mtxFromRealToVarchar( mtxEntry ** aEntry );

mtxModule mtxFromRealTo = {
    MTX_EXECUTOR_ID_REAL,
    mtx::calculateNA,
    mtxFromRealToGetSerialExecute
};

static mtxSerialExecuteFunc mtxFromRealToGetSerialExecute( UInt aMtd1Type,
                                                           UInt /* aMtd2Type */ )
{
    mtxSerialExecuteFunc sFunc = mtx::calculateNA;

    switch ( aMtd1Type )
    {
        case MTD_CHAR_ID:
            sFunc = mtxFromRealToChar;

            break;

        case MTD_DOUBLE_ID:
            sFunc = mtxFromRealToDouble;

            break;

        case MTD_VARCHAR_ID:
            sFunc = mtxFromRealToVarchar;

            break;

        default:
            sFunc = mtx::calculateNA;

            break;
    }

    return sFunc;
}

static IDE_RC mtxFromRealToDouble( mtxEntry ** aEntry )
{
    mtdDoubleType * sReturn   = (mtdDoubleType *)aEntry[0]->mAddress;
    UChar           sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdRealType     sArg1Val  = *(mtdRealType *)( aEntry[1] + 1 )->mAddress;
    SChar           sTemp[16];

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_REAL( & sArg1Val ) )
    {
        MTX_SET_NULL_DOUBLE( sReturn );
    }
    else
    {
        idlOS::snprintf( sTemp, ID_SIZEOF( sTemp ),
                         "%"ID_FLOAT_G_FMT"",
                         sArg1Val );

        *sReturn = idlOS::strtod( sTemp, NULL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromRealToDouble",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromRealToChar( mtxEntry ** aEntry )
{
    mtdCharType  * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar          sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdRealType    sArg1Val  = *(mtdRealType *)( aEntry[1] + 1 )->mAddress;
    mtdDoubleType  sDouble;
    SChar          sTemp[16];

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_REAL( & sArg1Val ) )
    {
        MTX_SET_NULL_CHAR( sReturn );
    }
    else
    {
        idlOS::snprintf( sTemp, ID_SIZEOF( sTemp ),
                         "%"ID_FLOAT_G_FMT"",
                         sArg1Val );

        sDouble = idlOS::strtod( sTemp, NULL );

        IDE_TEST( mtv::nativeR2Character( sDouble,
                                          sReturn )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromRealToChar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromRealToVarchar( mtxEntry ** aEntry )
{
    mtdCharType  * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar          sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdRealType    sArg1Val  = *(mtdRealType *)( aEntry[1] + 1 )->mAddress;
    mtdDoubleType  sDouble;
    SChar          sTemp[16];

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_REAL( & sArg1Val ) )
    {
        MTX_SET_NULL_VARCHAR( sReturn );
    }
    else
    {
        idlOS::snprintf( sTemp, ID_SIZEOF( sTemp ),
                         "%"ID_FLOAT_G_FMT"",
                         sArg1Val );

        sDouble = idlOS::strtod( sTemp, NULL );

        IDE_TEST( mtv::nativeR2Character( sDouble,
                                          sReturn )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromRealToVarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

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
 * $Id: mtxFromNcharTo.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/

#include <mtx.h>

extern mtxModule mtxFromNcharTo;

static mtxSerialExecuteFunc mtxFromNcharToGetSerialExecute( UInt aMtd1Type,
                                                      UInt /* aMtd2Type */ );

static IDE_RC mtxFromNcharToChar( mtxEntry ** aEntry );
static IDE_RC mtxFromNcharToNvarchar( mtxEntry ** aEntry );

mtxModule mtxFromNcharTo = {
    MTX_EXECUTOR_ID_NCHAR,
    mtx::calculateNA,
    mtxFromNcharToGetSerialExecute
};


static mtxSerialExecuteFunc mtxFromNcharToGetSerialExecute( UInt aMtd1Type,
                                                      UInt /* aMtd2Type */ )
{
    mtxSerialExecuteFunc sFunc = mtx::calculateNA;

    switch ( aMtd1Type )
    {
        case MTD_CHAR_ID:
            sFunc = mtxFromNcharToChar;

            break;

        case MTD_NVARCHAR_ID:
            sFunc = mtxFromNcharToNvarchar;

            break;

        default:
            sFunc = mtx::calculateNA;

            break;
    }

    return sFunc;
}

static IDE_RC mtxFromNcharToChar( mtxEntry ** aEntry )
{
    mtdCharType  * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar          sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNcharType * sArg1Val  = (mtdNcharType *)( aEntry[1] + 1 )->mAddress;
    mtcColumn    * sArg2Val  = (mtcColumn *)aEntry[2]->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    IDE_TEST( mtdNcharInterface::toChar( sArg2Val->precision,
                                         (const mtlModule *) mtl::mNationalCharSet,
                                         (const mtlModule *) mtl::mDBCharSet,
                                         sArg1Val,
                                         sReturn )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromNcharToChar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromNcharToNvarchar( mtxEntry ** aEntry )
{
    mtdNcharType * sReturn   = (mtdNcharType *)aEntry[0]->mAddress;
    UChar          sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNcharType * sArg1Val  = (mtdNcharType *)( aEntry[1] + 1 )->mAddress;

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
                                  "mtxFromNcharToNvarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

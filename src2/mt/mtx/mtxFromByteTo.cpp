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
 * $Id: mtxFromByteTo.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/

#include <mtx.h>

extern mtxModule mtxFromByteTo;

static mtxSerialExecuteFunc mtxFromByteToGetSerialExecute( UInt /* aMtd1Type */,
                                                           UInt /* aMtd2Type */ );

static IDE_RC mtxFromByteToVarbyte( mtxEntry ** aEntry );

mtxModule mtxFromByteTo = {
    MTX_EXECUTOR_ID_BYTE,
    mtx::calculateNA,
    mtxFromByteToGetSerialExecute
};

static mtxSerialExecuteFunc mtxFromByteToGetSerialExecute( UInt /* aMtd1Type */,
                                                           UInt /* aMtd2Type */ )
{
    return mtxFromByteToVarbyte;
}

static IDE_RC mtxFromByteToVarbyte( mtxEntry ** aEntry )
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
                                  "mtxFromByteToVarbyte",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

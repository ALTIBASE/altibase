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
 * $Id: mtxFromDateTo.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/

#include <mtx.h>

extern mtxModule mtxFromDateTo;

static mtxSerialExecuteFunc mtxFromDateToGetSerialExecute( UInt aMtd1Type,
                                                           UInt /* aMtd2Type */ );

static IDE_RC mtxFromDateToChar( mtxEntry ** aEntry );
static IDE_RC mtxFromDateToVarchar( mtxEntry ** aEntry );

mtxModule mtxFromDateTo = {
    MTX_EXECUTOR_ID_DATE,
    mtx::calculateNA,
    mtxFromDateToGetSerialExecute
};

static mtxSerialExecuteFunc mtxFromDateToGetSerialExecute( UInt aMtd1Type,
                                                           UInt /* aMtd2Type */ )
{
    mtxSerialExecuteFunc sFunc = mtx::calculateNA;

    switch ( aMtd1Type )
    {
        case MTD_CHAR_ID:
            sFunc = mtxFromDateToChar;

            break;

        case MTD_VARCHAR_ID:
            sFunc = mtxFromDateToVarchar;

            break;

        default:
            sFunc = mtx::calculateNA;

            break;
    }

    return sFunc;
}

static IDE_RC mtxFromDateToChar( mtxEntry ** aEntry )
{
    mtdCharType  * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar          sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdDateType  * sArg1Val  = (mtdDateType *)( aEntry[1] + 1 )->mAddress;
    SChar        * sArg2Val  = (SChar *)aEntry[2]->mAddress;
    idBool       * sArg3Val  = (idBool *)aEntry[3]->mAddress;
    UInt           sLength;
    SInt           sFormatLen;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTD_DATE_IS_NULL( sArg1Val ) )
    {
        MTX_SET_NULL_CHAR( sReturn );
    }
    else
    {
        sLength    = 0;
        sFormatLen = idlOS::strlen( sArg2Val );

        IDE_TEST( mtdDateInterface::toChar( sArg1Val,
                                            sReturn->value,
                                            & sLength,
                                            MTC_TO_CHAR_MAX_PRECISION,
                                            (UChar*)sArg2Val,
                                            sFormatLen )
                  != IDE_SUCCESS );

        /* PROJ-1436 - dateFormat을 참조했음을 표시한다. */
        *sArg3Val= ID_TRUE;

        sReturn->length = (UShort)sLength;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromDateToChar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


static IDE_RC mtxFromDateToVarchar( mtxEntry ** aEntry )
{
    mtdCharType  * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar          sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdDateType  * sArg1Val  = (mtdDateType *)( aEntry[1] + 1 )->mAddress;
    SChar        * sArg2Val  = (SChar *)aEntry[2]->mAddress;
    idBool       * sArg3Val  = (idBool *)aEntry[3]->mAddress;
    UInt           sLength;
    SInt           sFormatLen;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTD_DATE_IS_NULL( sArg1Val ) )
    {
        MTX_SET_NULL_CHAR( sReturn );
    }
    else
    {
        sLength    = 0;
        sFormatLen = idlOS::strlen( sArg2Val );

        IDE_TEST( mtdDateInterface::toChar( sArg1Val,
                                            sReturn->value,
                                            & sLength,
                                            MTC_TO_CHAR_MAX_PRECISION,
                                            (UChar*)sArg2Val,
                                            sFormatLen )
                  != IDE_SUCCESS );

        /* PROJ-1436 - dateFormat을 참조했음을 표시한다. */
        *sArg3Val= ID_TRUE;

        sReturn->length = (UShort)sLength;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromDateToVarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

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
 * $Id: mtxFromVarbitTo.cpp 84916 2019-02-22 04:21:01Z andrew.shin $
 **********************************************************************/

#include <mtx.h>

extern mtxModule mtxFromVarbitTo;

static mtxSerialExecuteFunc mtxFromVarbitToGetSerialExecute( UInt aMtd1Type,
                                                             UInt /* aMtd2Type */ );

static IDE_RC mtxFromVarbitToVarchar( mtxEntry ** aEntry );
static IDE_RC mtxFromVarbitToBit( mtxEntry ** aEntry );

mtxModule mtxFromVarbitTo = {
    MTX_EXECUTOR_ID_VARBIT,
    mtx::calculateNA,
    mtxFromVarbitToGetSerialExecute
};

static mtxSerialExecuteFunc mtxFromVarbitToGetSerialExecute( UInt aMtd1Type,
                                                             UInt /* aMtd2Type */ )
{
    mtxSerialExecuteFunc sFunc = mtx::calculateNA;

    switch ( aMtd1Type )
    {
        case MTD_VARCHAR_ID:
            sFunc = mtxFromVarbitToVarchar;

            break;

        case MTD_BIT_ID:
            sFunc = mtxFromVarbitToBit;

            break;

        default:
            sFunc = mtx::calculateNA;

            break;
    }

    return sFunc;
}

static IDE_RC mtxFromVarbitToVarchar( mtxEntry ** aEntry )
{
    mtdCharType * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar         sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBitType  * sArg1Val  = (mtdBitType *)( aEntry[1] + 1 )->mAddress;

    UInt  sBitLength  = 0;
    UInt  sByteLength = 0;
    UInt  sBitCnt     = 0;
    UInt  sIndex      = 0;
    UInt  sByteIndex  = 0;
    UInt  sIterator   = 0;
    UChar sChr        = 0;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_VARBIT( sArg1Val ) )
    {
        MTX_SET_NULL_VARCHAR( sReturn );
    }
    else
    {
        sBitLength  = sArg1Val->length;
        sByteLength = BIT_TO_BYTE( sArg1Val->length );

        for ( sByteIndex = 0;
              sByteIndex < sByteLength;
              sByteIndex++ )
        {
            for ( sIterator = 0;
                  sIterator < 8;
                  sIterator++ )
            {
                if ( sBitCnt > sBitLength - 1 )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }

                sChr = ( ( sArg1Val->value[ sByteIndex ] << sIterator ) >> 7 ) & 0x1;

                /*
                * 00001111의 경우 앞의 0000도 같이 변환이 되어야 한다.
                *
                * insert into tab_bit values ( bit'11111111' );
                * insert into tab_bit values ( bit'00001111' );
                * insert into tab_bit values ( bit'11110000' );
                * select to_char( a ) from tab_bit where a like '1111%';
                * select to_char( a ) from tab_bit where a like '11110%';
                * select to_char( a ) from tab_bit where a like '0%';
                *
                */
                /* if ( sChr != 0 || sIsEmpty == 0 ) */
                {
                    /* sIsEmpty = 0; */
                    sReturn->value[ sIndex ] = sChr + '0';
                    sIndex++;
                }
                sBitCnt++;
            }
        }

        sReturn->length = sIndex;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromVarbitToVarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromVarbitToBit( mtxEntry ** aEntry )
{
    mtdBitType * sReturn   = (mtdBitType *)aEntry[0]->mAddress;
    UChar        sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBitType * sArg1Val  = (mtdBitType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    sReturn->length = sArg1Val->length;

    idlOS::memcpy( sReturn->value,
                   sArg1Val->value,
                   BIT_TO_BYTE( sArg1Val->length ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromVarbitToBit",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

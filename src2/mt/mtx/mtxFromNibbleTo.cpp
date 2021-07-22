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
 * $Id: mtxFromNibbleTo.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/

#include <mtx.h>

extern mtxModule mtxFromNibbleTo;

static mtxSerialExecuteFunc mtxFromNibbleToGetSerialExecute( UInt /* aMtd1Type */,
                                                             UInt /* aMtd2Type */ );

static IDE_RC mtxFromNibbleToVarchar( mtxEntry ** aEntry );

mtxModule mtxFromNibbleTo = {
    MTX_EXECUTOR_ID_NIBBLE,
    mtx::calculateNA,
    mtxFromNibbleToGetSerialExecute
};

static mtxSerialExecuteFunc mtxFromNibbleToGetSerialExecute( UInt /* aMtd1Type */,
                                                             UInt /* aMtd2Type */ )
{
    return mtxFromNibbleToVarchar;
}

static IDE_RC mtxFromNibbleToVarchar( mtxEntry ** aEntry )
{
    mtdCharType   * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar           sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNibbleType * sArg1Val  = (mtdNibbleType *)( aEntry[1] + 1 )->mAddress;
    SChar           sHalf;
    SChar           sFull;
    SInt            sIterator;
    SInt            sCount;
    SInt            sFence;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_NIBBLE( sArg1Val ) )
    {
        MTX_SET_NULL_VARCHAR( sReturn );
    }
    else
    {
        sFence = ( sArg1Val->length + 1 ) / 2;

        for( sIterator = 0, sCount = 0;
             sIterator < sFence;
             sIterator++ )
        {
            sFull = sArg1Val->value[ sIterator ];

            sHalf = ( sFull & 0xF0 ) >> 4;

            sReturn->value[ sCount ] = ( sHalf < 10 ) ? ( sHalf + '0' ) : ( sHalf + 'A' - 10 );

            sCount++;

            if ( sCount >= sArg1Val->length )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }

            sHalf = ( sFull & 0x0F );

            sReturn->value[ sCount ] = ( sHalf < 10 ) ? ( sHalf + '0' ) : ( sHalf + 'A' - 10 );

            sCount++;

            if ( sCount >= sArg1Val->length )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }

        sReturn->length = sCount;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromNibbleToVarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

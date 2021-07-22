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
 * $Id: mtxFromIntervalTo.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/

#include <mtx.h>

extern mtxModule mtxFromIntervalTo;

static mtxSerialExecuteFunc mtxFromIntervalToGetSerialExecute( UInt /* aMtd1Type */,
                                                               UInt /* aMtd2Type */ );

static IDE_RC mtxFromIntervalToDouble( mtxEntry ** aEntry );

mtxModule mtxFromIntervalTo = {
    MTX_EXECUTOR_ID_INTERVAL,
    mtx::calculateNA,
    mtxFromIntervalToGetSerialExecute
};

static mtxSerialExecuteFunc mtxFromIntervalToGetSerialExecute( UInt /* aMtd1Type */,
                                                               UInt /* aMtd2Type */ )
{
    return mtxFromIntervalToDouble;
}

static IDE_RC mtxFromIntervalToDouble( mtxEntry ** aEntry )
{
    mtdDoubleType   * sReturn   = (mtdDoubleType *)aEntry[0]->mAddress;
    UChar             sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdIntervalType * sArg1Val  = (mtdIntervalType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTD_INTERVAL_IS_NULL( sArg1Val ) )
    {
        MTX_SET_NULL_DOUBLE( sReturn );
    }
    else
    {
        *sReturn = ( (SDouble)sArg1Val->second / 864e2 )
            + ( (SDouble)sArg1Val->microsecond / 864e8 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromIntervalToDouble",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

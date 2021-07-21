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
 * $Id: mtxAnd.cpp 84841 2019-01-31 02:04:54Z andrew.shin $
 **********************************************************************/

#include <mtx.h>

extern mtxModule mtxAnd;

static mtxSerialExecuteFunc mtxAndGetSerialExecute( UInt /* aMtd1Type */,
                                                    UInt /* aMtd2Type */ );

static IDE_RC mtxAndBoolean( mtxEntry ** aEntry );

mtxModule mtxAnd = {
    MTX_EXECUTOR_ID_AND,
    mtxAndBoolean,
    mtxAndGetSerialExecute
};

static mtxSerialExecuteFunc mtxAndGetSerialExecute( UInt /* aMtd1Type */,
                                                    UInt /* aMtd2Type */ )
{
    return mtxAndBoolean;
}

static IDE_RC mtxAndBoolean( mtxEntry ** aEntry )
{
    mtdBooleanType * sRetrun  = (mtdBooleanType *)aEntry[0]->mAddress;
    mtdBooleanType   sArg1Val = *(mtdBooleanType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST( sArg1Val > MTD_BOOLEAN_NULL );

    if ( sArg1Val == MTD_BOOLEAN_TRUE )
    {
        *sRetrun = MTD_BOOLEAN_TRUE;
    }
    else
    {
        *sRetrun = MTD_BOOLEAN_FALSE;

        aEntry[3] = aEntry[2];
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

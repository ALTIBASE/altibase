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
 * $Id: mtxRid.cpp 84841 2019-01-31 02:04:54Z andrew.shin $
 **********************************************************************/

#include <mtx.h>

extern mtxModule mtxRid;

static mtxSerialExecuteFunc mtxRidGetSerialExecute( UInt /* aMtd1Type */,
                                                    UInt /* aMtd2Type */ );

static IDE_RC mtxGetRid( mtxEntry ** aEntry );

mtxModule mtxRid = {
    MTX_EXECUTOR_ID_RID,
    mtxGetRid,
    mtxRidGetSerialExecute
};

static mtxSerialExecuteFunc mtxRidGetSerialExecute( UInt /* aMtd1Type */,
                                                    UInt /* aMtd2Type */ )
{
    return mtxGetRid;
}

static IDE_RC mtxGetRid( mtxEntry ** aEntry )
{
    mtcTuple * sTuple  = (mtcTuple  *)aEntry[1]->mAddress;

    IDE_TEST( SC_GRID_IS_NULL( sTuple->rid ) == ID_TRUE );

    aEntry[0]->mAddress = (void*)&( sTuple->rid );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

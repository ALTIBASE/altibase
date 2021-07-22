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
 * $Id: mtxMultiply.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/
#include <mtx.h>

extern mtxModule mtxMultiply;

static mtxSerialExecuteFunc mtxMultiplyGetSerialExecute( UInt aMtd1Type,
                                                         UInt /* aMtd2Type */ );

static IDE_RC mtxMultiplyReal( mtxEntry ** aEntry );
static IDE_RC mtxMultiplyDouble( mtxEntry ** aEntry );
static IDE_RC mtxMultiplyFloat( mtxEntry ** aEntry );

mtxModule mtxMultiply = {
    MTX_EXECUTOR_ID_MULTIPLY,
    mtx::calculateNA,
    mtxMultiplyGetSerialExecute
};

static mtxSerialExecuteFunc mtxMultiplyGetSerialExecute( UInt aMtd1Type,
                                                         UInt /* aMtd2Type */ )
{
    mtxSerialExecuteFunc sFunc = mtx::calculateNA;

    switch ( aMtd1Type )
    {
        case MTD_NUMERIC_ID:
        case MTD_FLOAT_ID:
            sFunc = mtxMultiplyFloat;

            break;

        case MTD_REAL_ID:
            sFunc = mtxMultiplyReal;

            break;

        case MTD_DOUBLE_ID:
            sFunc = mtxMultiplyDouble;

            break;

        default:
            sFunc = mtx::calculateNA;

            break;
    }

    return sFunc;
}

static IDE_RC mtxMultiplyReal( mtxEntry ** aEntry )
{
    mtdRealType * sReturn   = (mtdRealType *)aEntry[0]->mAddress;
    UChar         sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdRealType * sArg1Val  = (mtdRealType *)( aEntry[1] + 1 )->mAddress;
    UChar         sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdRealType * sArg2Val  = (mtdRealType *)( aEntry[2] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_REAL( sArg1Val ) || MTX_IS_NULL_REAL( sArg2Val ) )
    {
        MTX_SET_NULL_REAL( sReturn );
    }
    else
    {
        *sReturn = ( *sArg1Val ) * ( *sArg2Val );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxMultiplyReal",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxMultiplyDouble( mtxEntry ** aEntry )
{
    mtdDoubleType * sReturn   = (mtdDoubleType *)aEntry[0]->mAddress;
    UChar           sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdDoubleType * sArg1Val  = (mtdDoubleType *)( aEntry[1] + 1 )->mAddress;
    UChar           sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdDoubleType * sArg2Val  = (mtdDoubleType *)( aEntry[2] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_DOUBLE( sArg1Val ) || MTX_IS_NULL_DOUBLE( sArg2Val ) )
    {
        MTX_SET_NULL_DOUBLE( sReturn );
    }
    else
    {
        *sReturn = ( *sArg1Val ) * ( *sArg2Val );

        /* fix BUG-13757
         *  double'0' * double'-1' = double'-0' 이 나오므로
         *  -0 값을 0 값으로 처리하기 위한 코드임.
         */
        /* PATCH(BEGIN): GNU MATH LIBRARY - REMOVE MINUS ZERO */
        if ( *sReturn < 0.0 )
        {
            /* Nothing to do */
        }
        else if ( *sReturn > 0.0 )
        {
            /* Nothing to do */
        }
        else
        {
            *sReturn = 0;
        }
        /* PATCH(END): GNU MATH LIBRARY - REMOVE MINUS ZERO */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxMultiplyDouble",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxMultiplyFloat( mtxEntry ** aEntry )
{
    mtdNumericType * sReturn   = (mtdNumericType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNumericType * sArg1Val  = (mtdNumericType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdNumericType * sArg2Val  = (mtdNumericType *)( aEntry[2] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_FLOAT( sArg1Val ) || MTX_IS_NULL_FLOAT( sArg2Val ) )
    {
        MTX_SET_NULL_FLOAT( sReturn );
    }
    else
    {
        IDE_TEST( mtc::multiplyFloat( sReturn,
                                      MTD_FLOAT_PRECISION_MAXIMUM,
                                      sArg1Val,
                                      sArg2Val )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxMultiplyFloat",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

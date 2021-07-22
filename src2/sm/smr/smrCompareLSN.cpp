/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: smrCompareLSN.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <smDef.h>
#include <smrCompareLSN.h>

idBool smrCompareLSN::isGT( const smLSN   * aLSN1, 
                            const smLSN   * aLSN2 )
{

    if ( (UInt)(aLSN1->mFileNo) > (UInt)(aLSN2->mFileNo) )
    {
        return ID_TRUE;
    }
    else 
    {
        if ( (UInt)(aLSN1->mFileNo) == (UInt)(aLSN2->mFileNo) )
        {
            if ( (UInt)(aLSN1->mOffset) > (UInt)(aLSN2->mOffset) )
            {
                return ID_TRUE;
            }
        }
    }

    return ID_FALSE;

}

idBool smrCompareLSN::isGTE( const smLSN   * aLSN1, 
                             const smLSN   * aLSN2 )
{

    if ( (UInt)(aLSN1->mFileNo) > (UInt)(aLSN2->mFileNo) )
    {
        return ID_TRUE;
    }
    else 
    {
        if ( (UInt)(aLSN1->mFileNo) == (UInt)(aLSN2->mFileNo) )
        {
            if ( (UInt)(aLSN1->mOffset) >= (UInt)(aLSN2->mOffset) )
            {
                return ID_TRUE;
            }
        }
    }

    return ID_FALSE;

}

idBool smrCompareLSN::isEQ( const smLSN   * aLSN1, 
                            const smLSN   * aLSN2 )
{

    if ( ( (UInt)(aLSN1->mFileNo) == (UInt)(aLSN2->mFileNo) ) &&
         ( (UInt)(aLSN1->mOffset) == (UInt)(aLSN2->mOffset) ) )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }

}

idBool smrCompareLSN::isLTE( const smLSN   * aLSN1, 
                             const smLSN   * aLSN2 )
{

    if ( (UInt)(aLSN1->mFileNo) < (UInt)(aLSN2->mFileNo) )
    {
        return ID_TRUE;
    }
    else 
    {
        if ( (UInt)(aLSN1->mFileNo) == (UInt)(aLSN2->mFileNo) )
        {
            if ( (UInt)(aLSN1->mOffset) <= (UInt)(aLSN2->mOffset) )
            {
                return ID_TRUE;
            }
        }
    }

    return ID_FALSE;

}

idBool smrCompareLSN::isLT( const smLSN   * aLSN1, 
                            const smLSN   * aLSN2 )
{

    if ( (UInt)(aLSN1->mFileNo) < (UInt)(aLSN2->mFileNo) )
    {
        return ID_TRUE;
    }
    else 
    {
        if ( (UInt)(aLSN1->mFileNo) == (UInt)(aLSN2->mFileNo) )
        {
            if ( (UInt)(aLSN1->mOffset) < (UInt)(aLSN2->mOffset) )
            {
                return ID_TRUE;
            }
        }
    }

    return ID_FALSE;

}

/***********************************************************************
 *
 * Description :
 *
 * LSN 값이 0인지 검사한다.
 * 
 * Implementation : 
 *
 * 
 *
 **********************************************************************/
idBool smrCompareLSN::isZero( const smLSN   * aLSN )
{

    smLSN         sZeroLSN;
    
    SM_LSN_INIT(sZeroLSN);

    if ( isEQ( &sZeroLSN, aLSN ) == ID_TRUE )
    {
        return ID_TRUE;
    }
    
    return ID_FALSE;

}

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
 * $Id: mtlUTF16.cpp 25691 2008-04-18 03:32:56Z leejy $
 **********************************************************************/

#include <mte.h>
#include <mtl.h>
#include <mtdTypes.h>

//#define mtlUTF16_4BYTE_TYPE_HIGH_SURROGATE(c)  ( (c)>=0xDB00 && (c)<=0xDBFF )
//#define mtlUTF16_4BYTE_TYPE_LOW_SURROGATE(c)   ( (c)>=0xDC00 && (c)<=0xDFFF )

extern mtlModule mtlUTF16;

extern UChar* mtl2BYTESpecialCharSet[];

static mtlNCRet mtlUTF16NextCharPtr( UChar ** aSource, UChar * aFence );

static SInt mtlUTF16MaxPrecision( SInt aLength );

extern mtlExtractFuncSet mtlAsciiExtractSet;

extern mtlNextDayFuncSet mtlAsciiNextDaySet;

static mtcName mtlNames[2] = {
    { mtlNames+1, 5, (void*)"UTF16"     },
    { NULL      , 7, (void*)"UTF16BE"   }
};

mtlModule mtlUTF16 = {
    mtlNames,
    MTL_UTF16_ID,
    mtlUTF16NextCharPtr,
    mtlUTF16MaxPrecision,    
    &mtlAsciiExtractSet,
    &mtlAsciiNextDaySet,
    mtl2BYTESpecialCharSet,
    2
};

mtlNCRet mtlUTF16NextCharPtr( UChar ** aSource, UChar * aFence )
{
/***********************************************************************
 *
 * Description : Next Char Pointer
 *
 * Implementation :
 *    다음 문자 위치로 Index 이동
 *
 ***********************************************************************/    
    mtlNCRet sRet;

    if( aFence - *aSource > 1 )
    {
        sRet = NC_VALID;
        (*aSource) += 2;

    }
    else
    {
        sRet = NC_MB_INCOMPLETED;
        *aSource = aFence;
    }

    return sRet;
}

static SInt mtlUTF16MaxPrecision( SInt aLength )
{
/***********************************************************************
 *
 * Description : 문자갯수(aLength)의 UTF16의 최대 precision 계산
 *
 * Implementation :
 *
 *    인자로 받은 aLength에
 *    UTF16 한문자의 최대 크기를 곱한 값을 리턴함.
 *
 *    aLength는 문자갯수의 의미가 있음.
 *
 ***********************************************************************/
    
    return aLength * MTL_UTF16_PRECISION;
}

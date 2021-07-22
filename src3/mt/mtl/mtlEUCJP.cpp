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
 * $Id:$
 **********************************************************************/

#include <mte.h>
#include <mtl.h>
#include <mtdTypes.h>

// ECU-JP Code Set 0 : 1 byte, ASCII or JIS Roman
#define mtlCodeSet0_1BYTE(c)        ( (c)<=0x7E )

// ECU-JP Code Set 1 : 2 byte, JIS X0208:1997
#define mtlCodeSet1_First_BYTE(c)   ( ( (c)>=0xA1 ) && ( (c)<=0xFE ) ) 
#define mtlCodeSet1_Second_BYTE(c)   ( ( (c)>=0xA1 ) && ( (c)<=0xFE ) ) 

// ECU-JP Code Set 2 : 2 byte, Half Width Katakana
#define mtlCodeSet2_First_BYTE(c)   ( (c)==0x8E )
#define mtlCodeSet2_Second_BYTE(c)   ( ( (c)>=0xA1 ) && ( (c)<=0xDF ) )

// ECU-JP Code Set 3 : 3 byte, JIS X0212:1990
#define mtlCodeSet3_First_BYTE(c)   ( (c)==0x8F )
#define mtlCodeSet3_Second_BYTE(c)   ( ( (c)>=0xA1 ) && ( (c)<=0xFE ) )
#define mtlCodeSet3_Third_BYTE(c)   ( ( (c)>=0xA1 ) && ( (c)<=0xFE ) )

extern mtlModule mtlEUCJP;

extern UChar * mtl1BYTESpecialCharSet[];

// PROJ-1755
static mtlNCRet mtlEUCJPNextChar( UChar ** aSource, UChar * aFence );

static SInt mtlEUCJPMaxPrecision( SInt aLength );

extern mtlExtractFuncSet mtlAsciiExtractSet;

extern mtlNextDayFuncSet mtlAsciiNextDaySet;

static mtcName mtlNames[3] = {
    { mtlNames+1, 5, (void*)"EUCJP"  },
    { mtlNames+2, 6, (void*)"EUC-JP" },
    { NULL      , 6, (void*)"EUC_JP" }
};

mtlModule mtlEUCJP = {
    mtlNames,
    MTL_EUCJP_ID,
    mtlEUCJPNextChar,
    mtlEUCJPMaxPrecision,    
    &mtlAsciiExtractSet,
    &mtlAsciiNextDaySet,
    mtl1BYTESpecialCharSet,
    1
};

mtlNCRet mtlEUCJPNextChar( UChar ** aSource, UChar * aFence )
{
/***********************************************************************
 *
 * Description : PROJ-1755 Next Char 최적화
 *
 * Implementation :
 *    다음 문자 위치로 pointer 이동
 *
 ***********************************************************************/

    mtlNCRet sRet;
    
    if( mtlCodeSet0_1BYTE( *(*aSource) ) )
    {
        sRet = NC_VALID;
        (*aSource)++;
    }
    else if( mtlCodeSet1_First_BYTE( *(*aSource) ) )
    {
        if( aFence - *aSource > 1 )
        {
            if( mtlCodeSet1_Second_BYTE( *(*aSource+1) ) )
            {
                sRet = NC_VALID;
                (*aSource) += 2;
            }
            else
            {
                sRet = NC_MB_INVALID;
                (*aSource) += 2;
            }
        }
        else
        {
            sRet = NC_MB_INCOMPLETED;
            *aSource = aFence;
        }
    }
    else if( mtlCodeSet2_First_BYTE( *(*aSource) ) )
    {
        if( aFence - *aSource > 1 )
        {
            if( mtlCodeSet2_Second_BYTE( *(*aSource+1) ) )
            {
                sRet = NC_VALID;
                (*aSource) += 2;
            }
            else
            {
                sRet = NC_MB_INVALID;
                (*aSource) += 2;
            }
        }
        else
        {
            sRet = NC_MB_INCOMPLETED;
            *aSource = aFence;
        }
    }
    else if( mtlCodeSet3_First_BYTE( *(*aSource) ) )
    {
        if( aFence - *aSource > 2 )
        {
            if( (mtlCodeSet3_Second_BYTE( *(*aSource+1) ) == ID_FALSE) ||

                (mtlCodeSet3_Third_BYTE( *(*aSource+2) ) == ID_FALSE) )
            {
                sRet = NC_MB_INVALID;
                (*aSource) += 3;
            }
            else
            {
                sRet = NC_VALID;
                (*aSource) += 3;
            }
        }
        else
        {
            sRet = NC_MB_INCOMPLETED;
            *aSource = aFence;
        }
    }
    else
    {
        sRet = NC_INVALID;
        (*aSource)++;
    }

    return sRet;
}

static SInt mtlEUCJPMaxPrecision( SInt aLength )
{
/***********************************************************************
 *
 * Description : 문자갯수(aLength)의 ShiftJIS의 최대 precision 계산
 *
 * Implementation :
 *
 *    인자로 받은 aLength에
 *    EUCJP 한문자의 최대 크기를 곱한 값을 리턴함.
 *
 *    aLength는 문자갯수의 의미가 있음.
 *
 ***********************************************************************/
    
    return aLength * MTL_EUCJP_PRECISION;
}

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
 * $Id: mtlGB231280.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mte.h>
#include <mtl.h>
#include <mtdTypes.h>

#define mtlGB231280_1BYTE_TYPE(c)        ( (c)<=0x7F )

#define mtlGB231280_2BYTE_TYPE(c)        ( (c)>=0x81 && (c)<=0xFE )

#define mtlGB231280_MULTI_BYTE_TAIL(c)   ( ( (c)>=0x40 && (c)<=0x7E ) || \
                                         ( (c)>=0x80 && (c)<=0xFE ) )

extern mtlModule mtlGB231280;

extern UChar * mtl1BYTESpecialCharSet[];

// PROJ-1755
extern mtlNCRet mtlGB231280NextChar( UChar ** aSource, UChar * aFence );

static SInt mtlGB231280MaxPrecision( SInt aLength );

extern mtlExtractFuncSet mtlAsciiExtractSet;

extern mtlNextDayFuncSet mtlAsciiNextDaySet;

static mtcName mtlNames[3] = {
    { mtlNames+1, 8, (void*)"GB231280"       },
    { mtlNames+2,14, (void*)"ZHS16CGB231280" },
    { NULL      , 7, (void*)"CHINESE"        }
};

mtlModule mtlGB231280 = {
    mtlNames,
    MTL_GB231280_ID,
    mtlGB231280NextChar,
    mtlGB231280MaxPrecision,    
    &mtlAsciiExtractSet,
    &mtlAsciiNextDaySet,
    mtl1BYTESpecialCharSet,
    1
};

mtlNCRet mtlGB231280NextChar( UChar ** aSource, UChar * aFence )
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
    
    if ( mtlGB231280_1BYTE_TYPE( *(*aSource) ) )
    {
        sRet = NC_VALID;
        (*aSource)++;
    }
    else if ( mtlGB231280_2BYTE_TYPE( *(*aSource) ) )
    {
        if( aFence - *aSource > 2 )
        {
            if( mtlGB231280_MULTI_BYTE_TAIL( *(*aSource+1) ) )
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
    else
    {
        sRet = NC_INVALID;
        (*aSource)++;
    }

    return sRet;
}

static SInt mtlGB231280MaxPrecision( SInt aLength )
{
/***********************************************************************
 *
 * Description : 문자갯수(aLength)의 GB231280의 최대 precision 계산
 *
 * Implementation :
 *
 *    인자로 받은 aLength에
 *    GB231280 한문자의 최대 크기를 곱한 값을 리턴함.
 *
 *    aLength는 문자갯수의 의미가 있음.
 *
 ***********************************************************************/
    
    return aLength * MTL_GB231280_PRECISION;
}

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
 * $Id$
 **********************************************************************/

#include <mte.h>
#include <mtl.h>
#include <mtdTypes.h>

/* PROJ-2590 [기능성] CP932 database character set 지원 */

#define mtlMS932_1BYTE_TYPE(c)        ( ( (c) <= 0x7F ) || \
                                        ( ( (c) >= 0xA1 ) && ( (c) <= 0xDF ) ) )

#define mtlMS932_2BYTE_TYPE(c)        ( ( ( (c) >= 0x81 ) && ( (c) <= 0x84 ) ) || \
                                        ( ( (c) >= 0x87 ) && ( (c) <= 0x9F ) ) || \
                                        ( ( (c) >= 0xE0 ) && ( (c) <= 0xEA ) ) || \
                                        ( ( (c) >= 0xED ) && ( (c) <= 0xEE ) ) || \
                                        ( ( (c) >= 0xFA ) && ( (c) <= 0xFC ) ) )

#define mtlMS932_MULTI_BYTE_TAIL(c)   ( ( ( (c) >= 0x40 ) && ( (c) <= 0x7E ) ) || \
                                        ( ( (c) >= 0x80 ) && ( (c) <= 0xFC ) ) )

extern mtlModule mtlMS932;

extern UChar * mtl1BYTESpecialCharSet[];

/* PROJ-1755 */
extern mtlNCRet mtlMS932NextChar( UChar ** aSource, UChar * aFence );

static SInt mtlMS932MaxPrecision( SInt aLength );

extern mtlExtractFuncSet mtlAsciiExtractSet;

extern mtlNextDayFuncSet mtlAsciiNextDaySet;

static mtcName mtlNames[3] = {
    { mtlNames+1, 5, (void*)"MS932" },
    { mtlNames+2, 5, (void*)"CP932" },
    { NULL, 10, (void*)"WINDOWS932" }
};

mtlModule mtlMS932 = {
    mtlNames,
    MTL_MS932_ID,
    mtlMS932NextChar,
    mtlMS932MaxPrecision,
    &mtlAsciiExtractSet,
    &mtlAsciiNextDaySet,
    mtl1BYTESpecialCharSet,
    1
};

mtlNCRet mtlMS932NextChar( UChar ** aSource, UChar * aFence )
{
/***********************************************************************
 *
 * Description : PROJ-2590 [기능성] CP932 database character set 지원
 *
 * Implementation :
 *    다음 문자 위치로 pointer 이동
 *
 ***********************************************************************/
    mtlNCRet sRet;

    if ( mtlMS932_1BYTE_TYPE( *(*aSource) ) )
    {
        sRet = NC_VALID;
        (*aSource)++;
    }
    else
    {
        if ( mtlMS932_2BYTE_TYPE( *(*aSource) ) )
        {
            if ( ( aFence - *aSource ) > 1 )
            {
                if ( mtlMS932_MULTI_BYTE_TAIL( *(*aSource+1) ) )
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
    }

    return sRet;
}

static SInt mtlMS932MaxPrecision( SInt aLength )
{
/***********************************************************************
 *
 * Description : 문자갯수(aLength)의 MS932의 최대 precision 계산
 *
 * Implementation :
 *
 *    인자로 받은 aLength에
 *    MS932 한문자의 최대 크기를 곱한 값을 리턴함.
 *
 *    aLength는 문자갯수의 의미가 있음.
 *
 ***********************************************************************/
    return aLength * MTL_MS932_PRECISION;
}

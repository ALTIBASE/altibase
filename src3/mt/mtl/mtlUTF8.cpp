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
 * $Id: mtlUTF8.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mte.h>
#include <mtl.h>
#include <mtdTypes.h>

#define mtlUTF8_1BYTE_TYPE(c)       ( (c)<=0x7F )

#define mtlUTF8_2BYTE_TYPE(c)       ( (c)>=0xC0 && (c)<=0xDF )

#define mtlUTF8_3BYTE_TYPE(c)       ( (c)>=0xE0 && (c)<=0xEF )

#define mtlUTF8_4BYTE_TYPE(c)       ( (c)>=0xF0 && (c)<=0xFF )

#define mtlUTF8_MULTI_BYTE_TAIL(c)  ( (c)>=0x80 && (c)<=0xBF )

extern mtlModule mtlUTF8;

extern UChar* mtl1BYTESpecialCharSet[];

// PROJ-1755
static mtlNCRet mtlUTF8NextChar( UChar ** aSource, UChar * aFence );

// BUG-45608 JDBC 4Byte char length
extern mtlNCRet mtlUTF8NextCharClobForClient( UChar ** aSource, UChar * aFence );

static SInt mtlUTF8MaxPrecision( SInt aLength );

extern mtlExtractFuncSet mtlAsciiExtractSet;

extern mtlNextDayFuncSet mtlAsciiNextDaySet;

static mtcName mtlNames[1] = {
    { NULL, 4, (void*)"UTF8"     }
};

mtlModule mtlUTF8 = {
    mtlNames,
    MTL_UTF8_ID,
    mtlUTF8NextChar,
    mtlUTF8MaxPrecision,    
    &mtlAsciiExtractSet,
    &mtlAsciiNextDaySet,
    mtl1BYTESpecialCharSet,
    1
};

mtlNCRet mtlUTF8NextChar( UChar ** aSource, UChar * aFence )
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
    
    if( mtlUTF8_1BYTE_TYPE( *(*aSource) ) )
    {
        sRet = NC_VALID;
        (*aSource)++;
    }
    else if( mtlUTF8_2BYTE_TYPE( *(*aSource) ) )
    {
        if( aFence - *aSource > 1 )
        {
            if( mtlUTF8_MULTI_BYTE_TAIL( *(*aSource+1) ) )
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
    else if( mtlUTF8_3BYTE_TYPE( *(*aSource) ) )
    {
        if( aFence - *aSource > 2 )
        {
            if( mtlUTF8_MULTI_BYTE_TAIL( *(*aSource+1) ) &&
                mtlUTF8_MULTI_BYTE_TAIL( *(*aSource+2) ) )
            {
                sRet = NC_VALID;
                (*aSource) += 3;
            }
            else
            {
                sRet = NC_MB_INVALID;
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

static SInt mtlUTF8MaxPrecision( SInt aLength )
{
/***********************************************************************
 *
 * Description : 문자갯수(aLength)의 UTF8의 최대 precision 계산
 *
 * Implementation :
 *
 *    인자로 받은 aLength에
 *    UTF8 한문자의 최대 크기를 곱한 값을 리턴함.
 *
 *    aLength는 문자갯수의 의미가 있음.
 *
 ***********************************************************************/
    
    return aLength * MTL_UTF8_PRECISION;
}

mtlNCRet mtlUTF8NextCharClobForClient( UChar ** aSource, UChar * aFence )
{
/***********************************************************************
 *
 * Description : BUG-45608 JDBC 4Byte char length
 *
 * Implementation :
 *    다음 문자 위치로 pointer 이동
 *    JDBC CLOB의 경우 4바이트 문자에대한 length (1) return 하도록 추가.
 *
 ***********************************************************************/    

    mtlNCRet sRet;
    
    if( mtlUTF8_1BYTE_TYPE( *(*aSource) ) )
    {
        sRet = NC_VALID;
        (*aSource)++;
    }
    else if( mtlUTF8_2BYTE_TYPE( *(*aSource) ) )
    {
        if( aFence - *aSource > 1 )
        {
            if( mtlUTF8_MULTI_BYTE_TAIL( *(*aSource+1) ) )
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
    else if( mtlUTF8_3BYTE_TYPE( *(*aSource) ) )
    {
        if( aFence - *aSource > 2 )
        {
            if( mtlUTF8_MULTI_BYTE_TAIL( *(*aSource+1) ) &&
                mtlUTF8_MULTI_BYTE_TAIL( *(*aSource+2) ) )
            {
                sRet = NC_VALID;
                (*aSource) += 3;
            }
            else
            {
                sRet = NC_MB_INVALID;
                (*aSource) += 3;
            }
        }
        else
        {
            sRet = NC_MB_INCOMPLETED;
            *aSource = aFence;
        }
    }
    else if( mtlUTF8_4BYTE_TYPE( *(*aSource) ) )
    {
        if( aFence - *aSource > 3 )
        {
            if( mtlUTF8_MULTI_BYTE_TAIL( *(*aSource+1) ) &&
                mtlUTF8_MULTI_BYTE_TAIL( *(*aSource+2) ) &&
                mtlUTF8_MULTI_BYTE_TAIL( *(*aSource+3) ))
            {
                sRet = NC_VALID;
                (*aSource) += 4;
            }
            else
            {
                sRet = NC_MB_INVALID;
                (*aSource) += 4;
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


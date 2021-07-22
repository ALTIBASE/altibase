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
 * $Id: mtlMS949.cpp 32401 2009-04-29 06:01:36Z mhjeong $
 **********************************************************************/

#include <mtce.h>
#include <mtcl.h>
#include <mtcdTypes.h>

//----------------------------------
// MS949 Encoding
//----------------------------------

#define mtlMS949_1BYTE_TYPE(c)       ( (c)<=0x7F )

#define mtlMS949_2BYTE_TYPE(c)       ( (c)>=0x81 && (c)<=0xFE ) 

#define mtlMS949_MULTI_BYTE_TAIL(c)  ( ( (c)>=0x41 && (c)<=0x5A ) ||    \
                                       ( (c)>=0x61 && (c)<=0x7A ) ||    \
                                       ( (c)>=0x81 && (c)<=0xFE ) )

extern mtlModule mtclMS949;
extern acp_uint8_t* mtcl1BYTESpecialCharSet[];

// PROJ-1755
static mtlNCRet mtlMS949NextChar( acp_uint8_t ** aSource, acp_uint8_t * aFence );

static acp_sint32_t mtlMS949MaxPrecision( acp_sint32_t aLength );

extern mtlExtractFuncSet mtclKSC5601ExtractSet;

extern mtlNextDayFuncSet mtclKSC5601NextDaySet;

static mtcName mtlNames[3] = {
    { mtlNames+1,  5, (void*)"MS949"      },
    { mtlNames+2,  5, (void*)"CP949"      },
    { NULL      , 10, (void*)"WINDOWS949" }
};

mtlModule mtclMS949 = {
    mtlNames,
    MTL_MS949_ID,
    mtlMS949NextChar,
    mtlMS949MaxPrecision,
    &mtclKSC5601ExtractSet,
    &mtclKSC5601NextDaySet,
    mtcl1BYTESpecialCharSet,
    1
};

mtlNCRet mtlMS949NextChar( acp_uint8_t ** aSource, acp_uint8_t * aFence )
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
   
    if ( mtlMS949_1BYTE_TYPE( *(*aSource) ) )
    {
        sRet = NC_VALID;
        (*aSource)++;
    }
    else if ( mtlMS949_2BYTE_TYPE( *(*aSource) ) )
    {
        if( aFence - *aSource > 1 )
        {
            if( mtlMS949_MULTI_BYTE_TAIL( *(*aSource+1) ) )
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

static acp_sint32_t mtlMS949MaxPrecision( acp_sint32_t aLength )
{
/***********************************************************************
 *
 * Description : 문자갯수(aLength)의 MS949의 최대 precision 계산
 *
 * Implementation :
 *
 *    인자로 받은 aLength에
 *    MS949 한문자의 최대 크기를 곱한 값을 리턴함.
 *
 *    aLength는 문자갯수의 의미가 있음.
 *
 ***********************************************************************/
    
    return aLength * MTL_MS949_PRECISION;
}

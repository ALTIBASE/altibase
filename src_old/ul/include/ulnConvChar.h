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

#ifndef _O_ULN_CONV_CHAR_H_
#define _O_ULN_CONV_CHAR_H_ 1

acp_uint32_t ulnConvDumpAsChar(acp_uint8_t  *aDstBuffer,
                               acp_uint32_t  aDstSize,
                               acp_uint8_t  *aSrcBuffer,
                               acp_uint32_t  aSrcLength);

#define ULN_CONV_BIGINT_BUFFER_MAX 24

/* PROJ-2160 CM 타입제거
   acpSnprintf 의 Integer -> char 의 형변환은 느리다.
   별도의 함수를 작성하여 형변환 속도를 향상시킨다. */
ACP_INLINE acp_char_t* ulnItoA (acp_sint64_t aValue, acp_char_t *aBuffer)
{
    acp_sint32_t  sNegative = 0;
    acp_char_t   *sCurrent;
    acp_uint64_t  sValue;

    if( aValue < 0 )
    {
        sNegative = 1;
        sValue = -aValue;
    }
    else
    {
        sValue = aValue;
    }

    sCurrent      = aBuffer + ULN_CONV_BIGINT_BUFFER_MAX -1;
    *sCurrent     = '\0';

    do
    {
        --sCurrent;
        *sCurrent = '0' + (sValue % 10);
        sValue /= 10;
    } while( sValue != 0 );

    if( sNegative == 1 )
    {
        --sCurrent;
        *sCurrent = '-';
    }
    else
    {
        // nothing todo
    }

    return sCurrent;
}

#endif /* _O_ULN_CONV_CHAR_H_ */


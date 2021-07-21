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

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnConv.h>
#include <ulnConvChar.h>

static const acp_char_t ulnConvByteToCharTableHIGH[256] =
{
    '0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0',
    '1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1',
    '2','2','2','2','2','2','2','2','2','2','2','2','2','2','2','2',
    '3','3','3','3','3','3','3','3','3','3','3','3','3','3','3','3',
    '4','4','4','4','4','4','4','4','4','4','4','4','4','4','4','4',
    '5','5','5','5','5','5','5','5','5','5','5','5','5','5','5','5',
    '6','6','6','6','6','6','6','6','6','6','6','6','6','6','6','6',
    '7','7','7','7','7','7','7','7','7','7','7','7','7','7','7','7',
    '8','8','8','8','8','8','8','8','8','8','8','8','8','8','8','8',
    '9','9','9','9','9','9','9','9','9','9','9','9','9','9','9','9',
    'A','A','A','A','A','A','A','A','A','A','A','A','A','A','A','A',
    'B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B',
    'C','C','C','C','C','C','C','C','C','C','C','C','C','C','C','C',
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
    'E','E','E','E','E','E','E','E','E','E','E','E','E','E','E','E',
    'F','F','F','F','F','F','F','F','F','F','F','F','F','F','F','F'
};

static const acp_char_t ulnConvByteToCharTableLOW[256] =
{
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
};

/*
 * aDstBuffer 에 aSrcBuffer 를 char 로 덤프한다.
 * null termination 을 위한 공간에는 char 를 "쓰지" 않는다.
 * 리턴값은 "쓰여진" char 의 갯수이다.
 * 호출자는 aDstBuffer + 리턴값 에 null termination 을 한다.
 */

acp_uint32_t ulnConvDumpAsChar(acp_uint8_t *aDstBuffer, acp_uint32_t aDstSize, acp_uint8_t *aSrcBuffer, acp_uint32_t aSrcLength)
{
    acp_uint32_t i;
    acp_uint32_t sLengthToConvert;
    acp_uint32_t sDstPosition;

    if(aDstSize >= aSrcLength * 2 + 1)
    {
        sLengthToConvert = aSrcLength;
    }
    else
    {
        if(aDstSize == 0)
        {
            sLengthToConvert = 0;
        }
        else
        {
            sLengthToConvert = (aDstSize - 1) / 2;
        }
    }

    for(i = 0, sDstPosition = 0; i < sLengthToConvert; i++)
    {
        aDstBuffer[sDstPosition] = (acp_uint8_t)ulnConvByteToCharTableHIGH[*(aSrcBuffer + i)];
        sDstPosition++;
        aDstBuffer[sDstPosition] = (acp_uint8_t)ulnConvByteToCharTableLOW[*(aSrcBuffer + i)];
        sDstPosition++;
    }

    return sDstPosition;
}


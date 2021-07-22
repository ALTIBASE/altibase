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
 
/*******************************************************************************
 * $Id: aclCodeUTF8.c 4486 2009-02-10 07:37:20Z jykim $
 ******************************************************************************/

#include <aclCode.h>

/*
 * Index into the table below with the first byte of a UTF-8 sequence to
 * get the number of trailing bytes that are supposed to follow it.
 */
static const acp_uint8_t /*@unused@*/ gAclCodeTrailingBytesForUTF8[256] =
{
/*  0 1 2 3 4 5 6 7 8 9 A B C D E F */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0000 0000 ~ 0000 1111 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0001 0000 ~ 0001 1111 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0010 0000 ~ 0010 1111 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0011 0000 ~ 0011 1111 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0100 0000 ~ 0100 1111 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0101 0000 ~ 0101 1111 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0110 0000 ~ 0110 1111 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0111 0000 ~ 0111 1111 */
    9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, /* 1000 0000 ~ 1000 1111 Illegal! */
    9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, /* 1001 0000 ~ 1001 1111 Illegal! */
    9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, /* 1010 0000 ~ 1010 1111 Illegal! */
    9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, /* 1011 0000 ~ 1011 1111 Illegal! */
    9,9,                             /* 1100 0000 ~ 1100 0001 Illegal! */
        1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 1100 0010 ~ 1100 1111 */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 1101 0000 ~ 1101 1111 */
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* 1110 0000 ~ 1110 1111 */
    3,3,3,3,3,3,3,3,                 /* 1111 0000 ~ 1111 0111 */
                    4,4,4,4,         /* 1111 1000 ~ 1111 1011 */
                            5,5,     /* 1111 1100 ~ 1111 1101 */
                                9,9  /* 1111 1110 ~ 1111 1111 Illegal! */
};


ACP_EXPORT acp_rc_t aclCodeUTF8ToUint32
    (
        const acp_uint8_t *aData,
        const acp_uint32_t aLength,
        acp_uint32_t *aResult,
        acp_uint32_t *aPosition
    )
{
    static const acp_uint8_t sMask[6] =
    {
        0xFF,   /* 0000 0000 -> 1111 1111 - 1 Byte  */
        0x1F,   /* 110x xxxx -> 0001 1111 - 2 Bytes */
        0x0F,   /* 1110 xxxx -> 0000 1111 - 3 Bytes */
        0x07,   /* 1111 0xxx -> 0000 0111 - 4 Bytes */
        0x03,   /* 1111 10xx -> 0000 0011 - 5 Bytes */
        0x01    /* 1111 110x -> 0000 0001 - 6 Bytes */
    };
    static const acp_uint32_t sMinValue[6] =
    {
        0x0,         /* 1 Byte  UTF-8 value must be larger than U+0 */
        0x80,        /* 2 Bytes UTF-8 value must be larger than U+80h */
        0x800,       /* 3 Bytes UTF-8 value must be larger than U+800h */
        0x10000,     /* 4 Bytes UTF-8 value must be larger than U+10000h */
        0x200000,    /* 5 Bytes UTF-8 value must be larger than U+200000h */
        0x4000000    /* 6 Bytes UTF-8 value must be larger than U+4000000h */
    };

    acp_uint32_t sRet;
    acp_uint32_t sExtraBytesToRead = gAclCodeTrailingBytesForUTF8[*aData];
    acp_uint32_t i;
    acp_uint8_t sCh;

    if(9 == sExtraBytesToRead)
    {
        /* Invalid UTF-8 String */
        return ACP_RC_ENOTSUP;
    }
    else
    {
        /* Do Nothing. fall through */
    }

    if(sExtraBytesToRead > aLength)
    {
        /* The needed UTF-8 array exceeds buffer size */
        return ACP_RC_ETRUNC;
    }
    else
    {
        /* Do Nothing. fall through */
    }

    /* First byte with mask */
    sRet = aData[0] & sMask[sExtraBytesToRead];

    /* Trailing bytes */
    for(i=1; i<=sExtraBytesToRead; i++)
    {
        sCh = aData[i];
        /* Trailing bytes must be in the form 10xx xxxx */
        if((0x80 /* 10xx xxxx */) != (sCh & 0xC0 /* 11xx xxxx */))
        {
            /* An illegal UTF-8 String! */
            return ACP_RC_ENOTSUP;
        }
        else
        {
            /* Insert xx xxxx part in the trailing byte 10xx xxxx */
            sRet = (sRet << 6) | (sCh & 0x3F);
        }
    }

    /* Ensure that the byte array was not overlonged */
    if(sRet < sMinValue[sExtraBytesToRead])
    {
        /* A badly encoded UTF-8 string! */
        return ACP_RC_ENOTSUP;
    }
    else
    {
        if(
            0xFFFF == sRet ||
            0xFFFE == sRet ||
            (0xD800 <= sRet && sRet <= 0xDFFF)
            )
        {
            /* These values should not appear in UTF-8 encoding */
            return ACP_RC_ENOTSUP;
        }
        else
        {
            /* Return values completed */
            *aResult = sRet;
            *aPosition = sExtraBytesToRead + 1;

            return ACP_RC_SUCCESS;
        }
    }
}

ACP_EXPORT acp_rc_t aclCodeUint32ToUTF8
    (
        acp_uint8_t *aData,
        const acp_uint32_t aLength,
        const acp_uint32_t aValue,
        acp_uint32_t *aPosition
    )
{
    /* Maximum values for n-Bytes UTF-8 byte array */
    static const acp_uint32_t sLength[6] =
    {
        0x80,           /* 1 Byte  */
        0x800,          /* 2 Bytes */
        0x10000,        /* 3 Bytes */
        0x200000,       /* 4 Bytes */
        0x4000000,      /* 5 Bytes */
        0x80000000      /* 6 Bytes */
    };

    /*
     * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
     * into the first byte, depending on how many bytes follow.  There are
     * as many entries in this table as there are UTF-8 sequence types.
     */
    static const acp_uint8_t sMask[6] =
    {
        0x00,   /* 0000 0000 - 1 Byte  */
        0xC0,   /* 110x xxxx - 2 Bytes */
        0xE0,   /* 1110 xxxx - 3 Bytes */
        0xF0,   /* 1111 0xxx - 4 Bytes */
        0xF8,   /* 1111 10xx - 5 Bytes */
        0xFC    /* 1111 110x - 6 Bytes */
    };

    acp_uint32_t sCount;
    acp_uint32_t sCh = aValue;
    acp_uint32_t i;

    if(0xFFFF == sCh || 0xFFFE == sCh || (0xD800 <= sCh && sCh <= 0xDFFF))
    {
        /* These values should not appear in UTF-8 encoding */
        return ACP_RC_ENOTSUP;
    }
    else
    {
        /* Do nothing. Fall through */
    }

    /* Get Bytes to store */
    for(i=0, sCount = 0; i<6; i++)
    {
        if(sCh < sLength[i])
        {
            sCount = i + 1;
            break;
        }
        else
        {
            /* continue; */
        }
    }

    if(0 == sCount)
    {
        /* cannot convert values larger than 0x7FFFFFFF to UTF-8 byte array */
        return ACP_RC_ENOTSUP;
    }
    else
    {
        /* Do nothing. Fall through */
    }

    if(sCount > aLength)
    {
        /* The needed UTF-8 array exceeds buffer size */
        return ACP_RC_ETRUNC;
    }
    else
    {
        /* Do nothing. Fall through */
    }

    /* Process Trailing Bytes */
    for(i=sCount - 1; i>0; i--)
    {
        aData[i] = (acp_uint8_t)(0x80 | (sCh & 0x3f));
        sCh >>= 6;
    }

    /* First byte with mask */
    aData[0] = sCh | sMask[sCount - 1];

    /* We're done */
    *aPosition = sCount;

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t aclCodeGetCharUTF8(
    acp_std_file_t* aFile,
    acp_qchar_t* aChar)
{
    acp_byte_t sChars[9];
    acp_rc_t sRC;
    sRC = acpStdGetByte(aFile, &(sChars[0]));

    if(ACP_RC_IS_SUCCESS(sRC))
    {
        /* ACP good to read, sir. :) */
        acp_uint32_t sToRead = gAclCodeTrailingBytesForUTF8[sChars[0]];
        if(9 == sToRead)
        {
            /* Invalid first byte to decode */
            /* Return read byte anyway */
            *aChar = (acp_qchar_t)sChars[0];
            return ACP_RC_ENOTSUP;
        }
        else
        {
            /* Read trailing bytes */
            acp_uint32_t i;

            for(i = 1; i <= sToRead; i++)
            {
                sRC = acpStdGetByte(aFile, &(sChars[i]));

                if(ACP_RC_IS_SUCCESS(sRC))
                {
                    /* Do nothing. We're going well */
                }
                else
                {
                    return sRC;
                }
            }

            /* Decode */
            return aclCodeUTF8ToUint32(sChars, 9, aChar, &i);
        }
    }
    else
    {
        /* Return error code */
        return sRC;
    }
}

ACP_EXPORT acp_rc_t aclCodePutCharUTF8(
    acp_std_file_t* aFile,
    acp_qchar_t aChar)
{
    acp_uint32_t sLength;
    acp_size_t sDummy;
    acp_rc_t sRC;
    acp_byte_t sChars[9];

    /* Encode to UTF8 */
    sRC = aclCodeUint32ToUTF8(sChars, 9, aChar, &sLength);

    if(ACP_RC_IS_SUCCESS(sRC))
    {
        /* Right! Write! */
        return acpStdWrite(aFile, sChars, 1, sLength, &sDummy);
    }
    else
    {
        /* Not supported character */
        return sRC;
    }
}

ACP_EXPORT acp_rc_t aclCodeGetStringUTF8(
    acp_std_file_t* aFile,
    acp_qchar_t* aStr,
    acp_size_t aMaxLen,
    acp_size_t* aRead)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;
    *aRead = 0;

    while(
        (*aRead) < aMaxLen &&
        ACP_RC_IS_SUCCESS(sRC = aclCodeGetCharUTF8(aFile, aStr))
        )
    {
        aStr++;
        (*aRead)++;
    }

    return sRC;
}

ACP_EXPORT acp_rc_t aclCodePutStringUTF8(
    acp_std_file_t* aFile,
    acp_qchar_t* aStr,
    acp_size_t aMaxLen,
    acp_size_t* aWritten)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;
    *aWritten = 0;

    while(
        (*aWritten) < aMaxLen &&
        ACP_RC_IS_SUCCESS(sRC = aclCodePutCharUTF8(aFile, *aStr))
        )
    {
        aStr++;
        (*aWritten)++;
    }

    return sRC;
}

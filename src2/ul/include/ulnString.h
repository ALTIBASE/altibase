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

#ifndef _O_ULN_STRING_H_
#define _O_ULN_STRING_H_ 1

ACP_EXTERN_C_BEGIN

ACP_INLINE ACI_RC ulnStrCpyToCStr( acp_char_t *aDest,
                                   acp_size_t  aDestSize,
                                   acp_str_t  *aSrc )
{
    acp_char_t *sStrBuf;
    acp_size_t  sStrLen = acpStrGetLength(aSrc);
    ACI_RC      sRC;

    if (sStrLen > 0)
    {   
        sStrBuf = acpStrGetBuffer(aSrc);
        if ( acpCStrCpy(aDest, aDestSize, sStrBuf, sStrLen) == ACP_RC_SUCCESS )
        {   
            sRC = ACI_SUCCESS;
        }   
        else
        {
            sRC = ACI_FAILURE;
        }
    }   
    else
    {
        aDest[0] = '\0';
        sRC = ACI_SUCCESS;
    }   

    return sRC;
}

ACP_INLINE acp_bool_t ulnCStrIsNullOrEmpty( acp_char_t *aStr, acp_sint32_t aStrLen )
{
    return (aStr == NULL || aStrLen <= 0 || aStr[0] == '\0');
}

acp_char_t* ulnStrSep(acp_char_t **aStringp, const acp_char_t *aDelim);

acp_char_t* ulnStrRchr(acp_char_t *aString, acp_sint32_t aChar);
acp_char_t* ulnStrChr(acp_char_t *aString, acp_sint32_t aChar);

ACP_EXTERN_C_END

#endif /* _O_ULN_STRING_H_ */


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
 * $Id: aciVa.c 11151 2010-05-31 06:28:13Z gurugio $
 **********************************************************************/


#include <acpCStr.h>
#include <acpPath.h>
#include <acpFile.h>
#include <aceAssert.h>
#include <acpPrintf.h>

#include <aciVa.h>


ACP_EXPORT
const acp_char_t *aciVaBasename (const acp_char_t *pathname)
{
    acp_sint32_t temp = -1;
    acp_rc_t sRC;

    sRC = acpCStrFindCStr((acp_char_t *)pathname,
                          ACI_DIRECTORY_SEPARATOR_STR_A,
                          &temp,
                          (acp_sint32_t)acpCStrLen(pathname,
                                                   ACP_PATH_MAX_LENGTH),
                          ACP_CSTR_CASE_SENSITIVE | ACP_CSTR_SEARCH_BACKWARD);
    
    if (sRC == ACP_RC_ENOENT || temp < 0)
        return pathname;
    else
        return &pathname[temp + 1];
}



ACP_EXPORT
acp_sint32_t aciVaAppendFormat(acp_char_t *aBuffer, acp_size_t aBufferSize, const acp_char_t *aFormat, ...)
{
    acp_size_t  sLen;
    acp_size_t sWritten;
    acp_rc_t    sResult;
    va_list ap;

    sLen = acpCStrLen(aBuffer, aBufferSize);
    /*
      BUG-28379 [SD] sdnbBTree::dumpNodeHdr( UChar *aPage ) 내에서 local Array의
      ptr를 반환하고 있습니다.
      aBufferSize가 sLen보다 커야합니다. 그렇지 않으면 이후 vsnprintf함수 호출시
      음수값이 넘어가고, 그 음수값이 unsigned로 해석되면서 메모리를 긁을 수 
      있습니다.
    */
    ACE_ASSERT( aBufferSize > sLen );

    va_start(ap, aFormat);
    sResult = acpVsnprintf(aBuffer + sLen, aBufferSize - sLen, aFormat, ap);
    va_end(ap);

    if (ACP_RC_IS_SUCCESS(sResult))
    {
        sWritten = acpCStrLen(aBuffer + sLen, aBufferSize);
        return sWritten + sLen;
    }
    else
    {
        return -1;
    }
}


ACP_EXPORT
acp_bool_t aciVaFdeof(acp_file_t *aFile)
{
    acp_offset_t sEndPos;
    acp_offset_t sCurPos;
    acp_stat_t sStat;
    acp_bool_t   sResult;

    acpFileSeek(aFile,
                0,
                ACP_SEEK_CUR,
                &sCurPos);
    acpFileStat(aFile, &sStat);
    sEndPos = sStat.mSize;
    
    if(sEndPos == sCurPos)
    {
        sResult = ACP_TRUE;
    }
    else
    {
        sResult = ACP_FALSE;
    }

    return sResult;

}

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
 * $Id: aclCode.h 3964 2008-12-18 02:31:18Z jykim $
 ******************************************************************************/
#if !defined(ACL_UTF8_H)
#define ACL_UTF8_H

#include <acpTypes.h>
#include <acpError.h>
#include <acpStd.h>

ACP_EXTERN_C_BEGIN

/**
 * convert UTF-8 byte array to 32bit unsigned integer
 * @param aData points UTF-8 byte array
 * @param aLength length of aData
 * @param aResult points 32bit unsigned integer to store result
 * @param aPosition points 32bit unsigned integer to store the bytes decoded
 * @return ACP_RC_SUCCESS when done,
 * @return ACP_RC_ENOTSUP for Invalid UTF-8 vaule,
 * @return ACP_RC_TRUNC when aLength is not long enough
 */
ACP_EXPORT acp_rc_t aclCodeUTF8ToUint32
    (
        const acp_uint8_t *aData,
        const acp_uint32_t aLength,
        acp_uint32_t *aResult,
        acp_uint32_t *aPosition
    );

/**
 * convert a 32bit unsigned integer to UTF-8 byte array
 * @param aData points UTF-8 byte array
 * @param aLength length of aData
 * @param aValue a 32bit unsigned integer to convert
 * @param aPosition points 32bit unsigned integer to store the bytes encoded
 * @return ACP_RC_SUCCESS when done,
 * @return ACP_RC_ENOTSUP for Invalid UTF-8 String,
 * @return ACP_RC_TRUNC when aLength is not long enough
 */
ACP_EXPORT acp_rc_t aclCodeUint32ToUTF8
    (
        acp_uint8_t *aData,
        const acp_uint32_t aLength,
        const acp_uint32_t aValue,
        acp_uint32_t *aPosition
    );

ACP_EXPORT acp_rc_t aclCodeGetCharUTF8(
    acp_std_file_t* aFile,
    acp_qchar_t* aChar);

ACP_EXPORT acp_rc_t aclCodePutCharUTF8(
    acp_std_file_t* aFile,
    acp_qchar_t aChar);

ACP_EXPORT acp_rc_t aclCodeGetStringUTF8(
    acp_std_file_t* aFile,
    acp_qchar_t* aStr,
    acp_size_t aMaxLen,
    acp_size_t* aRead);

ACP_EXPORT acp_rc_t aclCodePutStringUTF8(
    acp_std_file_t* aFile,
    acp_qchar_t* aStr,
    acp_size_t aMaxLen,
    acp_size_t* aWritten);

ACP_EXTERN_C_END

#endif /*ACL_UTF8_H*/

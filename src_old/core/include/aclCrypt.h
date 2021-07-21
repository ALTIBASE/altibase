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
 * $Id:
 ******************************************************************************/

#if !defined(_O_ACL_CRYPT_H_)
#define _O_ACL_CRYPT_H_

/**
 * @dir core/src/acl
 *
 * Altibase Core Library Enciphering & Deciphering Library
 */

#include <acp.h>

ACP_EXTERN_C_BEGIN 

/**
 * Encipher using TEA algorithm.
 * @param aPlain a plain text to cipher
 * @param aKey a key to use while ciphering
 * @param aCipher pointer to buffer to store ciphered text
 * @param aTextLength size of aPlain and aCipher in bytes.
 * must be a multiple of 8
 * @param aKeyLength size of aKey in bytes. must be a multiple of 16.
 * @return ACP_RC_SUCCESS with proper aTextLength and aKeyLength value.
 * ACP_RC_ENOTSUP when aTextLength is not a multiple of 8, aKeyLength is
 * 0 or not a multiple of 16
 */
ACP_EXPORT acp_rc_t aclCryptTEAEncipher(void* aPlain,
                             void* aKey,
                             void* aCipher,
                             acp_size_t aTextLength,
                             acp_size_t aKeyLength);

/**
 * Decipher using TEA algorithm.
 * @param aCipher a ciphered text to decipher
 * @param aKey a key to use while deciphering
 * @param aPlain pointer to buffer to store deciphered plaintext
 * @param aTextLength size of aPlain and aCipher in bytes.
 * must be a multiple of 8
 * @param aKeyLength size of aKey in bytes. must be a multiple of 16.
 * @return ACP_RC_SUCCESS with proper aTextLength and aKeyLength value.
 * ACP_RC_ENOTSUP when aTextLength is not a multiple of 8, aKeyLength is
 * 0 or not a multiple of 16
 */
ACP_EXPORT acp_rc_t aclCryptTEADecipher(void* aCipher,
                             void* aKey,
                             void* aPlain,
                             acp_size_t aTextLength,
                             acp_size_t aKeyLength);

ACP_EXTERN_C_END

#endif

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

#ifndef _O_ULN_CATALOG_FUNCTIONS_H_
#define _O_ULN_CATALOG_FUNCTIONS_H_ 1

ACP_EXTERN_C_BEGIN

#define ULN_CATALOG_MAX_NAME_LEN          (128 + 3) // considered quoted identifiers
#define ULN_CATALOG_MAX_TYPE_LEN          (256 + 1) // BUG-17785
#define ULN_CATALOG_QUERY_STR_BUF_SIZE    (8192)    // BUG-17681, over 2048 --> BUG-40293, over 4096

/*
 * Catalog Functions :
 *      ulnTables()
 *      ulnColumns()
 *      ulnStatistics()
 *      ulnPrimaryKeys()
 */

// bug-25905: conn nls not applied to client lang module
/*
 * BUGBUG
 */
ACI_RC ulnMakeNullTermNameInSQL(mtlModule        *aMtlModule,
                                acp_char_t       *aDestBuffer,
                                acp_uint32_t      aDestLen,
                                const acp_char_t *aSrcBuffer,
                                acp_sint32_t      aSrcLen,
                                const acp_char_t *aDefaultString );

ACI_RC ulnCheckStringLength(acp_sint32_t aLength, acp_sint32_t aMaxLength);

// bug-25905: conn nls not applied to client lang module
acp_sint32_t ulnAppendFormatParameter(ulnFnContext     *aFnContext,
                                      acp_char_t       *aBuffer,
                                      acp_sint32_t      aBufferSize,
                                      acp_sint32_t      aBufferPos,
                                      const acp_char_t *aFormat,
                                      const acp_char_t *aName,
                                      acp_sint16_t      aLength);

ACP_EXTERN_C_END

#endif /* _O_ULN_CATALOG_FUNCTIONS_H_ */


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
 * $Id: aciVarString.h 11319 2010-06-23 02:39:42Z djin $
 **********************************************************************/
/*
 * Description :
 *     캐릭터 셋 변환 모듈
 *
 **********************************************************************/

#ifndef _O_ACIVARSTRING_H_
#define _O_ACIVARSTRING_H_ 1

#include <aciTypes.h>
#include <aciConvCharSet.h>


#include <acp.h>
#include <acl.h>

ACP_EXTERN_C_BEGIN

typedef struct aciVarString
{
    acl_mem_pool_t *mPiecePool;
    acp_char_t     *mBuffer;
    acp_list_t      mPieceList;
    acp_uint32_t    mPieceSize;
    acp_uint32_t    mLength;

} aci_var_string_t;

typedef struct aciVarStringPiece
{
   acp_list_node_t  mPieceListNode;
   acp_uint32_t mLength;
   acp_char_t   mData[1];

} aci_var_string_piece_t;


ACP_EXPORT ace_rc_t aciVarStringInitialize(aci_var_string_t *aString,
                                           acl_mem_pool_t   *aPool,
                                           acp_uint32_t     aPieceSize);

ACP_EXPORT ace_rc_t aciVarStringFinalize(aci_var_string_t  *aString);

ACP_EXPORT acp_uint32_t aciVarStringGetLength(aci_var_string_t  *aString);

ACP_EXPORT ace_rc_t aciVarStringTruncate(aci_var_string_t  *aString, acp_bool_t aReserveFlag);

ACP_EXPORT ace_rc_t aciVarStringPrint(aci_var_string_t  *aString, const acp_char_t *aCString);
ACP_EXPORT ace_rc_t aciVarStringPrintLength(aci_var_string_t  *aString, const acp_char_t *aCString, acp_uint32_t aLength);
ACP_EXPORT ace_rc_t aciVarStringPrintFormat(aci_var_string_t  *aString, const acp_char_t *aFormat, ...);
ACP_EXPORT ace_rc_t aciVarStringPrintFormatV(aci_var_string_t  *aString, const acp_char_t *aFormat, va_list aArg);

ACP_EXPORT ace_rc_t aciVarStringAppend(aci_var_string_t  *aString, const acp_char_t *aCString);
ACP_EXPORT ace_rc_t aciVarStringAppendLength(aci_var_string_t  *aString, const acp_char_t *aCString, acp_uint32_t aLength);
ACP_EXPORT ace_rc_t aciVarStringAppendFormat(aci_var_string_t  *aString, const acp_char_t *aFormat, ...);
ACP_EXPORT ace_rc_t aciVarStringAppendFormatV(aci_var_string_t *aString, const acp_char_t *aFormat, va_list aArg);

ACP_EXTERN_C_END

#endif /* _O_ACIVARSTRING_H_ */
 

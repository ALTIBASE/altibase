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

#ifndef _O_ULN_DATA_H_
#define _O_ULN_DATA_H_ 1

#include <uln.h>
#include <ulnCache.h>
#include <ulnBindCommon.h>

/* PROJ-1789 Updatable Scrollable Cursor */
void ulnDataBuildColumnZero( ulnFnContext *aFnContext,
                             ulnRow       *aRow,
                             ulnColumn    *aColumn );

/* PROJ-2160 CM 타입제거
   엔디안에 맞추어서 저장한다. */
ACI_RC ulnDataBuildColumnFromMT(ulnFnContext *aFnContext,
                                acp_uint8_t  *aSrc,
                                ulnColumn    *aColumn);

/* PROJ-2616 */
ACI_RC ulnCopyToUserBufferForSimpleQuery(ulnFnContext     *aFnContext,
                                         ulnStmt          *aKeysetStmt,
                                         acp_uint8_t      *aSrc,
                                         ulnDescRec       *aDescRecArd,
                                         ulnDescRec       *aDescRecIrd);

void ulnDataWriteStringToUserBuffer(ulnFnContext *aFnContext,
                                    acp_char_t   *aSourceString,
                                    acp_uint32_t  aSourceStringLength,
                                    acp_char_t    *aTargetBuffer,
                                    acp_uint32_t   aTargetBufferSize,
                                    acp_sint16_t  *aSourceStringSizePtr);

/* PROJ-2160 CM 타입제거
   데이타의 길이를 찾아올때 사용된다. */
ACI_RC ulnDataGetNextColumnOffset(ulnColumn    *aColumn,
                                  acp_uint8_t  *aSrc,
                                  acp_uint32_t *aOffset);

#endif /* _O_ULN_DATA_H_ */

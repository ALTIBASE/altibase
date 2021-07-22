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

#ifndef _O_ULSDN_LOB_H_
#define _O_ULSDN_LOB_H_ 1

#include <ulnPrivate.h>
#include <ulnLob.h>

SQLRETURN ulsdnLobPrepare4Write(acp_sint16_t  aHandleType,
                                ulnObject    *aObject,
                                acp_sint16_t  aLocatorCType,
                                acp_uint64_t  aLocator,
                                acp_uint32_t  aFromPosition,
                                acp_uint32_t  aSize);

SQLRETURN ulsdnLobWrite(acp_sint16_t  aHandleType,
                        ulnObject    *aObject,
                        acp_sint16_t  aLocatorCType,
                        acp_uint64_t  aLocator,
                        acp_sint16_t  aSourceCType,
                        void         *aBuffer,
                        acp_uint32_t  aBufferSize);

SQLRETURN ulsdnLobFinishWrite(acp_sint16_t  aHandleType,
                              ulnObject    *aObject,
                              acp_sint16_t  aLocatorCType,
                              acp_uint64_t  aLocator);

#endif  /* _O_ULSDN_LOB_H_ */


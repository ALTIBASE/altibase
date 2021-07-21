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

#ifndef _ULN_BIND_PARAMETER_H_
#define _ULN_BIND_PARAMETER_H_ 1

#include <uln.h>
#include <ulnPrivate.h>

/* PROJ-1721 Name-based Binding  @aParamName¿ª √ﬂ∞° */
ACI_RC ulnBindParamBody(ulnFnContext *aFnContext,
                        acp_uint16_t  aParamNumber,
                        acp_char_t   *aParamName,
                        acp_sint16_t  aInputOutputType,
                        acp_sint16_t  aValueType,
                        acp_sint16_t  aParamType,
                        ulvULen       aColumnSize,
                        acp_sint16_t  aDecimalDigits,
                        void         *aParamValuePtr,
                        ulvSLen       aBufferLength,
                        ulvSLen      *aStrLenOrIndPtr);

ACI_RC ulnBindParamApdPart(ulnFnContext      *aFnContext,
                           ulnDescRec       **aDescRecApd,
                           acp_uint16_t       aParamNumber,
                           acp_sint16_t       aSQL_C_TYPE,
                           ulnParamInOutType  aInputOutputType,
                           void              *aParamValuePtr,
                           ulvSLen            aBufferLength,
                           ulvSLen           *aStrLenOrIndPtr);

#endif /* _ULN_BIND_PARAMETER_H_ */


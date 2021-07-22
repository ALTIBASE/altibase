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

#ifndef _O_ULN_CONVERSION_H_
#define _O_ULN_CONVERSION_H_ 1

#include <ulnDef.h>

/*
 * PROJ-2047 Strengthening LOB - LOBCACHE
 *
 * LOCATOR (8) + SIZE (8) + HASDATA (1)
 *         0  ~ 15              16
 */
#define LOB_MT_SIZE           17
#define LOB_MT_HASDATA_OFFSET 16

typedef struct ulnLengthPair
{
    ulvSLen mWritten;
    ulvSLen mNeeded;
} ulnLengthPair;

acp_bool_t ulncIsValidNumericLiterals(acp_char_t   *aString,
                                      acp_uint32_t  aBufferSize,
                                      acp_sint32_t *aScale);
acp_sint64_t ulncStrToSLong(const acp_char_t  *aPtr,
                            acp_char_t       **aEndptr,
                            acp_sint32_t       aBase);

acp_uint32_t ulnConvCopy(acp_uint8_t *aDstBuffer,
                         acp_uint32_t aDstSize,
                         acp_uint8_t *aSrcBuffer,
                         acp_uint32_t aSrcLength);

ACI_RC ulnConvCopyStr(ulnFnContext  *aFnContext,
                      mtlModule     *aSrcCharSet,
                      mtlModule     *aDestCharSet,
                      ulnAppBuffer  *aAppBuffer,
                      ulnColumn     *aColumn,
                      acp_char_t    *aSourceBuffer,
                      acp_uint32_t   aSourceLength,
                      ulnLengthPair *aLength);

typedef ACI_RC ulnConvFunction(ulnFnContext  *aFnContext,
                               ulnAppBuffer  *aUserBuffer,
                               ulnColumn     *aColumn,
                               ulnLengthPair *aLength,
                               acp_uint16_t   aRowNumber);

ACI_RC ulnConvert(ulnFnContext     *aFnContext,
                  ulnAppBuffer     *aUserBuffer,
                  ulnColumn        *aColumn,
                  acp_uint16_t      aUserRowNumber,
                  ulnIndLenPtrPair *aUserIndLenPair);

ulnConvFunction *ulnConvGetFilter(ulnCTypeID aCTYPE, ulnMTypeID aMTYPE);

ACI_RC ulncNULL(ulnFnContext  *aFnContext,
                ulnAppBuffer  *aAppBuffer,
                ulnColumn     *aColumn,
                ulnLengthPair *aLength,
                acp_uint16_t   aRowNumber);

typedef void ulnConvEndianFunc(acp_uint8_t *aSourceBuffer,
                               ulvSLen      aSourceLength);

ulnConvEndianFunc *ulnConvGetEndianFunc(acp_uint8_t aIsSameEndian);

void ulnConvEndian_NONE(acp_uint8_t *aSourceBuffer,
                        ulvSLen      aSourceLength);

void ulnConvEndian_ADJUST(acp_uint8_t *aSourceBuffer,
                          ulvSLen      aSourceLength);

#endif  /* _O_ULN_CONVERSION_H_ */


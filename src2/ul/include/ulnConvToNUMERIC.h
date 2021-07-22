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

#ifndef _O_ULN_CONV_TO_NUMERIC_H_
#define _O_ULN_CONV_TO_NUMERIC_H_ 1

ACI_RC ulncCHAR_NUMERIC(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber);

ACI_RC ulncVARCHAR_NUMERIC(ulnFnContext  *aFnContext,
                           ulnAppBuffer  *aAppBuffer,
                           ulnColumn     *aColumn,
                           ulnLengthPair *aLength,
                           acp_uint16_t   aRowNumber);

ACI_RC ulncVARBIT_NUMERIC(ulnFnContext  *aFnContext,
                          ulnAppBuffer  *aAppBuffer,
                          ulnColumn     *aColumn,
                          ulnLengthPair *aLength,
                          acp_uint16_t   aRowNumber);

ACI_RC ulncBIT_NUMERIC(ulnFnContext  *aFnContext,
                       ulnAppBuffer  *aAppBuffer,
                       ulnColumn     *aColumn,
                       ulnLengthPair *aLength,
                       acp_uint16_t   aRowNumber);

ACI_RC ulncSMALLINT_NUMERIC(ulnFnContext  *aFnContext,
                            ulnAppBuffer  *aAppBuffer,
                            ulnColumn     *aColumn,
                            ulnLengthPair *aLength,
                            acp_uint16_t   aRowNumber);

ACI_RC ulncINTEGER_NUMERIC(ulnFnContext  *aFnContext,
                           ulnAppBuffer  *aAppBuffer,
                           ulnColumn     *aColumn,
                           ulnLengthPair *aLength,
                           acp_uint16_t   aRowNumber);

ACI_RC ulncBIGINT_NUMERIC(ulnFnContext  *aFnContext,
                          ulnAppBuffer  *aAppBuffer,
                          ulnColumn     *aColumn,
                          ulnLengthPair *aLength,
                          acp_uint16_t   aRowNumber);

ACI_RC ulncREAL_NUMERIC(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber);

ACI_RC ulncDOUBLE_NUMERIC(ulnFnContext  *aFnContext,
                          ulnAppBuffer  *aAppBuffer,
                          ulnColumn     *aColumn,
                          ulnLengthPair *aLength,
                          acp_uint16_t   aRowNumber);

ACI_RC ulncINTERVAL_NUMERIC(ulnFnContext  *aFnContext,
                            ulnAppBuffer  *aAppBuffer,
                            ulnColumn     *aColumn,
                            ulnLengthPair *aLength,
                            acp_uint16_t   aRowNumber);

ACI_RC ulncNUMERIC_NUMERIC(ulnFnContext  *aFnContext,
                           ulnAppBuffer  *aAppBuffer,
                           ulnColumn     *aColumn,
                           ulnLengthPair *aLength,
                           acp_uint16_t   aRowNumber);

ACI_RC ulncNCHAR_NUMERIC(ulnFnContext  *aFnContext,
                         ulnAppBuffer  *aAppBuffer,
                         ulnColumn     *aColumn,
                         ulnLengthPair *aLength,
                         acp_uint16_t   aRowNumber);

ACI_RC ulncNVARCHAR_NUMERIC(ulnFnContext  *aFnContext,
                            ulnAppBuffer  *aAppBuffer,
                            ulnColumn     *aColumn,
                            ulnLengthPair *aLength,
                            acp_uint16_t   aRowNumber);

#endif /* _O_ULN_CONV_TO_NUMERIC_H_ */


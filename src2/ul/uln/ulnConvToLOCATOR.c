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

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnConv.h>
#include <ulnConvToLOCATOR.h>

ACI_RC ulncCLOB_LOCATOR(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    ulnLob *sLob;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aRowNumber);

    sLob = (ulnLob *)aColumn->mBuffer;
    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    ULN_GET_LOB_LOCATOR_VALUE((acp_uint64_t *)aAppBuffer->mBuffer,&(sLob->mLocatorID));

    aLength->mWritten = ACI_SIZEOF(acp_uint64_t);
    aLength->mNeeded  = ACI_SIZEOF(acp_uint64_t);

    return ACI_SUCCESS;
}

ACI_RC ulncBLOB_LOCATOR(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    return ulncCLOB_LOCATOR(aFnContext,
                            aAppBuffer,
                            aColumn,
                            aLength,
                            aRowNumber);
}




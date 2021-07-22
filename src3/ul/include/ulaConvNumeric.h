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
 
#ifndef _O_MMC_CONV_NUMERIC_H_
#define _O_MMC_CONV_NUMERIC_H_ 1

#include <acp.h>
#include <acl.h>
#include <ace.h>

#include <aciTypes.h>

#define ULA_CONV_NUMERIC_BUFFER_SIZE 20 /* MMC_CONV_NUMERIC_BUFFER_SIZE */

typedef struct ulaConvNumeric ulaConvNumeric;

typedef enum
{
    ULA_BYTEORDER_BIG_ENDIAN = 0,
    ULA_BYTEORDER_LITTLE_ENDIAN,

    ULA_BYTEORDER_MAX
} ulaConvByteOrder;

typedef struct ulaConvNumericOp
{
    acp_uint8_t *(*mGetBuffer)(ulaConvNumeric *aNumeric);
    ACI_RC       (*mShiftLeft)(ulaConvNumeric *aNumeric);
    ACI_RC       (*mShiftRight)(ulaConvNumeric *aNumeric);
    ACI_RC       (*mAdd)(ulaConvNumeric *aNumeric, acp_uint32_t aValue);
    ACI_RC       (*mMultiply)(ulaConvNumeric *aNumeric, acp_uint32_t aValue);
    ACI_RC       (*mConvert)(ulaConvNumeric *aSrc, ulaConvNumeric *aDst);
} ulaConvNumericOp;


struct ulaConvNumeric
{
    ulaConvNumericOp  mOpAll[ULA_BYTEORDER_MAX];

    acp_uint8_t      *mBuffer;
    acp_uint32_t      mAllocSize;
    acp_uint32_t      mSize;

    acp_uint32_t      mBase;
    ulaConvByteOrder  mByteOrder;

    ulaConvNumericOp *mOp;
};


/*
 * -----------------------------------------------------------------------------
 *  Functions
 * -----------------------------------------------------------------------------
 */
void ulaConvNumericInitialize(ulaConvNumeric   *aNumeric,
                              acp_uint8_t      *aBuffer,
                              acp_uint8_t       aAllocSize,
                              acp_uint8_t       aSize,
                              acp_uint32_t      aBase,
                              ulaConvByteOrder  aByteOrder);

acp_uint8_t *ulaConvNumericGetBuffer(ulaConvNumeric *aNumeric);
acp_uint32_t ulaConvNumericGetSize(ulaConvNumeric *aNumeric);

ACI_RC ulaConvNumericShiftLeft(ulaConvNumeric *aNumeric);
ACI_RC ulaConvNumericShiftRight(ulaConvNumeric *aNumeric);

ACI_RC ulaConvNumericAdd(ulaConvNumeric *aNumeric, acp_uint32_t aValue);
ACI_RC ulaConvNumericMultiply(ulaConvNumeric *aNumeric, acp_uint32_t aValue);

ACI_RC ulaConvNumericConvert(ulaConvNumeric *aSrc, ulaConvNumeric *aDst);

#endif


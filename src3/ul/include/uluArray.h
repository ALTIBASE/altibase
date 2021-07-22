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

#ifndef _O_ULU_ARRAY_H_
#define _O_ULU_ARRAY_H_ 1

#include <acp.h>
#include <acl.h>

#include <uluMemory.h>

/*
 * +----------------------+
 * | uluArray::mBlockList |
 * +---|------------------+    uluArray::mElementSize
 *     |                    |<->|
 * +-uluArrayBlock----------|---|---+---+----------------------------------+ -------
 * | mList | ....       |   |   |   |   | . . .                            |     ^
 * +---|--------------------+---+---+---+----------------------------------+     |
 *     |                                                                         |
 *     |                                                             uluArray::mBlockCount
 *     |                                                                         |
 * +---|--------------------+---+---+---+----------------------------------+     |
 * | mList | ....       |   |   |   |   | . . .                            |     |
 * +---|--------------------+---+---+---+----------------------------------+     |
 *     |                                                                         |
 * +---|--------------------+---+---+---+----------------------------------+     |
 * | mList | ....       |   |   |   |   | . . .                            |     V
 * +--------------------|---+---+---+---+----------------------------------| -------
 *                      |                                                  |
 *                      |<----- uluArray::NumberOfElementsPerBlock ------->|
 */

typedef enum
{
    ULU_ARRAY_IGNORE,
    ULU_ARRAY_NEW
} uluArrayAllocNew;

typedef struct uluArray uluArray;

typedef ACI_RC uluArrayInitFunc(uluArray *aArray, void *mArgument);

struct uluArray
{
    uluMemory        *mMemory;
    acp_uint32_t      mElementSize;
    acp_uint32_t      mNumberOfElementsPerBlock;

    acp_uint32_t      mDummyAlign;
    acp_uint32_t      mBlockCount;

    acp_list_t        mBlockList;
    uluArrayInitFunc *mInitializeElements;
};

ACI_RC uluArrayCreate(uluMemory        *aMemory,
                      uluArray        **aArray,
                      acp_uint32_t      aElementSize,
                      acp_uint32_t      aElementsPerBlock,
                      uluArrayInitFunc *aInitFunc);

ACI_RC uluArrayGetElement(uluArray          *aArray,
                          acp_uint32_t       aIndex,
                          void             **aElementOut,
                          uluArrayAllocNew   aAllocNewBlock);

void   uluArrayInitializeContent(uluArray *aArray);

ACI_RC uluArrayReserve(uluArray *aArray, acp_uint32_t aCount);

void   uluArrayInitializeToInitial(uluArray *aArray);

#endif /* _O_ULU_ARRAY_H_ */


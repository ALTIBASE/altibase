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

#ifndef _ULP_MEMORY_H_
#define _ULP_MEMORY_H_ 1

#include <idl.h>


typedef struct ulpMemoryHeader {
    ulpMemoryHeader *mNext;
    UInt             mLen;
    void            *mBuf;
} ulpMemoryHeader;

class ulpMemory
{

    public:
        ulpMemory();

        void* ulpAlloc( UInt aBufSize );
        void  ulpFree ();

    private:
        ulpMemoryHeader *mHead;
};

#endif

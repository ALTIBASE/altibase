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
 
#ifndef _O_IDSRC4_H_
#define _O_IDSRC4_H_

#include <idl.h>

typedef struct RC4Context
{
    UInt        x;
    UInt        y;
    UChar       state[256];
} RC4Context;


class idsRC4
{
private:
    UInt rc4Byte(RC4Context *aCtx);

public:
    void setKey(RC4Context *aCtx, const UChar *aKey, UInt aKeyLen);

    void convert(RC4Context *aCtx, const UChar *aSrc,
                 UInt aSrcLen, UChar *aDest);
};

#endif /* _O_IDSRC4_H_ */

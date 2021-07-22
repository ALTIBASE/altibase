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
 
#ifndef _O_IDSDES_H_
#define _O_IDSDES_H_

#include <idl.h>

#define IDS_ENCODE  0
#define IDS_DECODE  1

class idsDES {

public:
    idsDES()    {};

    void    setkey( UChar *key, SInt edflag);
    void    des ( UChar *in, UChar *out);
    void    tripleDes (UChar *in, UChar *out, UChar *key, SInt edflag);

    void    ascii2bin ( SChar *in, SChar *out, SInt size);
    void    bin2ascii ( SChar *in, SChar *out, SInt size);

    // To fix BUG-14496
    // for CBC(Cipher Block Chaning)
    void    blockXOR( UChar *aTarget, UChar *aSource );

private:
    void    pack8 ( UChar *packed, UChar *binary);
    void    unpack8 ( UChar *packed, UChar *binary);

    /* Key schedule of 16 48-bit subkeys generated from 64-bit key */
    UChar   KS[16][48];
};

#endif /* _O_IDSDES_H_ */

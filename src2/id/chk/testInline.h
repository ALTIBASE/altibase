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
 
#ifndef _testInline_H_
#define _testInline_H_

typedef struct chkLSN
{
    UInt value1;
    UInt value2;
} chkLSN;

class testInline
{
public:
    testInline(){};
    ~testInline(){};
    static inline idBool isLT(chkLSN *a, chkLSN *b);
};

inline idBool testInline::isLT(chkLSN *a, chkLSN *b)
{
    if(a->value1 < b->value1)
    {
        return ID_TRUE;
    }
    else if(a->value1 == b->value1)
    {
        if(a->value2 < b->value2)
        {
            return ID_TRUE;
        }
    }
            
    return ID_FALSE;
}

# ifdef ENDIAN_IS_BIG_ENDIAN
#  define CHK_SCN_HIGH_WORD 0
#  define CHK_SCN_LOW_WORD  1
# else
#  define CHK_SCN_HIGH_WORD 1
#  define CHK_SCN_LOW_WORD  0
# endif /* ENDIAN_IS_BIG_ENDIAN */

# define CHK_SET_SCN( dest, src ) {                     \
    UChar * pDest = (UChar *)dest;                      \
    UChar * pSrc  = (UChar *)src;                       \
    SInt    sTemp;                                      \
    for(sTemp=0; sTemp<sizeof(ULong); sTemp++){         \
        pDest[sTemp] = pSrc[sTemp];}                    \
}

#endif




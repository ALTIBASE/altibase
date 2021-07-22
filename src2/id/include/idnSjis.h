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
 
/***********************************************************************
 * $Id: qdbCommon.cpp 25810 2008-04-30 00:20:56Z peh $
 **********************************************************************/

#ifndef _O_IDNSJIS_H_
#define  _O_IDNSJIS_H_  1

#include <idnConv.h>

/*
 * SHIFT_JIS
 */

/*
   Conversion between SJIS codes (s1,s2) and JISX0208 codes (c1,c2):
   Example. (s1,s2) = 0x8140, (c1,c2) = 0x2121.
   0x81 <= s1 <= 0x9F || 0xE0 <= s1 <= 0xEA,
   0x40 <= s2 <= 0x7E || 0x80 <= s2 <= 0xFC,
   0x21 <= c1 <= 0x74, 0x21 <= c2 <= 0x7E.
   Invariant:
     94*2*(s1 < 0xE0 ? s1-0x81 : s1-0xC1) + (s2 < 0x80 ? s2-0x40 : s2-0x41)
     = 94*(c1-0x21)+(c2-0x21)
   Conversion (s1,s2) -> (c1,c2):
     t1 := (s1 < 0xE0 ? s1-0x81 : s1-0xC1)
     t2 := (s2 < 0x80 ? s2-0x40 : s2-0x41)
     c1 := 2*t1 + (t2 < 0x5E ? 0 : 1) + 0x21
     c2 := (t2 < 0x5E ? t2 : t2-0x5E) + 0x21
   Conversion (c1,c2) -> (s1,s2):
     t1 := (c1 - 0x21) >> 1
     t2 := ((c1 - 0x21) & 1) * 0x5E + (c2 - 0x21)
     s1 := (t1 < 0x1F ? t1+0x81 : t1+0xC1)
     s2 := (t2 < 0x3F ? t2+0x40 : t2+0x41)
 */

SInt convertMbToWc4Sjis( void    * aSrc,
                         SInt      aSrcRemain,
                         void    * aDest,
                         SInt      aDestRemain );

SInt convertWcToMb4Sjis( void    * aSrc,
                         SInt      aSrcRemain,
                         void    * aDest,
                         SInt      aDestRemain );

#endif /* _O_IDNSJIS_H_ */
 

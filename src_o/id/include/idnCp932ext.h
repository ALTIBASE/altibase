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
 * $Id$
 **********************************************************************/

#ifndef _O_IDNCP932EXT_H_
#define  _O_IDNCP932EXT_H_ 1

#include <idnConv.h>

/* PROJ-2590 [기능성] CP932 database character set 지원 */
#define IDN_CP932EXT_XOR_VALUE ( 23739 )

/* PROJ-2590 [기능성] CP932 database character set 지원 */
SInt convertMbToWc4Cp932ext( void * aSrc,
                             SInt   aSrcRemain,
                             void * aDest,
                             SInt   aDestRemain );

/* PROJ-2590 [기능성] CP932 database character set 지원 */
SInt convertWcToMb4Cp932ext( void * aSrc,
                             SInt   aSrcRemain,
                             void * aDest,
                             SInt   aDestRemain );

#endif /* _O_IDNCP932EXT_H_ */

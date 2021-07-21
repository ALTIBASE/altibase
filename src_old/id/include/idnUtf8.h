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
 * $Id: idnUtf8.h 26208 2008-05-28 04:52:11Z copyrei $
 **********************************************************************/

#ifndef _O_IDNUTF8_H_
#define _O_IDNUTF8_H_ 1

#include <idnConv.h>

/*
 * UTF-8
 */

/* Specification: RFC 3629 */

SInt convertMbToWc4Utf8( void    * aSrc,
                         SInt      aSrcRemain,
                         void    * aDest,
                         SInt      aDestRemain );

SInt convertWcToMb4Utf8( void    * aSrc,
                         SInt      aSrcRemain,
                         void    * aDest,
                         SInt      aDestRemain );

#endif /* _O_IDNUTF8_H_ */
 

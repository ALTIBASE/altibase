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

#ifndef _O_IDNJISX0201_H_
#define  _O_IDNJISX0201_H_  1

#include <idnConv.h>

/*
 * JISX0201.1976-0
 */

SInt convertMbToWc4Jisx0201( void   * aSrc,
                             SInt     aSrcRemain,
                             void   * aDest,
                             SInt     aDestRemain );

SInt convertWcToMb4Jisx0201( void   * aSrc,
                             SInt     aSrcRemain,
                             void   * aDest,
                             SInt     aDestRemain );

#endif /* _O_IDNJISX0201_H_ */
 

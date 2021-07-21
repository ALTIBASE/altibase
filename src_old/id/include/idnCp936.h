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
 * $Id: idnCp936.h 65528 2014-06-16 07:10:52Z myungsub.shin $
 **********************************************************************/

#ifndef _O_IDNCP936_H_
#define  _O_IDNCP936_H_  1

#include <idnConv.h>

/* PROJ-2414 [기능성] GBK, CP936 character set 추가 */
SInt convertMbToWc4Cp936( void    * aSrc,
                          SInt      aSrcRemain,
                          void    * aDest,
                          SInt      aDestRemain );

/* PROJ-2414 [기능성] GBK, CP936 character set 추가 */
SInt convertWcToMb4Cp936( void    * aSrc,
                          SInt      aSrcRemain,
                          void    * aDest,
                          SInt      aDestRemain );

#endif /* _O_IDNCP936_H_ */

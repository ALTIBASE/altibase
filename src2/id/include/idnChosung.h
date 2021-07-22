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
 * $Id: idnChosung.h 80575 2017-07-21 07:06:35Z yoonhee.kim $
 **********************************************************************/

#ifndef _O_IDN_CHOSUNG_H_
# define  _O_IDN_CHOSUNG_H_  1

# include <idTypes.h>

UShort idnGetWansungCode   ( UChar  aByte, UInt * aFirst );
void   idnExtractChosung   ( UShort aWansungCode, UChar ** aOutBuf );

#endif /* _O_IDN_CHOSUNG_H_ */


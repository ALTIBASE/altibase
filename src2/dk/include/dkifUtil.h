/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $id$
 **********************************************************************/

#ifndef _O_DKIF_UTIL_H_
#define _O_DKIF_UTIL_H_ 1

extern IDE_RC dkifUtilConvertMtdCharToCString( mtdCharType * aValue,
                                               SChar ** aCString );
extern IDE_RC dkifUtilFreeCString( SChar * aCString );

extern IDE_RC dkifUtilCopyDblinkName( mtdCharType * aValue,
                                      SChar * aDblinkName );

extern IDE_RC dkifUtilCheckNodeFlag( ULong aFlag, UInt aArgumentCount );
extern IDE_RC dkifUtilCheckNullColumn( mtcStack * aStack, UInt aIndex );

extern IDE_RC dkifUtilGetIntegerValueFromNthArgument(
    mtcTemplate * aTemplate,
    mtcNode * aNode,
    SInt aArgumentNumber,
    SInt * aValue );

#endif /* _O_DKIF_UTIL_H_ */

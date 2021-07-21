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
 * $Id: $
 **********************************************************************/

#ifndef _O_STK_H_
# define _O_STK_H_ 1

#include <mtcDef.h>

class stk {
 private:

 public:

    static SInt RelationDefault( const mtcColumn* aColumn1,
                                 const void*      aRow1,
                                 UInt             aFlag1,
                                 const mtcColumn* aColumn2,
                                 const void*      aRow2,
                                 UInt             aFlag2,
                                 const void*      aInfo );

    static IDE_RC rangeCallBack( idBool     * aResult,
                                 const void * aRow,
                                 void       *, /* aDirectKey */
                                 UInt        , /* aDirectKeyPartialSize */
                                 const scGRID aGRID,
                                 void       * aData );

    static IDE_RC mergeAndRange( smiRange* aMerged,
                                 smiRange* aRange1,
                                 smiRange* aRange2 );

    static IDE_RC mergeOrRangeList( smiRange  * aMerged,
                                    smiRange ** aRangeListArray,
                                    SInt        aRangeCount );
};

#endif /* _O_STK_H_ */

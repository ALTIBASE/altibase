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
 * $Id:
 **********************************************************************/

#ifndef _O_MTL_COLLATE_H_
# define _O_MTL_COLLATE_H_ 1

#include <mtcDef.h>
#include <mtdTypes.h>

class mtlCollate {
private:

    static SInt mtlCompareMS949Collate( const UChar  * aValue1,
                                        UInt           aIterator1,
                                        UShort       * aLength1,
                                        const UChar  * aValue2,
                                        UInt           aIterator2,
                                        UShort       * aLength2 );

    static SInt mtlCompareMS949CollateWithCheck2ByteCharCut( const UChar  * aValue1,
                                                             UShort         aLength1,
                                                             UInt           aIterator1,
                                                             UShort       * aOneCharLength1,
                                                             const UChar  * aValue2,
                                                             UShort         aLength2,
                                                             UInt           aIterator2,
                                                             UShort       * aOneCharLength2,
                                                             idBool       * aIs2ByteCharCut ); /* BUG-43106 */

public:

    //-----------------------------------------
    // US7ASCII, KSC5601, MS949 언어에 대한
    // 코드와 길이를 구하는 함수
    //-----------------------------------------

    static void mtlSetCodeAndLenInfo( const UChar * aSource,
                                      UInt          aSourceIdx,
                                      UShort      * aCode,
                                      UShort      * aLength );

    static void mtlSetCodeAndLenInfoWithCheck2ByteCharCut( const UChar * aSource,
                                                           UShort        aSourceLen,
                                                           UInt          aSourceIdx,
                                                           UShort      * aCode,
                                                           UShort      * aLength,
                                                           idBool      * aIs2ByteCharCut ); /* BUG-43106 */

    //-----------------------------------------
    // MS949와 KSC5601 언어의 초성 포함 정렬
    //-----------------------------------------

    // CHAR 
    static SInt mtlCharMS949collateMtdMtdAsc(
        mtdValueInfo * aValueInfo1,
        mtdValueInfo * aValueInfo2 );

    static SInt mtlCharMS949collateMtdMtdDesc(
        mtdValueInfo * aValueInfo1,
        mtdValueInfo * aValueInfo2 );

    static SInt mtlCharMS949collateStoredMtdAsc(
        mtdValueInfo * aValueInfo1,
        mtdValueInfo * aValueInfo2 );

    static SInt mtlCharMS949collateStoredMtdDesc(
        mtdValueInfo * aValueInfo1,
        mtdValueInfo * aValueInfo2 );

    static SInt mtlCharMS949collateStoredStoredAsc(
        mtdValueInfo * aValueInfo1,
        mtdValueInfo * aValueInfo2 );

    static SInt mtlCharMS949collateStoredStoredDesc(
        mtdValueInfo * aValueInfo1,
        mtdValueInfo * aValueInfo2 );

    static SInt mtlCharMS949collateIndexKeyMtdAsc( mtdValueInfo * aValueInfo1,
                                                   mtdValueInfo * aValueInfo2 ); /* BUG-43106 */

    static SInt mtlCharMS949collateIndexKeyMtdDesc( mtdValueInfo * aValueInfo1,
                                                    mtdValueInfo * aValueInfo2 ); /* BUG-43106 */

    // VARCHAR 
    static SInt mtlVarcharMS949collateMtdMtdAsc(
        mtdValueInfo * aValueInfo1,
        mtdValueInfo * aValueInfo2 );
    
    static SInt mtlVarcharMS949collateMtdMtdDesc(
        mtdValueInfo * aValueInfo1,
        mtdValueInfo * aValueInfo2 );

    static SInt mtlVarcharMS949collateStoredMtdAsc(
        mtdValueInfo * aValueInfo1,
        mtdValueInfo * aValueInfo2 );

    static SInt mtlVarcharMS949collateStoredMtdDesc(
        mtdValueInfo * aValueInfo1,
        mtdValueInfo * aValueInfo2 );

    static SInt mtlVarcharMS949collateStoredStoredAsc(
        mtdValueInfo * aValueInfo1,
        mtdValueInfo * aValueInfo2 );

    static SInt mtlVarcharMS949collateStoredStoredDesc(
        mtdValueInfo * aValueInfo1,
        mtdValueInfo * aValueInfo2 );

    static SInt mtlVarcharMS949collateIndexKeyMtdAsc( mtdValueInfo * aValueInfo1,
                                                      mtdValueInfo * aValueInfo2 ); /* BUG-43106 */
    
    static SInt mtlVarcharMS949collateIndexKeyMtdDesc( mtdValueInfo * aValueInfo1,
                                                       mtdValueInfo * aValueInfo2 ); /* BUG-43106 */
};

#endif /* _O_MTL_COLLATE_H_ */

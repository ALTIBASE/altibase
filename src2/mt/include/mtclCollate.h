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

#include <mtccDef.h>
#include <mtcdTypes.h>

acp_sint32_t mtlCollateCompareMS949Collate( const acp_uint8_t  * aValue1,
                                            acp_uint32_t         aIterator1,
                                            acp_uint16_t       * aLength1,
                                            const acp_uint8_t  * aValue2,
                                            acp_uint32_t         aIterator2,
                                            acp_uint16_t       * aLength2 );


//-----------------------------------------
// US7ASCII, KSC5601, MS949 언어에 대한
// 코드와 길이를 구하는 함수
//-----------------------------------------

void mtlCollateSetCodeAndLenInfo( const acp_uint8_t  * aSource,
                                  acp_uint32_t         aSourceIdx,
                                  acp_uint16_t       * aCode,
                                  acp_uint16_t       * aLength );

//-----------------------------------------
// MS949와 KSC5601 언어의 초성 포함 정렬
//-----------------------------------------

// CHAR 
acp_sint32_t mtlCollateCharMS949collateMtdMtdAsc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtlCollateCharMS949collateMtdMtdDesc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtlCollateCharMS949collateStoredMtdAsc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtlCollateCharMS949collateStoredMtdDesc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtlCollateCharMS949collateStoredStoredAsc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtlCollateCharMS949collateStoredStoredDesc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

// VARCHAR 
acp_sint32_t mtlCollateVarcharMS949collateMtdMtdAsc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtlCollateVarcharMS949collateMtdMtdDesc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtlCollateVarcharMS949collateStoredMtdAsc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtlCollateVarcharMS949collateStoredMtdDesc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtlCollateVarcharMS949collateStoredStoredAsc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtlCollateVarcharMS949collateStoredStoredDesc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

#endif /* _O_MTL_COLLATE_H_ */

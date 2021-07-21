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
 * $Id: mtd.h 36181 2009-10-21 00:56:27Z sungminee $
 **********************************************************************/

#ifndef _O_MTCD_H_
#define _O_MTCD_H_ 1

#include <mtccDef.h>
#include <mtcdTypes.h>

#define MTD_MAX_DATATYPE_CNT                               (128)

ACI_RC mtdFloat2String( acp_uint8_t    * aBuffer,
                        acp_uint32_t     aBufferLength,
                        acp_uint32_t   * aLength,
                        mtdNumericType * aNumeric );

//----------------------------------------------------------------
// PROJ-1364 해당 value를  bigint type으로 converion하여 compare 
//----------------------------------------------------------------

// 해당 value를  bigint type으로 converion 하는 함수 
void mtdConvertToBigintType4MtdValue( mtdValueInfo  * aValueInfo,
                                      mtdBigintType * aBigintValue );

void mtdConvertToBigintType4StoredValue( mtdValueInfo  * aValueInfo,
                                         mtdBigintType * aBigintValue);

// bigint type으로 conversion하여 compare 하는 함수 
acp_sint32_t mtdCompareNumberGroupBigintMtdMtdAsc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtdCompareNumberGroupBigintMtdMtdDesc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtdCompareNumberGroupBigintStoredMtdAsc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtdCompareNumberGroupBigintStoredMtdDesc( 
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtdCompareNumberGroupBigintStoredStoredAsc( 
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtdCompareNumberGroupBigintStoredStoredDesc( 
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

//----------------------------------------------------------------
// PROJ-1364 해당 value를  double type으로 converion하여 compare 
//---------------------------------------------------------------

// double type으로 conversion하여 compare 하는 함수 
acp_sint32_t mtdCompareNumberGroupDoubleMtdMtdAsc( 
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtdCompareNumberGroupDoubleMtdMtdDesc( 
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtdCompareNumberGroupDoubleStoredMtdAsc( 
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtdCompareNumberGroupDoubleStoredMtdDesc( 
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtdCompareNumberGroupDoubleStoredStoredAsc( 
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtdCompareNumberGroupDoubleStoredStoredDesc( 
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

//----------------------------------------------------------------
// PROJ-1364 해당 value를  numeric type으로 converion하여 compare 
//---------------------------------------------------------------

// 해당 value를 numeric type으로 conversion 하는 함수 
void mtdConvertToNumericType4MtdValue(
    mtdValueInfo    * aValueInfo,
    mtdNumericType ** aNumericValue);

void mtdConvertToNumericType4StoredValue(
    mtdValueInfo    * aValueInfo,
    mtdNumericType ** aNumericValue,
    acp_uint8_t     * aLength,
    acp_uint8_t    ** aSignExponentMantissa);

// numeric type으로 conversion하여 compare 하는 함수 
acp_sint32_t mtdCompareNumberGroupNumericMtdMtdAsc( 
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtdCompareNumberGroupNumericMtdMtdDesc( 
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtdCompareNumberGroupNumericStoredMtdAsc( 
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtdCompareNumberGroupNumericStoredMtdDesc( 
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtdCompareNumberGroupNumericStoredStoredAsc( 
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );

acp_sint32_t mtdCompareNumberGroupNumericStoredStoredDesc( 
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 );


acp_uint32_t mtdGetNumberOfModules( void );

ACI_RC mtdIsTrueNA( acp_bool_t      * aResult,
                    const mtcColumn * aColumn,
                    const void      * aRow,
                    acp_uint32_t      aFlag );

ACI_RC mtdCanonizeDefault( const mtcColumn  *  aCanon,
                           void             ** aCanonized,
                           mtcEncryptInfo   *  aCanonInfo,
                           const mtcColumn  *  aColumn,
                           void             *  aValue,
                           mtcEncryptInfo   *  aColumnInfo,
                           mtcTemplate      *  aTemplate );

ACI_RC mtdInitializeModule( mtdModule    * aModule,
                            acp_uint32_t   aNo );

ACI_RC mtdInitialize( mtdModule    *** aExtTypeModuleGroup,
                      acp_uint32_t     aGroupCnt );

ACI_RC mtdFinalize( void );

// PROJ-1361 : mtdModule과 mtlModue 분리했으므로
//             mtdModue 검색시 language 정보 받지 않음
ACI_RC mtdModuleByName( const mtdModule ** aModule,
                        const void      *  aName,
                        acp_uint32_t       aLength );

ACI_RC mtdModuleById( const mtdModule ** aModule,
                      acp_uint32_t       aId );

ACI_RC mtdModuleByNo( const mtdModule ** aModule,
                      acp_uint32_t       aNo );

acp_bool_t mtdIsNullDefault( const mtcColumn * aColumn,
                             const void      * aRow,
                             acp_uint32_t      aFlag );

ACI_RC mtdEncodeCharDefault( mtcColumn    * aColumn,
                             void         * aValue,
                             acp_uint32_t   aValueSize,
                             acp_uint8_t  * aCompileFmt,
                             acp_uint32_t   aCompileFmtLen,
                             acp_uint8_t  * aText,
                             acp_uint32_t * aTextLen,
                             ACI_RC       * aRet );

ACI_RC mtdEncodeNumericDefault( mtcColumn    * aColumn,
                                void         * aValue,
                                acp_uint32_t   aValueSize,
                                acp_uint8_t  * aCompileFmt,
                                acp_uint32_t   aCompileFmtLen,
                                acp_uint8_t  * aText,
                                acp_uint32_t * aTextLen,
                                ACI_RC       * aRet );

ACI_RC mtdDecodeDefault( mtcTemplate  * aTemplate,
                         mtcColumn    * aColumn,
                         void         * aValue,
                         acp_uint32_t * aValueSize,
                         acp_uint8_t  * aCompileFmt,
                         acp_uint32_t   aCompileFmtLen,
                         acp_uint8_t  * aText,
                         acp_uint32_t   aTextLen,
                         ACI_RC       * aRet );

// To fix BUG-14235
ACI_RC mtdEncodeNA( mtcColumn    * aColumn,
                    void         * aValue,
                    acp_uint32_t   aValueSize,
                    acp_uint8_t  * aCompileFmt,
                    acp_uint32_t   aCompileFmtLen,
                    acp_uint8_t  * aText,
                    acp_uint32_t * aTextLen,
                    ACI_RC     * aRet );

// PROJ-2002 Column Security
ACI_RC mtdDecodeNA( mtcTemplate  * aTemplate,
                    mtcColumn    * aColumn,
                    void         * aValue,
                    acp_uint32_t * aValueSize,
                    acp_uint8_t  * aCompileFmt,
                    acp_uint32_t   aCompileFmtLen,
                    acp_uint8_t  * aText,
                    acp_uint32_t   aTextLen,
                    ACI_RC       * aRet );

ACI_RC mtdCompileFmtDefault( mtcColumn    * aColumn,
                             acp_uint8_t  * aCompiledFmt,
                             acp_uint32_t * aCompiledFmtLen,
                             acp_uint8_t  * aFormatString,
                             acp_uint32_t   aFormatStringLength,
                             ACI_RC       * aRet );

ACI_RC mtdValueFromOracleDefault( mtcColumn    * aColumn,
                                  void         * aValue,
                                  acp_uint32_t * aValueOffset,
                                  acp_uint32_t   aValueSize,
                                  const void   * aOracleValue,
                                  acp_sint32_t   aOracleLength,
                                  ACI_RC       * aResult );

ACI_RC mtdMakeColumnInfoDefault( void         * aStmt,
                                 void         * aTableInfo,
                                 acp_uint32_t   aIdx );

acp_double_t mtdSelectivityNA( void * aColumnMax,
                               void * aColumnMin,
                               void * aValueMax,
                               void * aValueMin );

acp_double_t mtdSelectivityFloat( void * aColumnMax,
                                  void * aColumnMin,
                                  void * aValueMax,
                                  void * aValueMin );

// PROJ-1579 NCHAR
acp_double_t mtdSelectivityDefault( void * aColumnMax,
                                    void * aColumnMin,
                                    void * aValueMax,
                                    void * aValueMin );

const void* mtdValueForModule( const void   * aColumn,
                               const void   * aRow,
                               acp_uint32_t   aFlag,
                               const void   * aDefaultNull );

// 해당 value를  double type으로 converion 하는 함수 
// 인자로 받은 data type을 double 형으로 변환
// 동일컬럼에 대한 통합 selectivity 계산시,
// 비교대상 data type을 double형으로 변환시켜서 selectivity를 구함.
void mtdConvertToDoubleType4MtdValue( mtdValueInfo  * aValueInfo,
                                      mtdDoubleType * aDoubleValue );
    
void mtdConvertToDoubleType4StoredValue( mtdValueInfo  * aValueInfo,
                                         mtdDoubleType * aDoubleValue);
  
// Data Type의 기본 인덱스 타입을 구한다.
acp_uint32_t mtdGetDefaultIndexTypeID( const mtdModule * aModule );

// Data Type에 사용가능한 인덱스인지 판단함.
acp_bool_t mtdIsUsableIndexType( const mtdModule * aModule,
                                 acp_uint32_t      aIndexType );

// PROJ-1558
// MM에서 NULL값을 지정한다.
ACI_RC mtdAssignNullValueById( acp_uint32_t    aId,
                               void         ** aValue,
                               acp_uint32_t  * aSize );

// PROJ-1558
// MM에서 NULL값인지 검사한다.
ACI_RC mtdCheckNullValueById( acp_uint32_t   aId,
                              void         * aValue,
                              acp_bool_t   * aIsNull );

// PROJ-1705
// 저장되지 않는 데이타타입에 대한 처

ACI_RC mtdStoredValue2MtdValueNA( acp_uint32_t   aColumnSize,
                                  void         * aRow,
                                  acp_uint32_t   aOffset,
                                  acp_uint32_t   aLength,
                                  const void   * aValue );
// PROJ-1705
// 저장되지 않는 데이타타입에 대한 처리     
acp_uint32_t mtdNullValueSizeNA();
    
// PROJ-1705
// 저장되지 않는 데이타타입에 대한 처리         
acp_uint32_t mtdHeaderSizeNA();

// PROJ-1705
// length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
// integer와 같은 고정길이 데이타타입은 default로 0 반환
acp_uint32_t mtdHeaderSizeDefault();
    
// PROJ-1579 NCHAR
// META 단계에서 CHAR, VARCHAR, CLOB, CLOBLOCATOR, NCHAR, NVARCHAR 
// 타입에 대해서 language를 다시 세팅해준다.
ACI_RC mtdModifyNls4MtdModule();

// PROJ-1877
// 데이터의 실제 precision, scale 값을 반환한다.
ACI_RC mtdGetPrecisionNA( const mtcColumn * aColumn,
                          const void      * aRow,
                          acp_uint32_t      aFlag,
                          acp_sint32_t    * aPrecision,
                          acp_sint32_t    * aScale );

// PROJ-1872
mtdCompareFunc  mtdFindCompareFunc( mtcColumn    * aColumn1,
                                    mtcColumn    * aColumn2,
                                    acp_uint32_t   aCompValueType,
                                    acp_uint32_t   aDirection );
//};

#endif /* _O_MTCD_H_ */
 

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
 * $Id: mtd.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_MTD_H_
# define _O_MTD_H_ 1

# include <mtc.h>
# include <mtcDef.h>
# include <mtdTypes.h>

#define MTD_MAX_DATATYPE_CNT                               (128)

#define MTD_VALUE_FIXED( aValueInfo ) \
        ((UChar*)(aValueInfo)->value +  \
        ((smiColumn*)(aValueInfo)->column)->offset )

#define MTD_VALUE_OFFSET_USELESS( aValueInfo ) \
        ((aValueInfo)->value)

class mtd {
private:
    static       mtdModule* mAllModule[MTD_MAX_DATATYPE_CNT];
    static const mtdModule* mInternalModule[];

    // PROJ-1872
    static const mtdCompareFunc compareNumberGroupBigintFuncs[MTD_COMPARE_FUNC_MAX_CNT][2];
    static const mtdCompareFunc compareNumberGroupDoubleFuncs[MTD_COMPARE_FUNC_MAX_CNT][2];
    static const mtdCompareFunc compareNumberGroupNumericFuncs[MTD_COMPARE_FUNC_MAX_CNT][2];
    
    // PROJ-1364 숫자형계열의 서로 다른 data type에 대한 비교시
    // 비교대상 기준이 되는 data type을 지정
    static const UInt comparisonNumberType[4][4];

    // PROJ-1346 numeric type을 double type으로 converion
    static void convertNumeric2DoubleType(
                                  UChar            aNumericLength,
                                  UChar          * aSignExponentMantissa,
                                  mtdDoubleType  * aDoubleValue );

    // PROJ-1364  bigint type을 numeric type으로 conversion
    static void convertBigint2NumericType( mtdBigintType  * aBigintValue,
                                           mtdNumericType * aNumericValue );

public:
    
    //----------------------------------------------------------------
    // PROJ-1364 해당 value를  bigint type으로 converion하여 compare 
    //----------------------------------------------------------------
    
    // 해당 value를  bigint type으로 converion 하는 함수 
    static void convertToBigintType4MtdValue( mtdValueInfo  * aValueInfo,
                                              mtdBigintType * aBigintValue );
    
    static void convertToBigintType4StoredValue( mtdValueInfo  * aValueInfo,
                                                 mtdBigintType * aBigintValue);

    // bigint type으로 conversion하여 compare 하는 함수 
    static SInt compareNumberGroupBigintMtdMtdAsc( mtdValueInfo * aValueInfo1,
                                                   mtdValueInfo * aValueInfo2 );

    static SInt compareNumberGroupBigintMtdMtdDesc( mtdValueInfo * aValueInfo1,
                                                    mtdValueInfo * aValueInfo2 );
    
    
    static SInt compareNumberGroupBigintStoredMtdAsc( mtdValueInfo * aValueInfo1,
                                                      mtdValueInfo * aValueInfo2 );

    static SInt compareNumberGroupBigintStoredMtdDesc( mtdValueInfo * aValueInfo1,
                                                       mtdValueInfo * aValueInfo2 );

    static SInt compareNumberGroupBigintStoredStoredAsc( mtdValueInfo * aValueInfo1,
                                                         mtdValueInfo * aValueInfo2 );

    static SInt compareNumberGroupBigintStoredStoredDesc( mtdValueInfo * aValueInfo1,
                                                          mtdValueInfo * aValueInfo2 );

    //----------------------------------------------------------------
    // PROJ-1364 해당 value를  double type으로 converion하여 compare 
    //---------------------------------------------------------------

    // 해당 value를  double type으로 converion 하는 함수 
    // 인자로 받은 data type을 double 형으로 변환
    // 동일컬럼에 대한 통합 selectivity 계산시,
    // 비교대상 data type을 double형으로 변환시켜서 selectivity를 구함.
    static void convertToDoubleType4MtdValue( mtdValueInfo  * aValueInfo,
                                              mtdDoubleType * aDoubleValue );
    
    static void convertToDoubleType4StoredValue( mtdValueInfo  * aValueInfo,
                                                 mtdDoubleType * aDoubleValue);
  
    // double type으로 conversion하여 compare 하는 함수 
    static SInt compareNumberGroupDoubleMtdMtdAsc( mtdValueInfo * aValueInfo1,
                                                   mtdValueInfo * aValueInfo2 );

    static SInt compareNumberGroupDoubleMtdMtdDesc( mtdValueInfo * aValueInfo1,
                                                    mtdValueInfo * aValueInfo2 );
    
    static SInt compareNumberGroupDoubleStoredMtdAsc( mtdValueInfo * aValueInfo1,
                                                      mtdValueInfo * aValueInfo2 );

    static SInt compareNumberGroupDoubleStoredMtdDesc( mtdValueInfo * aValueInfo1,
                                                       mtdValueInfo * aValueInfo2 );

    static SInt compareNumberGroupDoubleStoredStoredAsc( mtdValueInfo * aValueInfo1,
                                                         mtdValueInfo * aValueInfo2 );

    static SInt compareNumberGroupDoubleStoredStoredDesc( mtdValueInfo * aValueInfo1,
                                                          mtdValueInfo * aValueInfo2 );

    //----------------------------------------------------------------
    // PROJ-1364 해당 value를  numeric type으로 converion하여 compare 
    //---------------------------------------------------------------

    // 해당 value를 numeric type으로 conversion 하는 함수 
    static void convertToNumericType4MtdValue( mtdValueInfo    * aValueInfo,
                                               mtdNumericType ** aNumericValue);
    
    static void convertToNumericType4StoredValue(
                                     mtdValueInfo    * aValueInfo,
                                     mtdNumericType ** aNumericValue,
                                     UChar           * aLength,
                                     UChar          ** aSignExponentMantissa);

    // numeric type으로 conversion하여 compare 하는 함수 
    static SInt compareNumberGroupNumericMtdMtdAsc( mtdValueInfo * aValueInfo1,
                                                    mtdValueInfo * aValueInfo2 );

    static SInt compareNumberGroupNumericMtdMtdDesc( mtdValueInfo * aValueInfo1,
                                                     mtdValueInfo * aValueInfo2 );
    
    static SInt compareNumberGroupNumericStoredMtdAsc( mtdValueInfo * aValueInfo1,
                                                       mtdValueInfo * aValueInfo2 );

    static SInt compareNumberGroupNumericStoredMtdDesc( mtdValueInfo * aValueInfo1,
                                                        mtdValueInfo * aValueInfo2 );

    static SInt compareNumberGroupNumericStoredStoredAsc( mtdValueInfo * aValueInfo1,
                                                          mtdValueInfo * aValueInfo2 );

    static SInt compareNumberGroupNumericStoredStoredDesc( mtdValueInfo * aValueInfo1,
                                                           mtdValueInfo * aValueInfo2 );

    static UInt getNumberOfModules( void );

    static IDE_RC isTrueNA( idBool*          aResult,
                            const mtcColumn* aColumn,
                            const void*      aRow );

    static SInt compareNA( mtdValueInfo * /* aValueInfo1 */,
                           mtdValueInfo * /* aValueInfo2 */ );

    static IDE_RC canonizeDefault( const mtcColumn  *  aCanon,
                                   void             ** aCanonized,
                                   mtcEncryptInfo   *  aCanonInfo,
                                   const mtcColumn  *  aColumn,
                                   void             *  aValue,
                                   mtcEncryptInfo   *  aColumnInfo,
                                   mtcTemplate      *  aTemplate );

    static IDE_RC initializeModule( mtdModule* aModule,
                                    UInt       aNo );

    static IDE_RC initialize( mtdModule ***  aExtTypeModuleGroup,
                              UInt           aGroupCnt );

    static IDE_RC finalize( void );

    // PROJ-1361 : mtdModule과 mtlModue 분리했으므로
    //             mtdModue 검색시 language 정보 받지 않음
    static IDE_RC moduleByName( const mtdModule** aModule,
                                const void*       aName,
                                UInt              aLength );

    static IDE_RC moduleById( const mtdModule** aModule,
                              UInt              aId );

    static IDE_RC moduleByNo( const mtdModule** aModule,
                              UInt              aNo );

    static idBool isNullDefault( const mtcColumn* aColumn,
                                 const void*      aRow );

    static IDE_RC encodeCharDefault( mtcColumn  * aColumn,
                                     void       * aValue,
                                     UInt         aValueSize,
                                     UChar      * aCompileFmt,
                                     UInt         aCompileFmtLen,
                                     UChar      * aText,
                                     UInt       * aTextLen,
                                     IDE_RC     * aRet );

    static IDE_RC encodeNumericDefault( mtcColumn  * aColumn,
                                        void       * aValue,
                                        UInt         aValueSize,
                                        UChar      * aCompileFmt,
                                        UInt         aCompileFmtLen,
                                        UChar      * aText,
                                        UInt       * aTextLen,
                                        IDE_RC     * aRet );

    static IDE_RC decodeDefault( mtcTemplate * aTemplate,
                                 mtcColumn   * aColumn,
                                 void        * aValue,
                                 UInt        * aValueSize,
                                 UChar       * aCompileFmt,
                                 UInt          aCompileFmtLen,
                                 UChar       * aText,
                                 UInt          aTextLen,
                                 IDE_RC      * aRet );

    // To fix BUG-14235
    static IDE_RC encodeNA( mtcColumn  * aColumn,
                            void       * aValue,
                            UInt         aValueSize,
                            UChar      * aCompileFmt,
                            UInt         aCompileFmtLen,
                            UChar      * aText,
                            UInt       * aTextLen,
                            IDE_RC     * aRet );

    // PROJ-2002 Column Security
    static IDE_RC decodeNA( mtcTemplate * aTemplate,
                            mtcColumn   * aColumn,
                            void        * aValue,
                            UInt        * aValueSize,
                            UChar       * aCompileFmt,
                            UInt          aCompileFmtLen,
                            UChar       * aText,
                            UInt          aTextLen,
                            IDE_RC      * aRet );

    static IDE_RC compileFmtDefault( mtcColumn  * aColumn,
                                     UChar      * aCompiledFmt,
                                     UInt       * aCompiledFmtLen,
                                     UChar      * aFormatString,
                                     UInt         aFormatStringLength,
                                     IDE_RC     * aRet );

    static IDE_RC valueFromOracleDefault( mtcColumn*  aColumn,
                                          void*       aValue,
                                          UInt*       aValueOffset,
                                          UInt        aValueSize,
                                          const void* aOracleValue,
                                          SInt        aOracleLength,
                                          IDE_RC*     aResult );

    static IDE_RC validateDefault( mtcColumn * aColumn,
                                   void      * aValue,
                                   UInt        aValueSize );

    static IDE_RC makeColumnInfoDefault( void * aStmt,
                                         void * aTableInfo,
                                         UInt   aIdx );

    static SDouble selectivityNA( void     * aColumnMax,
                                  void     * aColumnMin,
                                  void     * aValueMax,
                                  void     * aValueMin,
                                  SDouble    aBoundFactor,
                                  SDouble    aTotalRecordCnt );

    // PROJ-1579 NCHAR
    static SDouble selectivityDefault( void     * aColumnMax,
                                       void     * aColumnMin,
                                       void     * aValueMax,
                                       void     * aValueMin,
                                       SDouble    aBoundFactor,
                                       SDouble    aTotalRecordCnt );

    static const void* valueForModule( const smiColumn* aColumn,
                                       const void*      aRow,
                                       UInt             aFlag,
                                       const void*      aDefaultNull );
    
    // Data Type의 기본 인덱스 타입을 구한다.
    static UInt getDefaultIndexTypeID( const mtdModule * aModule );

    // Data Type에 사용가능한 인덱스인지 판단함.
    static idBool isUsableIndexType( const mtdModule * aModule,
                                     UInt              aIndexType );

    // PROJ-1558
    // MM에서 NULL값을 지정한다.
    static IDE_RC assignNullValueById( UInt    aId,
                                       void ** aValue,
                                       UInt  * aSize );

    // PROJ-1558
    // MM에서 NULL값인지 검사한다.
    static IDE_RC checkNullValueById( UInt     aId,
                                      void   * aValue,
                                      idBool * aIsNull );

    // PROJ-1705
    // 저장되지 않는 데이타타입에 대한 처리     
    static IDE_RC mtdStoredValue2MtdValueNA( UInt              aColumnSize,
                                             void            * aRow,
                                             UInt              aOffset,
                                             UInt              aLength,
                                             const void      * aValue );
    // PROJ-1705
    // 저장되지 않는 데이타타입에 대한 처리     
    static UInt mtdNullValueSizeNA();
    
    // PROJ-1705
    // 저장되지 않는 데이타타입에 대한 처리         
    static UInt mtdHeaderSizeNA();

    // PROJ-1705
    // length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
    // integer와 같은 고정길이 데이타타입은 default로 0 반환
    static UInt mtdHeaderSizeDefault();


    // PROJ-2399 row tmaplate
    // sm에 저장되는 데이터의 크기를 반환한다.
    // VARCHAR, BLOB, CLOB, FLOAT, NUMBER, VARBIT, NUMERIC, NVARCHAR, 
    // EVARCHAR, ECHAR, GEOMETRY, NIBBLE 과 같은 variable 타입의 데이터 타입은 ID_UINT_MAX을 반.
    // mtheader가 sm에 저장된경우가 아니면 mtheader크기를 빼서 반환.
    static UInt mtdStoreSizeDefault( const smiColumn * aColumn );
    
    // PROJ-1579 NCHAR
    // META 단계에서 CHAR, VARCHAR, CLOB, CLOBLOCATOR, NCHAR, NVARCHAR 
    // 타입에 대해서 language를 다시 세팅해준다.
    static IDE_RC modifyNls4MtdModule();

    // PROJ-1877
    // 데이터의 실제 precision, scale 값을 반환한다.
    static IDE_RC getPrecisionNA( const mtcColumn * aColumn,
                                  const void      * aRow,
                                  SInt            * aPrecision,
                                  SInt            * aScale );

    // PROJ-1872
    static mtdCompareFunc  findCompareFunc( mtcColumn * aColumn1,
                                            mtcColumn * aColumn2,
                                            UInt        aCompValueType,
                                            UInt        aDirection );

    // PROJ-2429
    static IDE_RC mtdStoredValue2MtdValue4CompressColumn( UInt              aColumnSize,
                                                          void            * aDestValue,
                                                          UInt              aDestValueOffset,
                                                          UInt              aLength,
                                                          const void      * aValue );
    /* PROJ-2446 ONE SOURCE */
    static IDE_RC setMtcColumnInfo( void * aColumn );

    /* PROJ-2433 Direct Key Index
     * direct key index를 사용할수있는 data type 인지 확인 */
    static inline idBool isUsableDirectKeyIndex( void *aColumn );

    /* PROJ-2433 Direct Key Index
     * partial direct key index를 사용할수있는 data type 인지 확인 */
    static inline idBool isUsablePartialDirectKeyIndex( void *aColumn );
};

/*******************************************************************
 * Definition
 *    mtdModule에서 사용할 value 함수
 *
 * Description
 *    mtc::value는 상위 레이어(QP, MM)에서 사용할 수 있는 범용 value 함수
 *    mtd::valueForModule은 mtd에서만 사용한다.
 *    네번째 인자인 aDefaultNull에 mtdModule.staticNull을 assign한다.
 *
 * by kumdory, 2005-03-15
 *
 *******************************************************************/
inline const void* mtd::valueForModule( const smiColumn* aColumn,
                                        const void*      aRow,
                                        UInt             aFlag,
                                        const void*      aDefaultNull )
{
    const void* sValue;
    UInt        sLength;

    /*BUG-24052*/
    sValue = NULL;

    if( ( aFlag & MTD_OFFSET_MASK ) == MTD_OFFSET_USELESS )
    {
        sValue = aRow;

        if ( aColumn != NULL )
        {
            if ( SMI_COLUMN_TYPE_IS_TEMP( aColumn->flag ) == ID_TRUE )
            {
                sValue = aColumn->value;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // PROJ-2264 Dictionary table
        if( ( aColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
            != SMI_COLUMN_COMPRESSION_TRUE )
        {
            if( ( aColumn->flag & SMI_COLUMN_TYPE_MASK )
                == SMI_COLUMN_TYPE_FIXED )
            {
                sValue = (const void*)((const UChar*)aRow+aColumn->offset);
            }
            else if ( ( ( aColumn->flag & SMI_COLUMN_TYPE_MASK )
                        == SMI_COLUMN_TYPE_VARIABLE ) ||
                      ( ( aColumn->flag & SMI_COLUMN_TYPE_MASK )
                        == SMI_COLUMN_TYPE_VARIABLE_LARGE ) )
            {
                IDE_DASSERT( ( aColumn->flag & SMI_COLUMN_STORAGE_MASK )
                             == SMI_COLUMN_STORAGE_MEMORY );

                sValue = mtc::getColumn( aRow,
                                         aColumn,
                                         &sLength );
                
                // sValue가 NULL일 수 있다.
                if( sValue == NULL )
                {
                    sValue = aDefaultNull;
                }
            }
            else if( ( aColumn->flag & SMI_COLUMN_TYPE_MASK )
                     == SMI_COLUMN_TYPE_LOB )
            {
                // PROJ-1362
                sValue = aDefaultNull;
            }
            // PROJ-2362 memory temp 저장 효율성 개선
            else if( SMI_COLUMN_TYPE_IS_TEMP( aColumn->flag )
                     == ID_TRUE )
            {
                sValue = (const void*) aColumn->value;
            }
            else
            {
                IDE_ASSERT(0);
            }
        }
        else
        {
            IDE_DASSERT( SMI_COLUMN_TYPE_IS_TEMP( aColumn->flag )
                         == ID_FALSE );
            
            // PROJ-2264 Dictionary table
            sValue = mtc::getCompressionColumn( aRow,
                                                aColumn,
                                                ID_TRUE, // aUseColumnOffset
                                                &sLength );
            
            // sValue가 NULL일 수 있다.
            if ( ( ( ( aColumn->flag & SMI_COLUMN_TYPE_MASK )
                     == SMI_COLUMN_TYPE_VARIABLE ) ||
                   ( ( aColumn->flag & SMI_COLUMN_TYPE_MASK )
                     == SMI_COLUMN_TYPE_VARIABLE_LARGE ) )
                && ( sValue == NULL ) )
            {
                sValue = aDefaultNull;
            }
        }
    }

    return sValue;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : mtd::isUsableDirectKeyIndex                *
 * ------------------------------------------------------------------*
 * PROJ-2433 Direct Key Index
 * direct key index가 가능한 data type인지 확인한다.
 *
 * aColumn  - [IN]  컬럼정보
 *********************************************************************/
inline idBool mtd::isUsableDirectKeyIndex( void *aColumn )
{
    UInt sDataType;

    sDataType = ((mtcColumn *)aColumn)->type.dataTypeId;

    if ( ( sDataType == MTD_BIGINT_ID ) ||
         ( sDataType == MTD_DOUBLE_ID ) ||
         ( sDataType == MTD_INTEGER_ID ) ||
         ( sDataType == MTD_REAL_ID ) ||
         ( sDataType == MTD_SMALLINT_ID ) ||
         ( sDataType == MTD_FLOAT_ID ) ||
         ( sDataType == MTD_NUMBER_ID ) ||
         ( sDataType == MTD_NUMERIC_ID ) ||
         ( sDataType == MTD_CHAR_ID ) ||
         ( sDataType == MTD_VARCHAR_ID ) ||
         ( sDataType == MTD_NCHAR_ID ) ||
         ( sDataType == MTD_NVARCHAR_ID ) ||
         ( sDataType == MTD_DATE_ID ) ||
         ( sDataType == MTD_BIT_ID ) ||
         ( sDataType == MTD_VARBIT_ID ) ||
         ( sDataType == MTD_BYTE_ID ) ||
         ( sDataType == MTD_VARBYTE_ID ) ||
         ( sDataType == MTD_NIBBLE_ID ) )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/*********************************************************************
 * FUNCTION DESCRIPTION : mtd::isUsablePartialDirectKeyIndex         *
 * ------------------------------------------------------------------*
 * PROJ-2433 Direct Key Index
 * partial direct key index가 가능한 data type인지 확인한다.
 *
 * aColumn  - [IN]  컬럼정보
 *********************************************************************/
inline idBool mtd::isUsablePartialDirectKeyIndex( void *aColumn )
{
    UInt sDataType;

    sDataType = ((mtcColumn *)aColumn)->type.dataTypeId;

    if ( ( sDataType == MTD_CHAR_ID ) ||
         ( sDataType == MTD_VARCHAR_ID ) ||
         ( sDataType == MTD_NCHAR_ID ) ||
         ( sDataType == MTD_NVARCHAR_ID ) )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

#endif /* _O_MTD_H_ */
 

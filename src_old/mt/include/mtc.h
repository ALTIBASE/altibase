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
 * $Id: mtc.h 90192 2021-03-12 02:01:03Z jayce.park $
 **********************************************************************/

#ifndef _O_MTC_H_
# define _O_MTC_H_ 1

# include <mtcDef.h>
# include <mtdTypes.h>
# include <idn.h>

#define MTC_MAXIMUM_EXTERNAL_GROUP_CNT          (128)
#define MTC_MAXIMUM_CHAR_SIZE_FOR_NUMBER_GROUP  (1024)

/* PROJ-2209 DBTIMEZONE */
#define MTC_TIMEZONE_NAME_LEN  (40)
#define MTC_TIMEZONE_VALUE_LEN (6)

// PROJ-4155 DBMS PIPE
#define MTC_MSG_VARBYTE_TYPE        (MTD_VARBYTE_ID)
#define MTC_MSG_MAX_BUFFER_SIZE     (8192) // 8k

// 참고.
// permission 0644(rw-r-r), 0600(rw-----)으로 user1에서 생성한
// msq queue를 user2에서 snd,rcv 할수 없다. 
// 차이점은 0600으로 생성한 msg queue는  user2 에서 ipcs -a 조회시
// msg queue 정보 조회가 않됨
#define MTC_MSG_PUBLIC_PERMISSION   (0666)
#define MTC_MSG_PRIVATE_PERMISSION  (0644)

typedef struct mtcMsgBuffer
{
    SLong  mType;
    UChar  mMessage[MTC_MSG_MAX_BUFFER_SIZE];
} mtcMsgBuffer;

#if defined(WRS_VXWORKS)
static SInt sDays[2][14] = {
    { 0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
    { 0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};
#endif

/* PROJ-2446 ONE SOURCE */
typedef void * mmlDciTableColumn;

typedef IDE_RC (*dciCharConverFn)(
                     void            * ,
                     const mtlModule * ,
                     const mtlModule * ,
                     void            * ,
                     SInt              ,
                     void            * ,
                     SInt            * ,
                     SInt      );

#define DCI_CONV_DATA_IN       (0x01)
#define DCI_CONV_DATA_OUT      (0x02)
#define DCI_CONV_CALC_TOTSIZE  (0x10) /* 문자열 전체 길이 계산 */

class mtc
{
private:
    static const UChar hashPermut[256];

    static IDE_RC cloneTuple( iduMemory   * aMemory,
                              mtcTemplate * aSrcTemplate,
                              UShort        aSrcTupleID,
                              mtcTemplate * aDstTemplate,
                              UShort        aDstTupleID );

    static IDE_RC cloneTuple( iduVarMemList * aMemory,
                              mtcTemplate   * aSrcTemplate,
                              UShort          aSrcTupleID,
                              mtcTemplate   * aDstTemplate,
                              UShort          aDstTupleID );

    /* PROJ-1071 Parallel Query */
    static IDE_RC cloneTuple4Parallel( iduMemory   * aMemory,
                                       mtcTemplate * aSrcTemplate,
                                       UShort        aSrcTupleID,
                                       mtcTemplate * aDstTemplate,
                                       UShort        aDstTupleID );

public:
    static inline SInt dayOfYear( SInt aYear,
                                  SInt aMonth,
                                  SInt aDay )
    {
#if !defined(WRS_VXWORKS)
        static SInt sDays[2][14] = {
            { 0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
            { 0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
        };
#endif

        // BUG-22710
        if ( mtdDateInterface::isLeapYear( aYear ) == ID_TRUE )
        {
            return sDays[1][aMonth] + aDay;
        }
        else
        {
            return sDays[0][aMonth] + aDay;
        }
    }

    static inline SInt dayOfCommonEra( SInt aYear,
                                       SInt aMonth,
                                       SInt aDay )
    {
        SInt sDays;

        if ( aYear < 1582 )
        {
            /* BUG-36296 기원전에는 4년마다 윤년이라고 가정한다.
             *  - 고대 로마의 정치가 율리우스 카이사르가 기원전 45년부터 율리우스력을 시행하였다.
             *  - 초기의 율리우스력(기원전 45년 ~ 기원전 8년)에서는 윤년을 3년에 한 번 실시하였다. (Oracle 11g 미적용)
             *  - BC 4713년부터 율리우스일을 계산한다. 천문학자들이 율리우스일을 사용한다. 4년마다 윤년이다.
             *
             *  AD 0년은 존재하지 않는다. aYear가 0이면, BC 1년이다. 즉, BC 1년의 다음은 AD 1년이다.
             */
            if ( aYear <= 0 )
            {
                sDays = ( aYear * 365 ) + ( aYear / 4 )
                      + dayOfYear( aYear, aMonth, aDay )
                      - 366; /* BC 1년(aYear == 0)이 윤년이다. */
            }
            /* BUG-36296 그레고리력의 윤년 규칙은 1583년부터 적용한다. 1582년 이전에는 4년마다 윤년이다. */
            else
            {
                sDays = ( ( aYear - 1 ) * 365 ) + ( ( aYear - 1 ) / 4 )
                      + dayOfYear( aYear, aMonth, aDay );
            }
        }
        else if ( aYear == 1582 )
        {
            /* BUG-36296 그레고리력은 1582년 10월 15일부터 시작한다. */
            if ( ( aMonth < 10 ) ||
                 ( ( aMonth == 10 ) && ( aDay < 15 ) ) )
            {
                sDays = ( ( aYear - 1 ) * 365 ) + ( ( aYear - 1 ) / 4 )
                      + dayOfYear( aYear, aMonth, aDay );
            }
            else
            {
                sDays = ( ( aYear - 1 ) * 365 ) + ( ( aYear - 1 ) / 4 )
                      + dayOfYear( aYear, aMonth, aDay )
                      - 10; /* 1582년 10월 4일(목)에서 10월 15일(금)으로 바로 건너뛴다. */
            }
        }
        else
        {
            /* BUG-36296 1600년 이전은 율리우스력과 그레고리력의 윤년이 같다. */
            if ( aYear <= 1600 )
            {
                sDays = ( ( aYear - 1 ) * 365 ) + ( ( aYear - 1 ) / 4 )
                      + dayOfYear( aYear, aMonth, aDay )
                      - 10; /* 1582년 10월 4일(목)에서 10월 15일(금)으로 바로 건너뛴다. */
            }
            else
            {
                sDays = ( ( aYear - 1 ) * 365 ) + ( ( aYear - 1 ) / 4 )
                      - ( ( aYear - 1 - 1600 ) / 100 ) + ( ( aYear - 1 - 1600 ) / 400 )
                      + dayOfYear( aYear, aMonth, aDay )
                      - 10; /* 1582년 10월 4일(목)에서 10월 15일(금)으로 바로 건너뛴다. */
            }
        }

        /* AD 0001년 1월 1일은 Day 0 이다. 그리고, BC 0001년 12월 31일은 Day -1 이다. */
        sDays--;

        return sDays;
    }

    /* 일요일 : 0, 토요일 : 6 */
    static inline SInt dayOfWeek( SInt aYear,
                                  SInt aMonth,
                                  SInt aDay )
    {
        /* 기원전인 경우에는 음수가 나오므로, %7을 먼저하고 보정값을 더한다.
         * AD 0001년 01월 01일은 토요일(sDays = 0)이다. +6를 하여 보정한다.
         */
        return ( ( dayOfCommonEra( aYear, aMonth, aDay ) % 7 ) + 6 ) % 7;
    }

    /* 달력과 일치하는 주차 : 주가 일요일부터 시작한다. */
    static inline SInt weekOfYear( SInt aYear,
                                   SInt aMonth,
                                   SInt aDay )
    {
        SInt sWeek;

        sWeek = (SInt) idlOS::ceil( (SDouble)( dayOfWeek ( aYear, 1, 1 ) +
                                               dayOfYear ( aYear, aMonth, aDay ) ) / 7 );
        return sWeek;
    }

    /* BUG-42941 TO_CHAR()에 WW2(Oracle Version WW) 추가 */
    static inline SInt weekOfYearForOracle( SInt aYear,
                                            SInt aMonth,
                                            SInt aDay )
    {
        SInt sWeek = 0;

        sWeek = ( dayOfYear( aYear, aMonth, aDay ) + 6 ) / 7;

        return sWeek;
    }

    /* BUG-42926 TO_CHAR()에 IW 추가 */
    static SInt weekOfYearForStandard( SInt aYear,
                                       SInt aMonth,
                                       SInt aDay );

    /* BUG-46727 TO_CHAR()에 IYYY 추가 */
    static SInt yearForStandard( SInt aYear,
                                 SInt aMonth,
                                 SInt aDay );

    static const UInt  hashInitialValue;

    // mtd::valueForModule에서 사용한다.
    static mtcGetColumnFunc            getColumn;
    static mtcOpenLobCursorWithRow     openLobCursorWithRow;
    static mtcOpenLobCursorWithGRID    openLobCursorWithGRID;
    static mtcReadLob                  readLob;
    static mtcGetLobLengthWithLocator  getLobLengthWithLocator;
    static mtcIsNullLobColumnWithRow   isNullLobColumnWithRow;
    static mtcGetCompressionColumnFunc getCompressionColumn;
    /* PROJ-2446 ONE SOURCE */
    static mtcGetStatistics            getStatistics;

    // PROJ-2047 Strengthening LOB
    static IDE_RC getLobLengthLocator( smLobLocator   aLobLocator,
                                       idBool       * aIsNull,
                                       UInt         * aLobLength,
                                       idvSQL       * aStatistics );
    static IDE_RC isNullLobRow( const void      * aRow,
                                const smiColumn * aColumn,
                                idBool          * aIsNull );
    
    // PROJ-1579 NCHAR
    static mtcGetDBCharSet            getDBCharSet;
    static mtcGetNationalCharSet      getNationalCharSet;

    static const void* getColumnDefault( const void*,
                                         const smiColumn*,
                                         UInt* );

    static UInt         mExtTypeModuleGroupCnt;
    static mtdModule ** mExtTypeModuleGroup[MTC_MAXIMUM_EXTERNAL_GROUP_CNT];

    static UInt         mExtCvtModuleGroupCnt;
    static mtvModule ** mExtCvtModuleGroup[MTC_MAXIMUM_EXTERNAL_GROUP_CNT];

    static UInt         mExtFuncModuleGroupCnt;
    static mtfModule ** mExtFuncModuleGroup[MTC_MAXIMUM_EXTERNAL_GROUP_CNT];

    static UInt              mExtRangeFuncGroupCnt;
    static smiCallBackFunc * mExtRangeFuncGroup[MTC_MAXIMUM_EXTERNAL_GROUP_CNT];

    static UInt             mExtCompareFuncCnt;
    static mtdCompareFunc * mExtCompareFuncGroup[MTC_MAXIMUM_EXTERNAL_GROUP_CNT];

    static IDE_RC addExtTypeModule( mtdModule **  aExtTypeModules );
    static IDE_RC addExtCvtModule( mtvModule **  aExtCvtModules );
    static IDE_RC addExtFuncModule( mtfModule **  aExtFuncModules );
    static IDE_RC addExtRangeFunc( smiCallBackFunc * aExtRangeFuncs );
    static IDE_RC addExtCompareFunc( mtdCompareFunc * aExtCompareFuncs );

    static IDE_RC initialize( mtcExtCallback    * aCallBacks );

    static IDE_RC finalize( idvSQL * aStatistics );

    static UInt isSameType( const mtcColumn* aColumn1,
                            const mtcColumn* aColumn2 );

    static void copyColumn( mtcColumn*       aDestination,
                            const mtcColumn* aSource );

    static const void* value( const mtcColumn* aColumn,
                              const void*      aRow,
                              UInt             aFlag );

    static UInt hash( UInt         aHash,
                      const UChar* aValue,
                      UInt         aLength );

    // fix BUG-9496
    static UInt hashWithExponent( UInt         aHash,
                                  const UChar* aValue,
                                  UInt         aLength );

    static UInt hashSkip( UInt         aHash,
                          const UChar* aValue,
                          UInt         aLength );

    // BUG-41937
    static UInt hashSkipWithoutZero( UInt         aHash,
                                     const UChar* aValue,
                                     UInt         aLength );

    static UInt hashWithoutSpace( UInt         aHash,
                                  const UChar* aValue,
                                  UInt         aLength );

    static IDE_RC makeNumeric( mtdNumericType* aNumeric,
                               UInt            aMaximumMantissa,
                               const UChar*    aString,
                               UInt            aLength );

    /* PROJ-1517 */
    static void makeNumeric( mtdNumericType *aNumeric,
                             const SLong     aNumber );

    // BUG-41194
    static IDE_RC makeNumeric( mtdNumericType *aNumeric,
                               const SDouble   aNumber );
    
    static IDE_RC numericCanonize( mtdNumericType *aValue,
                                   mtdNumericType *aCanonizedValue,
                                   SInt            aCanonPrecision,
                                   SInt            aCanonScale,
                                   idBool         *aCanonized );

    static IDE_RC floatCanonize( mtdNumericType *aValue,
                                 mtdNumericType *aCanonizedValue,
                                 SInt            aCanonPrecision,
                                 idBool         *aCanonized );
        
    static IDE_RC makeSmallint( mtdSmallintType* aSmallint,
                                const UChar*     aString,
                                UInt             aLength );
    
    static IDE_RC makeInteger( mtdIntegerType* aInteger,
                               const UChar*    aString,
                               UInt            aLength );
    
    static IDE_RC makeBigint( mtdBigintType* aBigint,
                              const UChar*   aString,
                              UInt           aLength );
    
    static IDE_RC makeReal( mtdRealType* aReal,
                            const UChar* aString,
                            UInt         aLength );
    
    static IDE_RC makeDouble( mtdDoubleType* aDouble,
                              const UChar*   aString,
                              UInt           aLength );
    
    static IDE_RC makeInterval( mtdIntervalType* aInterval,
                                const UChar*     aString,
                                UInt             aLength );
    
    static IDE_RC makeBinary( mtdBinaryType* aBinary,
                              const UChar*   aString,
                              UInt           aLength );
    
    static IDE_RC makeBinary( UChar*       aBinaryValue,
                              const UChar* aString,
                              UInt         aLength,
                              UInt*        aBinaryLength,
                              idBool*      aOddSizeFlag );
    
    static IDE_RC makeByte( mtdByteType* aByte,
                            const UChar* aString,
                            UInt         aLength );

    static IDE_RC makeByte( UChar*       aByteValue,
                            const UChar* aString,
                            UInt         aLength,
                            UInt       * aByteLength,
                            idBool     * aOddSizeFlag);
    
    static IDE_RC makeNibble( mtdNibbleType* aNibble,
                              const UChar*   aString,
                              UInt           aLength );
    
    static IDE_RC makeNibble( UChar*       aNibbleValue,
                              UChar        aOffsetInByte,
                              const UChar* aString,
                              UInt         aLength,
                              UInt*        aNibbleLength );
    
    static IDE_RC makeBit( mtdBitType*  aBit,
                           const UChar* aString,
                           UInt         aLength );
    
    static IDE_RC makeBit( UChar*       aBitValue,
                           UChar        aOffsetInBit,
                           const UChar* aString,
                           UInt         aLength,
                           UInt*        aBitLength );
    
    static IDE_RC findCompare( const smiColumn* aColumn,
                               UInt             aFlag,
                               smiCompareFunc*  aCompare );

    // PROJ-1618
    static IDE_RC findKey2String( const smiColumn   * aColumn,
                                  UInt                aFlag,
                                  smiKey2StringFunc * aKey2String );

    // PROJ-1629
    static IDE_RC findNull( const smiColumn   * aColumn,
                            UInt                aFlag,
                            smiNullFunc       * aNull );
    
    static IDE_RC findIsNull( const smiColumn * aColumn,
                              UInt             aFlag,
                              smiIsNullFunc *  aIsNull );

    static IDE_RC getAlignValue( const smiColumn * aColumn,
                                 UInt *            aAlignValue );

    // Column에 맞는 Hash Key 추출 함수 검색
    static IDE_RC findHashKeyFunc( const smiColumn * aColumn,
                                   smiHashKeyFunc  * aHashKeyFunc );

    // PROJ-1705
    // QP영역처리가능한레코드에서 인덱스레코드구성시
    // 레코드로부터 컬럼데이타의 size와 value ptr를 얻는다.
    static IDE_RC getValueLengthFromFetchBuffer( idvSQL          * aStatistic,
                                                 const smiColumn * aColumn,
                                                 const void      * aRow,
                                                 UInt            * aColumnSize,
                                                 idBool          * aIsNullValue );

    // PROJ-1705
    // 디스크테이블컬럼의 데이타를 복사하는 함수 검색
    static IDE_RC findStoredValue2MtdValue(
        const smiColumn            * aColumn,
        smiCopyDiskColumnValueFunc * aStoredValue2MtdValueFunc );

    // PROJ-2429 Dictionary based data compress for on-disk DB 
    // 디스크테이블컬럼의 데이타 타입에 맞는 데이터 복사함수 검색
    static IDE_RC findStoredValue2MtdValue4DataType(
        const smiColumn            * aColumn,
        smiCopyDiskColumnValueFunc * aStoredValue2MtdValueFunc );

    static IDE_RC findActualSize( const smiColumn   * aColumn,                            
                                  smiActualSizeFunc * aActualSizeFunc );

    // PROJ-1705
    // 디스크테이블컬럼의 데이타를
    // qp 레코드처리영역의 해당 컬럼위치에 복사
    static void storedValue2MtdValue( const smiColumn * aColumn,
                                      void            * aDestValue,
                                      UInt              aDestValueOffset,
                                      UInt              aLength,
                                      const void      * aValue );    
                                       

    static IDE_RC cloneTemplate( iduMemory    * aMemory,
                                 mtcTemplate  * aSource,
                                 mtcTemplate  * aDestination );

    static IDE_RC cloneTemplate( iduVarMemList * aMemory,
                                 mtcTemplate   * aSource,
                                 mtcTemplate   * aDestination );

    // PROJ-2527 WITHIN GROUP AGGR
    static void finiTemplate( mtcTemplate * aTemplate );

    static IDE_RC addFloat( mtdNumericType *aValue,
                            UInt            aPrecisionMaximum,
                            mtdNumericType *aArgument1,
                            mtdNumericType *aArgument2 );

    static IDE_RC subtractFloat( mtdNumericType *aValue,
                                 UInt            aPrecisionMaximum,
                                 mtdNumericType *aArgument1,
                                 mtdNumericType *aArgument2 );

    static IDE_RC multiplyFloat( mtdNumericType *aValue,
                                 UInt            aPrecisionMaximum,
                                 mtdNumericType *aArgument1,
                                 mtdNumericType *aArgument2 );

    static IDE_RC divideFloat( mtdNumericType *aValue,
                               UInt            aPrecisionMaximum,
                               mtdNumericType *aArgument1,
                               mtdNumericType *aArgument2 );

    /* PROJ-1517 */
    static IDE_RC modFloat( mtdNumericType *aValue,
                            UInt            aPrecisionMaximum,
                            mtdNumericType *aArgument1,
                            mtdNumericType *aArgument2 );

    static IDE_RC signFloat( mtdNumericType *aValue,
                             mtdNumericType *aArgument1 );

    static IDE_RC absFloat( mtdNumericType *aValue,
                            mtdNumericType *aArgument1 );

    static IDE_RC ceilFloat( mtdNumericType *aValue,
                             mtdNumericType *aArgument1 );

    static IDE_RC floorFloat( mtdNumericType *aValue,
                              mtdNumericType *aArgument1 );

    static IDE_RC numeric2Slong( SLong          *aValue,
                                 mtdNumericType *aArgument1 );

    static void numeric2Double( mtdDoubleType  * aValue,
                                mtdNumericType * aArgument1 );

    // BUG-41194
    static IDE_RC double2Numeric( mtdNumericType *aValue,
                                  mtdDoubleType   aArgument1 );

    static IDE_RC roundFloat( mtdNumericType *aValue,
                              mtdNumericType *aArgument1,
                              mtdNumericType *aArgument2 );

    static IDE_RC truncFloat( mtdNumericType *aValue,
                              mtdNumericType *aArgument1,
                              mtdNumericType *aArgument2 );

    // To fix BUG-12944 정확한 precision 얻어오기.
    static IDE_RC getPrecisionScaleFloat( const mtdNumericType * aValue,
                                          SInt                 * aPrecision,
                                          SInt                 * aScale );

    // BUG-16531 이중화 Conflict 검사를 위한 Image 비교 함수
    static IDE_RC isSamePhysicalImage( const mtcColumn * aColumn,
                                       const void      * aRow1,
                                       UInt              aFlag1,
                                       const void      * aRow2,
                                       UInt              aFlag2,
                                       idBool          * aOutIsSame );

    //----------------------
    // mtcColumn의 초기화
    //----------------------

    // data module 및 language module을 지정한 경우
    static IDE_RC initializeColumn( mtcColumn       * aColumn,
                                    const mtdModule * aModule,
                                    UInt              aArguments,
                                    SInt              aPrecision,
                                    SInt              aScale );

    // 해당 data module 및 language module을 찾아서 설정해야 하는 경우
    static IDE_RC initializeColumn( mtcColumn       * aColumn,
                                    UInt              aDataTypeId,
                                    UInt              aArguments,
                                    SInt              aPrecision,
                                    SInt              aScale );

    // BUG-23012
    // src column으로 dest column을 초기화하는 경우
    static void initializeColumn( mtcColumn  * aDestColumn,
                                  mtcColumn  * aSrcColumn );
    
    // PROJ-2002 Column Security
    // data module 및 language module을 지정한 경우
    static IDE_RC initializeEncryptColumn( mtcColumn    * aColumn,
                                           const SChar  * aPolicy,
                                           UInt           aEncryptedSize,
                                           UInt           aECCSize );

    static idBool compareOneChar( UChar * aValue1,
                                  UChar   aValue1Size,
                                  UChar * aValue2,
                                  UChar   aValue2Size );

    // PROJ-1597
    static IDE_RC getNonStoringSize( const smiColumn * acolumn, UInt * aOutSize );


    // PROJ-2002 Column Security
    // echar/evarchar type의 like key size를 계산
    static IDE_RC getLikeEcharKeySize( mtcTemplate * aTemplate,
                                       UInt        * aECCSize,
                                       UInt        * aKeySize );
	/* PROJ-2209 DBTIMEZONE */
    static SLong  getSystemTimezoneSecond( void );
    static SChar *getSystemTimezoneString( SChar * aTimezoneString );

    /* PROJ-1517 */
    static void makeFloatConversionModule( mtcStack         *aStack,
                                           const mtdModule **aModule );

    // BUG-37484
    static idBool needMinMaxStatistics( const smiColumn* aColumn );

    /* PROJ-1071 Parallel query */
    static IDE_RC cloneTemplate4Parallel( iduMemory    * aMemory,
                                          mtcTemplate  * aSource,
                                          mtcTemplate  * aDestination );

    /* PROJ-2399 */
    static IDE_RC getColumnStoreLen( const smiColumn * aColumn,
                                     UInt            * aActualColLen );
};

#endif /* _O_MTC_H_ */

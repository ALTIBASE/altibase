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
 * $Id: mtdInterval.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>

#include <math.h>

extern mtdModule mtdInterval;

extern mtdModule mtdDouble;

#define MTD_INTERVAL_ALIGN (ID_SIZEOF(ULong))
#define MTD_INTERVAL_SIZE  (ID_SIZEOF(mtdIntervalType))
#define MTD_INTERVAL_IS_NULL( i )                      \
    ( (i)->second      == mtdIntervalNull.second    && \
      (i)->microsecond == mtdIntervalNull.microsecond  )

const mtdIntervalType mtdIntervalNull = {
    -ID_LONG(9223372036854775807)-1,
    -ID_LONG(9223372036854775807)-1
};

static IDE_RC mtdInitialize( UInt aNo );

static IDE_RC mtdEstimate( UInt * aColumnSize,
                           UInt * aArguments,
                           SInt * aPrecision,
                           SInt * aScale );

static IDE_RC mtdValue( mtcTemplate* aTemplate,
                        mtcColumn*   aColumn,
                        void*        aValue,
                        UInt*        aValueOffset,
                        UInt         aValueSize,
                        const void*  aToken,
                        UInt         aTokenLength,
                        IDE_RC*      aResult );

static UInt mtdActualSize( const mtcColumn* aColumn,
                           const void*      aRow );

static void mtdSetNull( const mtcColumn* aColumn,
                        void*            aRow );

static UInt mtdHash( UInt             aHash,
                     const mtcColumn* aColumn,
                     const void*      aRow );

static idBool mtdIsNull( const mtcColumn* aColumn,
                         const void*      aRow );

static SInt mtdIntervalFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                                   mtdValueInfo * aValueInfo2 );

static SInt mtdIntervalFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                                    mtdValueInfo * aValueInfo2  );

static SInt mtdIntervalMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * aValueInfo2 );

static SInt mtdIntervalMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                          mtdValueInfo * aValueInfo2  );

static SInt mtdIntervalStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                            mtdValueInfo * aValueInfo2 );

static SInt mtdIntervalStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                             mtdValueInfo * aValueInfo2 );

static void mtdEndian( void* aValue );

static IDE_RC mtdValidate( mtcColumn * aColumn,
                           void      * aValue,
                           UInt        aValueSize);

static IDE_RC mtdStoredValue2MtdValue( UInt              aColumnSize,
                                       void            * aDestValue,
                                       UInt           /* aDestValueOffset */,
                                       UInt              aLength,
                                       const void      * aValue );

static mtcName mtdTypeName[1] = {
    { NULL, 8, (void*)"INTERVAL" }
};

static mtcColumn mtdColumn;

mtdModule mtdInterval = {
    mtdTypeName,
    &mtdColumn,
    MTD_INTERVAL_ID,
    0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    MTD_INTERVAL_ALIGN,
    MTD_GROUP_INTERVAL|
      MTD_CANON_NEEDLESS|
      MTD_CREATE_DISABLE|
      MTD_COLUMN_TYPE_FIXED|
      MTD_SELECTIVITY_DISABLE|
      MTD_VARIABLE_LENGTH_TYPE_FALSE| // PROJ-1705
      MTD_DATA_STORE_DIVISIBLE_FALSE| // PROJ-1705
      MTD_DATA_STORE_MTDVALUE_FALSE|  // PROJ-1705
      MTD_PSM_TYPE_DISABLE, // PROJ-1904
    0,
    0,
    0,
    (void*)&mtdIntervalNull,
    mtdInitialize,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtd::getPrecisionNA,
    mtdSetNull,
    mtdHash,
    mtdIsNull,
    mtd::isTrueNA,
    {
        mtdIntervalMtdMtdKeyAscComp,   // Logical Comparison
        mtdIntervalMtdMtdKeyDescComp
    },
    {
        // Key Comparison
        {
            // mt value들 간의 compare
            mtdIntervalFixedMtdFixedMtdKeyAscComp, // Ascending Key Comparison
            mtdIntervalFixedMtdFixedMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value들 간의 compare
            mtdIntervalMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdIntervalMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // BUG-35413 disk temp에서 사용될 수 있음
            mtdIntervalStoredMtdKeyAscComp,     // Ascending Key Comparison
            mtdIntervalStoredMtdKeyDescComp     // Descending Key Comparison
        }
        ,
        {
            // 저장되지 않는 타입이므로 이 부분으로 들어올 수 없음
            mtd::compareNA,         // Ascending Key Comparison
            mtd::compareNA          // Descending Key Comparison
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 fixed mt value들 간의 compare */
            mtdIntervalFixedMtdFixedMtdKeyAscComp,
            mtdIntervalFixedMtdFixedMtdKeyDescComp
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 mt value들 간의 compare */
            mtdIntervalMtdMtdKeyAscComp,
            mtdIntervalMtdMtdKeyDescComp
        }
    },
    mtd::canonizeDefault,
    mtdEndian,
    mtdValidate,
    mtd::selectivityNA,
    NULL,
    mtd::decodeDefault,
    mtd::compileFmtDefault,
    mtd::valueFromOracleDefault,
    mtd::makeColumnInfoDefault,

    // BUG-28934
    mtk::mergeAndRangeNA,
    mtk::mergeOrRangeListNA,

    {    
        // PROJ-1705
        mtdStoredValue2MtdValue,
        // PROJ-2429
        NULL 
    }, 
    mtd::mtdNullValueSizeNA,
    mtd::mtdHeaderSizeDefault,

    // PROJ-2399
    mtd::mtdStoreSizeDefault
};

IDE_RC mtdInitialize( UInt aNo )
{
    IDE_TEST( mtd::initializeModule( &mtdInterval, aNo ) != IDE_SUCCESS );
    
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdInterval,
                                     0,   // arguments
                                     0,   // precision
                                     0 )  // scale
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtdEstimate( UInt * aColumnSize,
                    UInt * aArguments,
                    SInt * /*aPrecision*/,
                    SInt * /*aScale*/ )
{
    IDE_TEST_RAISE( *aArguments != 0, ERR_INVALID_LENGTH );

    *aColumnSize = MTD_INTERVAL_SIZE;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtdValue( mtcTemplate* /* aTemplate */,
                 mtcColumn*   aColumn,
                 void*        aValue,
                 UInt*        aValueOffset,
                 UInt         aValueSize,
                 const void*  aToken,
                 UInt         aTokenLength,
                 IDE_RC*      aResult )
{
    UInt             sValueOffset;
    mtdIntervalType* sValue;
    
    sValueOffset = idlOS::align( *aValueOffset, MTD_INTERVAL_ALIGN );

    if( sValueOffset + MTD_INTERVAL_SIZE <= aValueSize )
    {
        sValue = (mtdIntervalType*)( (UChar*)aValue + sValueOffset );
        if( aTokenLength == 0 )
        {
            *sValue = mtdIntervalNull;
        }
        else
        {
            /* BUGBUG: rewrite */
            char   sBuffer[1024];
            SDouble sDouble;
            SDouble sIntegerPart;
            SDouble sFractionalPart;
            IDE_TEST_RAISE( aTokenLength >= ID_SIZEOF(sBuffer),
                            ERR_INVALID_LITERAL );
            idlOS::memcpy( sBuffer, aToken, aTokenLength );
            sBuffer[aTokenLength] = 0;

            errno = 0;
            
            sDouble = idlOS::strtod( sBuffer, (char**)NULL );

            IDE_TEST_RAISE( mtdDouble.isNull( mtdDouble.column,
                                              &sDouble ) == ID_TRUE,
                            ERR_VALUE_OVERFLOW );
            /* PATCH(BEGIN): GNU MATH LIBRARY - REMOVE MINUS ZERO */
            if( sDouble == 0 )
            {
                sDouble = 0;
            }
            /* PATCH(END): GNU MATH LIBRARY - REMOVE MINUS ZERO */
            sDouble *= 864e2;
            IDE_TEST_RAISE( mtdDouble.isNull( mtdDouble.column,
                                              &sDouble ) == ID_TRUE,
                            ERR_VALUE_OVERFLOW );
            sFractionalPart     = modf( sDouble, &sIntegerPart );
            sValue->second      = (SLong)sIntegerPart;
            sValue->microsecond = (SLong)( sFractionalPart * 1E6 );
        }
        
        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset + MTD_INTERVAL_SIZE;
        
        *aResult = IDE_SUCCESS;
    }
    else
    {
        *aResult = IDE_FAILURE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));
    
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

UInt mtdActualSize( const mtcColumn*,
                    const void* )
{
    return MTD_INTERVAL_SIZE;
}

void mtdSetNull( const mtcColumn* /* aColumn */,
                 void*            aRow )
{
    if( aRow != NULL )
    {
        *(mtdIntervalType*)aRow = mtdIntervalNull;
    }
}

UInt mtdHash( UInt             aHash,
              const mtcColumn* ,
              const void*      aRow )
{
    return mtc::hash( aHash, (const UChar*)aRow, MTD_INTERVAL_SIZE );
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    return MTD_INTERVAL_IS_NULL( (mtdIntervalType*)aRow ) ? ID_TRUE : ID_FALSE ;
}

SInt mtdIntervalFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                            mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Fixed Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/
    
    const mtdIntervalType * sIntervalValue1;
    const mtdIntervalType * sIntervalValue2;
    SLong                   sSecond1;
    SLong                   sSecond2;
    SLong                   sMicrosecond1;
    SLong                   sMicrosecond2;
    idBool                  sNull1;
    idBool                  sNull2;

    //---------
    // value1
    //---------    
    sIntervalValue1 = (const mtdIntervalType*)MTD_VALUE_FIXED( aValueInfo1 );

    sSecond1      = sIntervalValue1->second;
    sMicrosecond1 = sIntervalValue1->microsecond;

    sNull1 = ( ( sSecond1 == mtdIntervalNull.second ) &&
               ( sMicrosecond1 == mtdIntervalNull.microsecond ) )
             ? ID_TRUE : ID_FALSE;

    //---------
    // value2
    //---------    
    sIntervalValue2 = (const mtdIntervalType*)MTD_VALUE_FIXED( aValueInfo2 );

    sSecond2      = sIntervalValue2->second;
    sMicrosecond2 = sIntervalValue2->microsecond;

    sNull2 = ( ( sSecond2 == mtdIntervalNull.second ) &&
               ( sMicrosecond2 == mtdIntervalNull.microsecond ) )
             ? ID_TRUE : ID_FALSE;

    //---------
    // compare
    //---------        

    if( ( sNull1 == ID_FALSE ) && ( sNull2 == ID_FALSE ) )
    {
        if( sSecond1      > sSecond2     ) return  1;
        if( sSecond1      < sSecond2      ) return -1;
        if( sMicrosecond1 > sMicrosecond2 ) return  1;
        if( sMicrosecond1 < sMicrosecond2 ) return -1;
        return 0;
    }
    
    if( ( sNull1 == ID_TRUE ) && ( sNull2 == ID_FALSE ) )
    {
        return 1;
    }
    if( ( sNull1 == ID_FALSE ) && ( sNull2 == ID_TRUE ) )
    {
        return -1;
    }
    return 0;
}

SInt mtdIntervalFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                             mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Fixed Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdIntervalType * sIntervalValue1;
    const mtdIntervalType * sIntervalValue2;
    SLong                   sSecond1;
    SLong                   sSecond2;
    SLong                   sMicrosecond1;
    SLong                   sMicrosecond2;
    idBool                  sNull1;
    idBool                  sNull2;

    //---------
    // value1
    //---------    
    sIntervalValue1 = (const mtdIntervalType*)MTD_VALUE_FIXED( aValueInfo1 );

    sSecond1      = sIntervalValue1->second;
    sMicrosecond1 = sIntervalValue1->microsecond;

    sNull1 = ( ( sSecond1 == mtdIntervalNull.second ) &&
               ( sMicrosecond1 == mtdIntervalNull.microsecond ) )
             ? ID_TRUE : ID_FALSE;

    //---------
    // value2
    //---------    
    sIntervalValue2 = (const mtdIntervalType*)MTD_VALUE_FIXED( aValueInfo2 );

    sSecond2      = sIntervalValue2->second;
    sMicrosecond2 = sIntervalValue2->microsecond;

    sNull2 = ( ( sSecond2 == mtdIntervalNull.second ) &&
               ( sMicrosecond2 == mtdIntervalNull.microsecond ) )
             ? ID_TRUE : ID_FALSE;

    //---------
    // compare
    //---------        

    if( ( sNull1 == ID_FALSE ) && ( sNull2 == ID_FALSE ) )
    {
        if( sSecond1      < sSecond2      ) return  1;
        if( sSecond1      > sSecond2      ) return -1;
        if( sMicrosecond1 < sMicrosecond2 ) return  1;
        if( sMicrosecond1 > sMicrosecond2 ) return -1;
        return 0;
    }
    
    if( ( sNull1 == ID_TRUE ) && ( sNull2 == ID_FALSE ) )
    {
        return 1;
    }
    if( ( sNull1 == ID_FALSE ) && ( sNull2 == ID_TRUE ) )
    {
        return -1;
    }
    return 0;
}

SInt mtdIntervalMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                  mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/
    
    const mtdIntervalType * sIntervalValue1;
    const mtdIntervalType * sIntervalValue2;
    SLong                   sSecond1;
    SLong                   sSecond2;
    SLong                   sMicrosecond1;
    SLong                   sMicrosecond2;
    idBool                  sNull1;
    idBool                  sNull2;

    //---------
    // value1
    //---------    
    sIntervalValue1 = (const mtdIntervalType*)
                       mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                            aValueInfo1->value,
                                            aValueInfo1->flag,
                                            mtdInterval.staticNull );

    sSecond1      = sIntervalValue1->second;
    sMicrosecond1 = sIntervalValue1->microsecond;

    sNull1 = ( ( sSecond1 == mtdIntervalNull.second ) &&
               ( sMicrosecond1 == mtdIntervalNull.microsecond ) )
             ? ID_TRUE : ID_FALSE;

    //---------
    // value2
    //---------    
    sIntervalValue2 = (const mtdIntervalType*)
                       mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                            aValueInfo2->value,
                                            aValueInfo2->flag,
                                            mtdInterval.staticNull );

    sSecond2      = sIntervalValue2->second;
    sMicrosecond2 = sIntervalValue2->microsecond;

    sNull2 = ( ( sSecond2 == mtdIntervalNull.second ) &&
               ( sMicrosecond2 == mtdIntervalNull.microsecond ) )
             ? ID_TRUE : ID_FALSE;

    //---------
    // compare
    //---------        

    if( ( sNull1 == ID_FALSE ) && ( sNull2 == ID_FALSE ) )
    {
        if( sSecond1      > sSecond2     ) return  1;
        if( sSecond1      < sSecond2      ) return -1;
        if( sMicrosecond1 > sMicrosecond2 ) return  1;
        if( sMicrosecond1 < sMicrosecond2 ) return -1;
        return 0;
    }
    
    if( ( sNull1 == ID_TRUE ) && ( sNull2 == ID_FALSE ) )
    {
        return 1;
    }
    if( ( sNull1 == ID_FALSE ) && ( sNull2 == ID_TRUE ) )
    {
        return -1;
    }
    return 0;
}

SInt mtdIntervalMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                   mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdIntervalType * sIntervalValue1;
    const mtdIntervalType * sIntervalValue2;
    SLong                   sSecond1;
    SLong                   sSecond2;
    SLong                   sMicrosecond1;
    SLong                   sMicrosecond2;
    idBool                  sNull1;
    idBool                  sNull2;

    //---------
    // value1
    //---------    
    sIntervalValue1 = (const mtdIntervalType*)
                       mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                            aValueInfo1->value,
                                            aValueInfo1->flag,
                                            mtdInterval.staticNull );

    sSecond1      = sIntervalValue1->second;
    sMicrosecond1 = sIntervalValue1->microsecond;

    sNull1 = ( ( sSecond1 == mtdIntervalNull.second ) &&
               ( sMicrosecond1 == mtdIntervalNull.microsecond ) )
             ? ID_TRUE : ID_FALSE;

    //---------
    // value2
    //---------    
    sIntervalValue2 = (const mtdIntervalType*)
                       mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                            aValueInfo2->value,
                                            aValueInfo2->flag,
                                            mtdInterval.staticNull );

    sSecond2      = sIntervalValue2->second;
    sMicrosecond2 = sIntervalValue2->microsecond;

    sNull2 = ( ( sSecond2 == mtdIntervalNull.second ) &&
               ( sMicrosecond2 == mtdIntervalNull.microsecond ) )
             ? ID_TRUE : ID_FALSE;

    //---------
    // compare
    //---------        

    if( ( sNull1 == ID_FALSE ) && ( sNull2 == ID_FALSE ) )
    {
        if( sSecond1      < sSecond2      ) return  1;
        if( sSecond1      > sSecond2      ) return -1;
        if( sMicrosecond1 < sMicrosecond2 ) return  1;
        if( sMicrosecond1 > sMicrosecond2 ) return -1;
        return 0;
    }
    
    if( ( sNull1 == ID_TRUE ) && ( sNull2 == ID_FALSE ) )
    {
        return 1;
    }
    if( ( sNull1 == ID_FALSE ) && ( sNull2 == ID_TRUE ) )
    {
        return -1;
    }
    return 0;
}

SInt mtdIntervalStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/
    
    const mtdIntervalType * sIntervalValue2;
    SLong                   sSecond1;
    SLong                   sSecond2;
    SLong                   sMicrosecond1;
    SLong                   sMicrosecond2;
    idBool                  sNull1;
    idBool                  sNull2;

    //---------
    // value1
    //---------
    ID_LONG_BYTE_ASSIGN( &sSecond1, aValueInfo1->value );
    ID_LONG_BYTE_ASSIGN( &sMicrosecond1, ((UChar*)aValueInfo1->value) + 8 );
    
    sNull1 = ( ( sSecond1 == mtdIntervalNull.second ) &&
               ( sMicrosecond1 == mtdIntervalNull.microsecond ) )
             ? ID_TRUE : ID_FALSE;

    //---------
    // value2
    //---------    
    sIntervalValue2 = (const mtdIntervalType*)
                       mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                            aValueInfo2->value,
                                            aValueInfo2->flag,
                                            mtdInterval.staticNull );

    sSecond2      = sIntervalValue2->second;
    sMicrosecond2 = sIntervalValue2->microsecond;

    sNull2 = ( ( sSecond2 == mtdIntervalNull.second ) &&
               ( sMicrosecond2 == mtdIntervalNull.microsecond ) )
             ? ID_TRUE : ID_FALSE;

    //---------
    // compare
    //---------        

    if( ( sNull1 == ID_FALSE ) && ( sNull2 == ID_FALSE ) )
    {
        if( sSecond1      > sSecond2     ) return  1;
        if( sSecond1      < sSecond2      ) return -1;
        if( sMicrosecond1 > sMicrosecond2 ) return  1;
        if( sMicrosecond1 < sMicrosecond2 ) return -1;
        return 0;
    }
    
    if( ( sNull1 == ID_TRUE ) && ( sNull2 == ID_FALSE ) )
    {
        return 1;
    }
    if( ( sNull1 == ID_FALSE ) && ( sNull2 == ID_TRUE ) )
    {
        return -1;
    }
    return 0;
}

SInt mtdIntervalStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdIntervalType * sIntervalValue2;
    SLong                   sSecond1;
    SLong                   sSecond2;
    SLong                   sMicrosecond1;
    SLong                   sMicrosecond2;
    idBool                  sNull1;
    idBool                  sNull2;

    //---------
    // value1
    //---------
    ID_LONG_BYTE_ASSIGN( &sSecond1, aValueInfo1->value );
    ID_LONG_BYTE_ASSIGN( &sMicrosecond1, ((UChar*)aValueInfo1->value) + 8 );

    sNull1 = ( ( sSecond1 == mtdIntervalNull.second ) &&
               ( sMicrosecond1 == mtdIntervalNull.microsecond ) )
             ? ID_TRUE : ID_FALSE;

    //---------
    // value2
    //---------    
    sIntervalValue2 = (const mtdIntervalType*)
                       mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                            aValueInfo2->value,
                                            aValueInfo2->flag,
                                            mtdInterval.staticNull );

    sSecond2      = sIntervalValue2->second;
    sMicrosecond2 = sIntervalValue2->microsecond;

    sNull2 = ( ( sSecond2 == mtdIntervalNull.second ) &&
               ( sMicrosecond2 == mtdIntervalNull.microsecond ) )
             ? ID_TRUE : ID_FALSE;

    //---------
    // compare
    //---------        

    if( ( sNull1 == ID_FALSE ) && ( sNull2 == ID_FALSE ) )
    {
        if( sSecond1      < sSecond2      ) return  1;
        if( sSecond1      > sSecond2      ) return -1;
        if( sMicrosecond1 < sMicrosecond2 ) return  1;
        if( sMicrosecond1 > sMicrosecond2 ) return -1;
        return 0;
    }
    
    if( ( sNull1 == ID_TRUE ) && ( sNull2 == ID_FALSE ) )
    {
        return 1;
    }
    if( ( sNull1 == ID_FALSE ) && ( sNull2 == ID_TRUE ) )
    {
        return -1;
    }
    return 0;
}

void mtdEndian( void* aValue )
{
    UChar* sValue;
    UChar  sIntermediate;
    
    sValue = (UChar*)&(((mtdIntervalType*)aValue)->second);

    sIntermediate = sValue[0];
    sValue[0]     = sValue[7];
    sValue[7]     = sIntermediate;
    sIntermediate = sValue[1];
    sValue[1]     = sValue[6];
    sValue[6]     = sIntermediate;
    sIntermediate = sValue[2];
    sValue[2]     = sValue[5];
    sValue[5]     = sIntermediate;
    sIntermediate = sValue[3];
    sValue[3]     = sValue[4];
    sValue[4]     = sIntermediate;
        
    sValue = (UChar*)&(((mtdIntervalType*)aValue)->microsecond);

    sIntermediate = sValue[0];
    sValue[0]     = sValue[7];
    sValue[7]     = sIntermediate;
    sIntermediate = sValue[1];
    sValue[1]     = sValue[6];
    sValue[6]     = sIntermediate;
    sIntermediate = sValue[2];
    sValue[2]     = sValue[5];
    sValue[5]     = sIntermediate;
    sIntermediate = sValue[3];
    sValue[3]     = sValue[4];
    sValue[4]     = sIntermediate;
}

IDE_RC mtdValidate( mtcColumn * aColumn,
                    void      * aValue,
                    UInt        aValueSize)
{
/***********************************************************************
 *
 * Description : value의 semantic 검사 및 mtcColum 초기화
 *
 * Implementation :
 *
 ***********************************************************************/
        
    IDE_TEST_RAISE( aValue == NULL, ERR_INVALID_NULL);
    
    IDE_TEST_RAISE( aValueSize != ID_SIZEOF(mtdIntervalType),
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdInterval,
                                     0,   // arguments
                                     0,   // precision
                                     0 )  // scale
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_NULL);
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_VALUE));
    }
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 
static IDE_RC mtdStoredValue2MtdValue( UInt              aColumnSize,
                                       void            * aDestValue,
                                       UInt           /* aDestValueOffset */,
                                       UInt              aLength,
                                       const void      * aValue )
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdIntervalType *sValue;

    // 고정길이 데이타 타입의 경우
    // 하나의 컬럼 데이타가 여러페이지에 나누어 저장되는 경우는 없다.

    sValue = (mtdIntervalType*)aDestValue;

    if( aLength == 0 )
    {
        // NULL 데이타
        *sValue = mtdIntervalNull;
    }
    else
    {
        IDE_TEST_RAISE( aLength != aColumnSize, ERR_INVALID_STORED_VALUE );

        idlOS::memcpy( sValue, aValue, aLength );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_INVALID_STORED_VALUE);
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_STORED_DATA_LENGTH));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


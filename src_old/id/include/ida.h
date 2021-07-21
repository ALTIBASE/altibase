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
 * $Id: ida.h 90444 2021-04-02 10:15:58Z minku.kang $
 **********************************************************************/

#ifndef _O_IDA_H_
# define  _O_IDA_H_  1

/**
 * @file
 * @ingroup ida
 * Arithmetic operations for NUMERIC and VARCHAR data.
 */

#include <idl.h>

#define IDA_INTEGER_DISPLAY_SIZE       12
#define IDA_DATE_DISPLAY_SIZE          11
#define IDA_TIMESTAMP_DISPLAY_SIZE     21
#define IDA_NUMERIC_DISPLAY_SIZE       10
#define IDA_TNUMERIC_DISPLAY_SIZE      10
#define IDA_BYTES_DISPLAY_SIZE( l )    ((l)*2)
#define IDA_NIBBLE_DISPLAY_SIZE( l )   ((l)*2)
#define IDA_CHAR_DISPLAY_SIZE( l )     (l)
#define IDA_VARCHAR_DISPLAY_SIZE( l )  (l)

#define IDA_DISPLAY_SIZE(t, l) ((t) == IDDT_INT) ? IDA_INTEGER_DISPLAY_SIZE : \
((t) == IDDT_BYT ? IDA_BYTES_DISPLAY_SIZE(l) : \
 ((t) == IDDT_CHA ? IDA_CHAR_DISPLAY_SIZE(l) : \
  ((t) == IDDT_VAR ? IDA_VARCHAR_DISPLAY_SIZE(l) : \
   ((t) == IDDT_BIT ? 0 : \
    ((t) == IDDT_DAT ? IDA_DATE_DISPLAY_SIZE : \
     ((t) == IDDT_TSP ? IDA_TIMESTAMP_DISPLAY_SIZE : \
      ((t) == IDDT_NUM ? IDA_NUMERIC_DISPLAY_SIZE : \
       ((t) == IDDT_TNU ? IDA_TNUMERIC_DISPLAY_SIZE : \
        ((t) == IDDT_NIB ? IDA_NIBBLE_DISPLAY_SIZE(l) : \
         0 \
        ) \
       ) \
      ) \
     ) \
    ) \
   ) \
  ) \
 ) \
) 

#define IDA_CHAR_MAX                   ( 256  )
#define IDA_VARCHAR_MAX                ( 4000 )
#define IDA_BIT_MAX                    ( 32000 )
#define IDA_NUMERIC_PRECISION_MAX      ( (sizeof(idaTNumericType)-2)*2 )
#define IDA_NUMERIC_PRECISION_MIN      ( 1 )
#define IDA_NUMERIC_SCALE_MAX          ( 126 )
#define IDA_NUMERIC_SCALE_MIN          ( -((SLong)126)+((SLong)(sizeof(idaTNumericType)*2)) )
#define IDA_NIBBLE_MAX                 ( 254 )
// #define IDA_BYTES_MAX                  ( 127 )
// fix BUG-12840
#define IDA_BYTES_MAX                  (65535 - ID_SIZEOF(UShort))

#define IDA_GET_LENGTH( x )            ( (((x)[0])<<8) | ((x)[1]) )
#define IDA_SET_LENGTH( x, y ) \
{                               \
    (x)[0] = (y)>>8;            \
    (x)[1] = (y)&0xFF;          \
}

#define IDA_GET_CHAR_SIZE( l )       ( l )
#define IDA_GET_VARCHAR_SIZE( l )    ( l + sizeof(idaVarcharType) - 1 )
#define IDA_GET_BIT_SIZE( l )        ( l + sizeof(idaBitType) - 1 )
#define IDA_GET_NUMERIC_SIZE( p, s ) ( 1+((p)/2)+((((p)%2)||((s)%2))?1:0) )
#define IDA_GET_DATE_SIZE            ( sizeof(idaDateType) )
#define IDA_GET_TIMESTAMP_SIZE       ( sizeof(idaTimestampType) )
#define IDA_GET_BOOLEAN_SIZE         ( sizeof(idaBooleanType) )
#define IDA_GET_TNUMERIC_SIZE        ( sizeof(idaTNumericType) )
#define IDA_GET_INTEGER_SIZE         ( sizeof(SInt) )
#define IDA_GET_NIBBLE_SIZE( l )     ( ((l+1)>>1) + sizeof(idaNibbleType) - 1 )
#define IDA_GET_BYTES_SIZE( l )      ( l )

#define IDA_GET_SIZE( t, p, s ) ((t) == IDDT_INT) ? IDA_GET_INTEGER_SIZE : \
((t) == IDDT_BYT ? IDA_GET_BYTES_SIZE(p) : \
 ((t) == IDDT_CHA ? IDA_GET_CHAR_SIZE(p) : \
  ((t) == IDDT_VAR ? IDA_GET_VARCHAR_SIZE(p) : \
   ((t) == IDDT_BIT ? IDA_GET_BIT_SIZE(p) : \
    ((t) == IDDT_DAT ? IDA_GET_DATE_SIZE : \
     ((t) == IDDT_TSP ? IDA_GET_TIMESTAMP_SIZE : \
      ((t) == IDDT_NUM ? IDA_GET_NUMERIC_SIZE((p),(s)) : \
       ((t) == IDDT_TNU ? IDA_GET_TNUMERIC_SIZE : \
        ((t) == IDDT_NIB ? IDA_GET_NIBBLE_SIZE(p) : \
         0 \
        ) \
       ) \
      ) \
     ) \
    ) \
   ) \
  ) \
 ) \
) 

#define IDA_BOOLEAN_IS_NULL( b )       ( (b)->value == IDA_NULL )
#define IDA_CHAR_IS_NULL( v )          ( (v)->value[0] == 0xFF )
#define IDA_VARCHAR_IS_NULL( v )       ( (v)->length[0] == 0 && (v)->length[1] == 0 )
#define IDA_BIT_IS_NULL( i )           ( (i)->length[0] == 0 && (i)->length[1] == 0 )
#define IDA_NUMERIC_IS_NULL( n )       ( (n)->sebyte == 0 )
#define IDA_DATE_IS_NULL( d )          ( (d)->year[0] == 0 && (d)->year[1] == 0 )
#define IDA_TIMESTAMP_IS_NULL( d )     ( \
(d)->second[0] == 0xFF \
&& (d)->second[1] == 0xFF \
&& (d)->second[2] == 0xFF \
&& (d)->second[3] == 0xFF \
&& (d)->second[4] == 0xFF \
&& (d)->second[5] == 0xFF \
&& (d)->second[6] == 0xFF \
&& (d)->second[7] == 0xFF \
&& (d)->microsecond[0] == 0xFF \
&& (d)->microsecond[1] == 0xFF \
&& (d)->microsecond[2] == 0xFF \
&& (d)->microsecond[3] == 0xFF \
&& (d)->microsecond[4] == 0xFF \
&& (d)->microsecond[5] == 0xFF \
&& (d)->microsecond[6] == 0xFF \
&& (d)->microsecond[7] == 0xFF \
)
#define IDA_TNUMERIC_IS_NULL( t )      ( (t)->sebyte == 0 )
#define IDA_NIBBLE_IS_NULL( n )        ( (n)->length == 0xFF )
#define IDA_INTEGER_IS_NULL( i )       ( idlOS::memcmp((i),&idaIntegerNULL,sizeof(SInt)) ? 0 : 1 )
#define IDA_BYTES_IS_NULL( b, l )      ( idlOS::memcmp((b)->value,idaByteBuffer255,(l)) ? 0 : 1 )

#define IDA_PNULL(t)  ((t) == IDDT_CHA ? (void *)(&idaCharNULL) :          \
                       ((t) == IDDT_VAR ? (void *)(&idaVarcharNULL) :      \
                        ((t) == IDDT_BIT ? (void *)(&idaBitNULL) :         \
                         ((t) == IDDT_NUM ? (void *)(&idaNumericNULL) :    \
                          ((t) == IDDT_TNU ? (void *)(&idaTNumericNULL) :  \
                           ((t) == IDDT_DAT ? (void *)(&idaDateNULL) :     \
                            ((t) == IDDT_TSP ? (void *)(&idaTimestampNULL) : \
                             ((t) == IDDT_INT ? (void *)(&idaIntegerNULL) : \
                              NULL                                          \
                             )                                              \
                            )                                              \
                           )                                               \
                          )                                                \
                         )                                                 \
                        )                                                  \
                       )                                                   \
                      )

#define IDA_PNULL_SIZE(t) ((t) == IDDT_CHA ? sizeof(idaCharNULL) :         \
                           ((t) == IDDT_VAR ? sizeof(idaVarcharNULL) :     \
                            ((t) == IDDT_BIT ? sizeof(idaBitNULL) :        \
                             ((t) == IDDT_NUM ? sizeof(idaNumericNULL) :   \
                              ((t) == IDDT_TNU ? sizeof(idaTNumericNULL) : \
                               ((t) == IDDT_DAT ? sizeof(idaDateNULL) :    \
                                ((t) == IDDT_TSP ? sizeof(idaTimestampNULL) : \
                                 ((t) == IDDT_INT ? sizeof(idaIntegerNULL) :\
                                  0                                         \
                                 )                                          \
                                )                                          \
                               )                                           \
                              )                                            \
                             )                                             \
                            )                                              \
                           )                                               \
                          )

#define IDA_INTEGER_IS_ZERO( i )       ( idlOS::memcmp((i),&idaIntegerZERO,sizeof(SInt)) ? 0 : 1 )
#define IDA_NUMERIC_IS_ZERO( n )       ( (n)->sebyte == 0x80 )
#define IDA_TNUMERIC_IS_ZERO( t )      ( (t)->sebyte == 0x80 )
#define IDA_TNUMERIC_IS_ONE( t )       ( idlOS::memcmp((t),&idaTNumericONE,sizeof(idaTNumericType)) ? 0 : 1 )

#define IDA_NUMERIC_IS_POSITIVE( n )      ((((n)->sebyte & 0x80) == 0x80)?1:0)
#define IDA_TNUMERIC_IS_POSITIVE( t )     ((((t)->sebyte & 0x80) == 0x80)?1:0)

#define IDA_NUMERIC_IS_NEGATIVE( n )      ((((n)->sebyte & 0x80) == 0)?1:0)
#define IDA_TNUMERIC_IS_NEGATIVE( t )     ((((t)->sebyte & 0x80) == 0)?1:0)

#define IDA_TNUMERIC_EQUAL_SIGN(t1,t2)     ( ((t1)->sebyte & 0x80) == ((t2)->sebyte & 0x80))

#define IDA_NUMERIC_SET_POSITIVE( n )      ((n)->sebyte = ((n)->sebyte | 0x80))
#define IDA_TNUMERIC_SET_POSITIVE( t )     ((t)->sebyte = ((t)->sebyte | 0x80))

#define IDA_NUMERIC_SET_ABS_POSITIVE( n )      ((n)->sebyte = 0x80)
#define IDA_TNUMERIC_SET_ABS_POSITIVE( t )     ((t)->sebyte = 0x80)

#define IDA_NUMERIC_SET_NEGAVIVE( n )      ((n)->sebyte = ((n)->sebyte & 0x7F))
#define IDA_TNUMERIC_SET_NEGATIVE( t )     ((t)->sebyte = ((t)->sebyte & 0x7F))

#define IDA_NUMERIC_SET_ABS_NEGAVIVE( n )      ((n)->sebyte = 0)
#define IDA_TNUMERIC_SET_ABS_NEGATIVE( t )     ((t)->sebyte = 0)

#define IDA_TNUMERIC_SET_SEBYTEVAL( t1, t2, expval )    ( (t1)->sebyte = ((t2)->sebyte & 0x80) | (expval) )
#define IDA_TNUMERIC_SET_SEBYTE2(t1,t2)     ( (t1)->sebyte = (t2)->sebyte )
#define IDA_TNUMERIC_SET_SEBYTECOMP2(t1,t2) ( (t1)->sebyte = ( (t2)->sebyte & 0x80 ) | (0x80 - ((t2)->sebyte & 0x7F)) )
#define IDA_TNUMERIC_SET_ABS_SIGN(t1,t2)    ( (t1)->sebyte = ((t2)->sebyte & 0x80) )
#define IDA_TNUMERIC_SET_SIGN( t1, t2 )    ( (t1)->sebyte = ((t1)->sebyte & 0x7F) | ((t2)->sebyte & 0x80) )

#define IDA_TNUMERIC_SET_EXP( t1, t2 )     ( (t1)->sebyte = ((t1)->sebyte & 0x80) | ((t2)->sebyte & 0x7F) )

#define IDA_TNUMERIC_SET_EXPVAL( t, v )    ( (t)->sebyte = ((t)->sebyte & 0x80) | ((v) & 0x7F) )

#define IDA_TNUMERIC_SET_COMP_EXP( t1, t2 ) ( (t1)->sebyte = ((t1)->sebyte & 0x80) | (0x80 - ((t2)->sebyte & 0x7F)) )
 
#define IDA_TNUMERIC_EXP( t )          ( (t)->sebyte & 0x7F )

/**
 * Boolean Values
 */
typedef enum idaBoolean
{
    IDA_FALSE = 0,  /** FALSE */
    IDA_TRUE  = 1,  /** TRUE */
    IDA_NULL  = 2   /** NULL */
} idaBoolean;

/**
 * Comparison Results
 */
typedef enum idaCompareType
{
    idaCompareError   = -1,     /** Error on Comparison */
    idaCompareGreater = 0,      /** >= */
    idaCompareEqual   = 1,      /** == */
    idaCompareLess    = 2,      /** <= */
    idaCompareNull    = 3       /** Null in Either of the Operands */
}idaCompareType;

/**
 * Ordering Type of Sort
 */
typedef enum idaOrderType
{
    idaOrderAscending,      /** Ascending Order */
    idaOrderDescending      /** Descending Order */
}idaOrderType;

/**
 * Boolean Type
 */
typedef struct idaBooleanType
{
    idaBoolean value; /** Value */
}
idaBooleanType;

/**
 * Character Type
 */
typedef struct idaCharType
{
    UChar value[1]; /** Value */
}
idaCharType;

/**
 * Variable Character Type
 */
typedef struct idaVarcharType
{
    UChar length[2];    /** Length of Value */
    UChar value[1];     /** String Value */
}
idaVarcharType;

/**
 * Bitmap Type
 */
typedef struct idaBitType
{
    UChar length[2];    /** Size of Value */
    UChar value[1];     /** Bitmap Value */
}
idaBitType;

/**
 * Integer Type
 */
typedef SInt idaIntegerType;

/**
 * Numeric Type
 */
typedef struct idaNumericType
{
    UChar sebyte;       /** Sign and Exponent */
    UChar mantissa[1];  /** Mantissa */
}
idaNumericType;

/**
 * Numeric Type
 */
typedef struct idaTNumericType
{
    UChar sebyte;           /** Sign and Exponent */
    UChar mantissa[20];     /** Mantissa */
}
idaTNumericType;

/**
 * Date Type
 */
typedef struct idaDateType
{
    UChar year[2];          /** Year */
    UChar month;            /** Month */
    UChar day;              /** Date of Month */
    UChar hour;             /** Hour */
    UChar minute;           /** Minute */
    UChar second;           /** Second */
    UChar microsecond[3];   /** Microsecond */
}
idaDateType;

/**
 * Timestamp Type
 */
typedef struct idaTimestampType
{
    UChar second[8];
    UChar microsecond[8];
}
idaTimestampType;

/**
 * Nibble Type
 */
typedef struct idaNibbleType
{
    UChar length;
    UChar value[1];
}
idaNibbleType;

/**
 * Bytes Type
 */
typedef struct idaBytesType
{
    UChar value[1];
}
idaBytesType;

// idaConst.cpp ( *** )
extern const idaCharType      idaCharNULL;
extern const idaVarcharType   idaVarcharNULL;
extern const idaBitType       idaBitNULL;
extern const idaNumericType   idaNumericNULL;
extern const idaTNumericType  idaTNumericNULL;
extern const idaDateType      idaDateNULL;
extern const idaTimestampType idaTimestampNULL;
extern const idaNibbleType    idaNibbleNULL;
extern const SInt             idaIntegerNULL;
extern const idaIntegerType   idaIntegerZERO;
extern const idaTNumericType  idaTNumericZERO;
extern const idaTNumericType  idaTNumericONE;
extern const idaTNumericType  idaTNumericTWELVE;
extern const idaNibbleType    idaNibbleNULL;
extern const UChar            idaByteBuffer255[256];

// idaCTypes.cpp ( ** )
SInt idaFloat   ( SFloat*  l, const idaTNumericType* t );
SInt idaDouble  ( SDouble* l, const idaTNumericType* t );
SInt idaSChar   ( SChar*   l, const idaTNumericType* t );
SInt idaUChar   ( UChar*   l, const idaTNumericType* t );
SInt idaSShort  ( SShort*  l, const idaTNumericType* t );
SInt idaUShort  ( UShort*  l, const idaTNumericType* t );
SInt idaSInt    ( SInt*    l, const idaTNumericType* t );
// added by leekmo
// TODO : IDDT_INT doesn't know SInt, UInt, SLong, ULong
SInt idaSInt    ( SInt*   l, const idaNumericType* nu, SInt n2Precision, SInt n2Scale );
SInt idaSInt    ( SInt*   l, const idaNumericType* nu, SInt n2Length );

SInt idaUInt    ( UInt*   l, const idaTNumericType* t );
SInt idaSLong   ( SLong*  l, const idaTNumericType* t );
SInt idaULong   ( ULong*  l, const idaTNumericType* t );
SInt idaTM      ( struct tm *timer,      const idaDateType* d );
SInt idaTimeT   ( time_t *timer,         const idaDateType* d );
SInt idaTimeval ( struct timeval *timer, const idaDateType* d );

// idaTNumeric.cpp ( *** )
SInt idaTNumeric( idaTNumericType* t, SLong  l );
SInt idaTNumeric( idaTNumericType* t, ULong  l );
SInt idaTNumeric( idaTNumericType* t1, const idaNumericType* n2, SInt n2Length );
SInt idaTNumeric( idaTNumericType* t1, const UChar* str, UShort length );

// idaVarchar.cpp ( **** )
SInt idaVarchar( idaVarcharType* v1, UShort v1Max, const idaTNumericType* t2 );
SInt idaVarcharExact( idaVarcharType* v1, UShort v1Max, const idaNumericType* n2, SInt n2Length );
SInt idaVarcharExact( idaVarcharType* v1, UShort v1Max, const idaTNumericType* t2 );

// idaDate.cpp (****)
SInt idaDate( idaDateType* d, const UChar*  string, UShort length );

// idaCompare.cpp ( *** )
SInt idaGreaterEqual ( idaBooleanType* b1, const idaTNumericType* t2, const idaTNumericType* t3 );
SInt idaLessEqual    ( idaBooleanType* b1, const idaTNumericType* t2, const idaTNumericType* t3 );

// idaOperator.cpp ( **** )
SInt idaAdd      ( idaTNumericType* t1, const idaTNumericType* t2, const idaTNumericType* t3 );
SInt idaSubtract ( idaTNumericType* t1, const idaTNumericType* t2, const idaTNumericType* t3 );
SInt idaMultiply ( idaTNumericType* t1, const idaTNumericType* t2, const idaTNumericType* t3 );
SInt idaDivide   ( idaTNumericType* t1, const idaTNumericType* t2, const idaTNumericType* t3 );
SInt idaMod      ( idaTNumericType* t1, const idaTNumericType* t2, const idaTNumericType* t3 );

// idaTNumericMisc.cpp ( *** )
SInt idaSign  ( idaTNumericType* t1, const idaTNumericType* t2 );
SInt idaMinus ( idaTNumericType* t1, const idaTNumericType* t2 );
SInt idaAbs   ( idaTNumericType* t1, const idaTNumericType* t2 );
SInt idaCeil  ( idaTNumericType* t1, const idaTNumericType* t2 );
SInt idaFloor ( idaTNumericType* t1, const idaTNumericType* t2 );
SInt idaRound ( idaTNumericType* t1, const idaTNumericType* t2, const idaTNumericType* t3 );
SInt idaTrunc ( idaTNumericType* t1, const idaTNumericType* t2, const idaTNumericType* t3 );

// idaXa.cpp
IDE_RC idaXaXidToString(
    ID_XID          * aXid,
    vULong          * aFormatId,
    SChar           * aGlobalTxId,
    SChar           * aBranchQualifier );
void idaXaStringToXid(
    vULong            aFormatId,
    SChar           * aGlobalTxId,
    SChar           * aBranchQualifier,
    ID_XID          * aXid);
UInt idaXaConvertXIDToString( void        *aBaseObj,
                              void        *aMember,
                              UChar       *aBuf,
                              UInt         aBufSize);
UInt idaXaConvertGlobalXIDToString( void        *aBaseObj,
                                    void        *aMember,
                                    UChar       *aBuf,
                                    UInt         aBufSize);

#endif /* _O_IDA_H_ */


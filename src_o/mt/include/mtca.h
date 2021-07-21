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
 * $Id$
 **********************************************************************/

#ifndef _O_MTCA_H_
# define  _O_MTCA_H_  1

#include <acpTypes.h>

typedef enum mtaBoolean
{
    MTA_FALSE = 0,
    MTA_TRUE  = 1,
    MTA_NULL  = 2
} mtaBoolean;

typedef struct mtaBooleanType
{
    mtaBoolean value;
}
mtaBooleanType;

typedef struct mtaVarcharType
{
    acp_uint8_t length[2];
    acp_uint8_t value[1];
}
mtaVarcharType;

typedef struct mtaNumericType
{
    acp_uint8_t sebyte;
    acp_uint8_t mantissa[1];
}
mtaNumericType;

typedef struct mtaTNumericType
{
    acp_uint8_t sebyte;
    acp_uint8_t mantissa[20];
}
mtaTNumericType;

#define MTA_VARCHAR_MAX                ( 4000 )
#define MTA_NUMERIC_PRECISION_MAX      ( (sizeof(mtaTNumericType)-2)*2 )
#define MTA_NUMERIC_PRECISION_MIN      ( 1 )
#define MTA_NUMERIC_SCALE_MAX          ( 126 )
#define MTA_NUMERIC_SCALE_MIN          ( -((acp_sint64_t)126)+((acp_sint64_t)(sizeof(mtaTNumericType)*2)) )

#define MTA_GET_LENGTH( x )            ( (((x)[0])<<8) | ((x)[1]) )
#define MTA_SET_LENGTH( x, y ) \
{                               \
    (x)[0] = (y)>>8;            \
    (x)[1] = (y)&0xFF;          \
}

#define MTA_GET_VARCHAR_SIZE( l )    ( l + sizeof(mtaVarcharType) - 1 )
#define MTA_GET_NUMERIC_SIZE( p, s ) ( 1+((p)/2)+((((p)%2)||((s)%2))?1:0) )
#define MTA_GET_BOOLEAN_SIZE         ( sizeof(mtaBooleanType) )
#define MTA_GET_TNUMERIC_SIZE        ( sizeof(mtaTNumericType) )

#define MTA_BOOLEAN_IS_NULL( b )       ( (b)->value == MTA_NULL )
#define MTA_VARCHAR_IS_NULL( v )       ( (v)->length[0] == 0 && (v)->length[1] == 0 )
#define MTA_NUMERIC_IS_NULL( n )       ( (n)->sebyte == 0 )
#define MTA_TNUMERIC_IS_NULL( t )      ( (t)->sebyte == 0 )

#define MTA_NUMERIC_IS_ZERO( n )       ( (n)->sebyte == 0x80 )
#define MTA_TNUMERIC_IS_ZERO( t )      ( (t)->sebyte == 0x80 )
#define MTA_TNUMERIC_IS_ONE( t )       ( idlOS::memcmp((t),&mtaTNumericONE,sizeof(mtaTNumericType)) ? 0 : 1 )

#define MTA_NUMERIC_IS_POSITIVE( n )      ((((n)->sebyte & 0x80) == 0x80)?1:0)
#define MTA_TNUMERIC_IS_POSITIVE( t )     ((((t)->sebyte & 0x80) == 0x80)?1:0)

#define MTA_NUMERIC_IS_NEGATIVE( n )      ((((n)->sebyte & 0x80) == 0)?1:0)
#define MTA_TNUMERIC_IS_NEGATIVE( t )     ((((t)->sebyte & 0x80) == 0)?1:0)

#define MTA_TNUMERIC_EQUAL_SIGN(t1,t2)     ( ((t1)->sebyte & 0x80) == ((t2)->sebyte & 0x80))

#define MTA_NUMERIC_SET_POSITIVE( n )      ((n)->sebyte = ((n)->sebyte | 0x80))
#define MTA_TNUMERIC_SET_POSITIVE( t )     ((t)->sebyte = ((t)->sebyte | 0x80))

#define MTA_NUMERIC_SET_ABS_POSITIVE( n )      ((n)->sebyte = 0x80)
#define MTA_TNUMERIC_SET_ABS_POSITIVE( t )     ((t)->sebyte = 0x80)

#define MTA_NUMERIC_SET_NEGAVIVE( n )      ((n)->sebyte = ((n)->sebyte & 0x7F))
#define MTA_TNUMERIC_SET_NEGATIVE( t )     ((t)->sebyte = ((t)->sebyte & 0x7F))

#define MTA_NUMERIC_SET_ABS_NEGAVIVE( n )      ((n)->sebyte = 0)
#define MTA_TNUMERIC_SET_ABS_NEGATIVE( t )     ((t)->sebyte = 0)

#define MTA_TNUMERIC_SET_SEBYTEVAL( t1, t2, expval )    ( (t1)->sebyte = ((t2)->sebyte & 0x80) | (expval) )
#define MTA_TNUMERIC_SET_SEBYTE2(t1,t2)     ( (t1)->sebyte = (t2)->sebyte )
#define MTA_TNUMERIC_SET_SEBYTECOMP2(t1,t2) ( (t1)->sebyte = ( (t2)->sebyte & 0x80 ) | (0x80 - ((t2)->sebyte & 0x7F)) )
#define MTA_TNUMERIC_SET_ABS_SIGN(t1,t2)    ( (t1)->sebyte = ((t2)->sebyte & 0x80) )
#define MTA_TNUMERIC_SET_SIGN( t1, t2 )    ( (t1)->sebyte = ((t1)->sebyte & 0x7F) | ((t2)->sebyte & 0x80) )

#define MTA_TNUMERIC_SET_EXP( t1, t2 )     ( (t1)->sebyte = ((t1)->sebyte & 0x80) | ((t2)->sebyte & 0x7F) )

#define MTA_TNUMERIC_SET_EXPVAL( t, v )    ( (t)->sebyte = ((t)->sebyte & 0x80) | ((v) & 0x7F) )

#define MTA_TNUMERIC_SET_COMP_EXP( t1, t2 ) ( (t1)->sebyte = ((t1)->sebyte & 0x80) | (0x80 - ((t2)->sebyte & 0x7F)) )
 
#define MTA_TNUMERIC_EXP( t )          ( (t)->sebyte & 0x7F )

// mtaConst.c ( *** )
extern const mtaVarcharType   mtaVarcharNULL;
extern const mtaNumericType   mtaNumericNULL;
extern const mtaTNumericType  mtaTNumericNULL;
extern const mtaTNumericType  mtaTNumericZERO;
extern const mtaTNumericType  mtaTNumericONE;
extern const mtaTNumericType  mtaTNumericTWELVE;

// mtaCTypes.c ( ** )
acp_sint32_t mtaSint64( acp_sint64_t*  l, const mtaTNumericType* t );
acp_sint32_t mtaUint64( acp_uint64_t*  l, const mtaTNumericType* t );

// mtaTNumeric.c ( *** )
acp_sint32_t mtaTNumeric( mtaTNumericType* t, acp_sint64_t  l );
acp_sint32_t mtaTNumeric2( mtaTNumericType* t, acp_uint64_t  l );
acp_sint32_t mtaTNumeric3( mtaTNumericType* t1, const mtaNumericType* n2, acp_sint32_t n2Length );
acp_sint32_t mtaTNumeric4( mtaTNumericType* t1, const acp_uint8_t* str, acp_uint16_t length );

// mtaVarchar.c ( **** )
acp_sint32_t mtaVarchar( mtaVarcharType* v1, acp_uint16_t v1Max, const mtaTNumericType* t2 );
acp_sint32_t mtaVarcharExact( mtaVarcharType* v1, acp_uint16_t v1Max, const mtaNumericType* n2, acp_sint32_t n2Length );
acp_sint32_t mtaVarcharExact2( mtaVarcharType* v1, acp_uint16_t v1Max, const mtaTNumericType* t2 );

// mtaCompare.c ( *** )
acp_sint32_t mtaGreaterEqual ( mtaBooleanType* b1, const mtaTNumericType* t2, const mtaTNumericType* t3 );
acp_sint32_t mtaLessEqual    ( mtaBooleanType* b1, const mtaTNumericType* t2, const mtaTNumericType* t3 );

// mtaOperator.c ( **** )
acp_sint32_t mtaAdd      ( mtaTNumericType* t1, const mtaTNumericType* t2, const mtaTNumericType* t3 );
acp_sint32_t mtaSubtract ( mtaTNumericType* t1, const mtaTNumericType* t2, const mtaTNumericType* t3 );
acp_sint32_t mtaMultiply ( mtaTNumericType* t1, const mtaTNumericType* t2, const mtaTNumericType* t3 );
acp_sint32_t mtaDivide   ( mtaTNumericType* t1, const mtaTNumericType* t2, const mtaTNumericType* t3 );
acp_sint32_t mtaMod      ( mtaTNumericType* t1, const mtaTNumericType* t2, const mtaTNumericType* t3 );

// mtaTNumericMisc.c ( *** )
acp_sint32_t mtaSign  ( mtaTNumericType* t1, const mtaTNumericType* t2 );
acp_sint32_t mtaMinus ( mtaTNumericType* t1, const mtaTNumericType* t2 );
acp_sint32_t mtaAbs   ( mtaTNumericType* t1, const mtaTNumericType* t2 );
acp_sint32_t mtaCeil  ( mtaTNumericType* t1, const mtaTNumericType* t2 );
acp_sint32_t mtaFloor ( mtaTNumericType* t1, const mtaTNumericType* t2 );
acp_sint32_t mtaRound ( mtaTNumericType* t1, const mtaTNumericType* t2, const mtaTNumericType* t3 );
acp_sint32_t mtaTrunc ( mtaTNumericType* t1, const mtaTNumericType* t2, const mtaTNumericType* t3 );

#endif /* _O_MTCA_H_ */


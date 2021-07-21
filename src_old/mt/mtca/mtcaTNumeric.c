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
 * $Log$
 * Revision 1.5  2006/05/16 00:23:36  sjkim
 * TASK-1786
 *
 * Revision 1.4.102.2  2006/04/12 05:47:29  sjkim
 * TASK-1786
 *
 * Revision 1.4.102.1  2006/04/10 06:28:32  sjkim
 * TASK-1786
 *
 * Revision 1.4  2003/12/17 01:33:14  jdlee
 * remove unused functions
 *
 * Revision 1.3  2003/02/10 05:34:26  assam
 * fix PR-2815, PR-3412
 *
 * Revision 1.2  2002/12/17 07:54:05  jdlee
 * remove
 *
 * Revision 1.1  2002/12/17 05:43:23  jdlee
 * create
 *
 * Revision 1.1.1.1  2002/10/18 13:55:36  jdlee
 * create
 *
 * Revision 1.20  2002/04/30 05:35:25  assam
 * *** empty log message ***
 *
 * Revision 1.19  2002/04/30 01:50:04  mylee
 * delete comment
 *
 * Revision 1.18  2002/04/29 11:25:05  assam
 * *** empty log message ***
 *
 * Revision 1.17  2002/04/29 11:24:36  assam
 * *** empty log message ***
 *
 * Revision 1.16  2002/01/14 08:30:25  jdlee
 * fix PR0-1730
 *
 * Revision 1.15  2001/07/07 06:24:39  jdlee
 * fix PR-1222
 *
 * Revision 1.14  2001/06/21 03:15:19  jdlee
 * fix PR-1217
 *
 * Revision 1.13  2001/03/27 12:29:21  jdlee
 * remove dummy ACI_CLEAR
 *
 * Revision 1.12  2001/03/02 07:56:04  kmkim
 * 1st version of WIN32 porting
 *
 * Revision 1.11.6.1  2001/02/21 09:00:01  kmkim
 * win32 porting - 컴파일만 성공
 *
 * Revision 1.11  2000/12/22 03:47:50  jdlee
 * fix
 *
 * Revision 1.10  2000/10/31 13:10:27  assam
 * *** empty log message ***
 *
 * Revision 1.9  2000/10/12 03:19:49  sjkim
 * func call fix
 *
 * Revision 1.8  2000/10/11 10:04:03  sjkim
 * trace info added
 *
 * Revision 1.7  2000/09/30 06:57:29  sjkim
 * error code fix
 *
 * Revision 1.6  2000/09/18 09:24:56  jdlee
 * refine spi interfaces
 *
 * Revision 1.5  2000/07/13 08:25:26  sjkim
 * error code fatal->abort
 *
 * Revision 1.4  2000/06/16 02:50:56  jdlee
 * fix naming
 *
 * Revision 1.3  2000/06/15 03:57:30  assam
 * ideErrorMgr changed.
 *
 * Revision 1.2  2000/06/07 13:19:23  jdlee
 * fix
 *
 * Revision 1.1.1.1  2000/06/07 11:44:06  jdlee
 * init
 *
 * Revision 1.1  2000/05/31 11:14:32  jdlee
 * change naming to id
 *
 * Revision 1.1  2000/05/31 07:33:57  sjkim
 * id 라이브러리 생성
 *
 * Revision 1.8  2000/05/22 10:12:26  sjkim
 * 에러 처리 루틴 수정 : ACTION 추가
 *
 * Revision 1.7  2000/04/20 07:25:43  assam
 * idaDate added.
 *
 * Revision 1.6  2000/03/23 11:43:41  assam
 * *** empty log message ***
 *
 * Revision 1.5  2000/03/22 08:46:10  assam
 * *** empty log message ***
 *
 * Revision 1.4  2000/03/15 01:52:18  assam
 * *** empty log message ***
 *
 * Revision 1.3  2000/03/06 04:20:49  assam
 * *** empty log message ***
 *
 * Revision 1.2  2000/02/25 03:42:10  assam
 * *** empty log message ***
 *
 * Revision 1.1  2000/02/24 01:10:39  assam
 * *** empty log message ***
 *
 * Revision 1.6  2000/02/21 09:34:19  assam
 * *** empty log message ***
 *
 * Revision 1.5  2000/02/21 07:11:20  assam
 * *** empty log message ***
 *
 * Revision 1.4  2000/02/18 04:19:28  assam
 * *** empty log message ***
 *
 * Revision 1.3  2000/02/18 01:35:45  assam
 * 64-bit incompatibility removed.
 *
 * Revision 1.2  2000/02/17 09:37:18  assam
 * *** empty log message ***
 *
 * Revision 1.1  2000/02/17 03:17:32  assam
 * Migration from QP.
 *
 **********************************************************************/

#include <mtca.h>
#include <mtce.h>
#include <mtcn.h>

#define MTA_SET_MANTISSA( m, i, c )                     \
    if( (i) < (acp_sint16_t)(sizeof(m)*2) ) {           \
        if( (i)%2 ) {                                   \
            (m)[(i)/2] += (acp_uint8_t)((c)-0x30);      \
        } else {                                        \
            (m)[(i)/2] = (acp_uint8_t)((c)-0x30)*10;    \
        }                                               \
        (i)++;                                          \
    } else {                                            \
        if( (i) == (acp_sint16_t)(sizeof(m)*2) ) {      \
            if( ((c)-0x30) >= 5 ) {                     \
                round = 1;                              \
            }                                           \
        }                                               \
        (i)++;                                          \
    }

acp_sint32_t mtaTNumeric( mtaTNumericType* t, acp_sint64_t l )
{
    if( l < 0 ){
        if( l == -ACP_SINT64_LITERAL(9223372036854775807)-ACP_SINT64_LITERAL(1) ){
            mtaTNumeric( t, (acp_uint64_t)(ACP_UINT64_LITERAL(9223372036854775807) + ACP_UINT64_LITERAL(1)));
        }else{
            mtaTNumeric( t, (acp_uint64_t)(-l) );
        }
        MTA_TNUMERIC_SET_NEGATIVE(t);
    }else{
        mtaTNumeric( t, (acp_uint64_t)l );
    }
	
    return ACI_SUCCESS;
}

acp_sint32_t mtaTNumeric2( mtaTNumericType* t, acp_uint64_t l )
{
    acp_sint16_t mantissaIndex;
    acp_sint16_t exponent;
    
    *t = mtaTNumericZERO;
    if( l != (acp_uint64_t)0 ){
        for( mantissaIndex = 9; mantissaIndex >= 0; mantissaIndex-- ){
            t->mantissa[mantissaIndex] = (acp_uint8_t)(l%100);
            l /= 100;
        }
        for( exponent = 0; exponent < 10; exponent++ ){
            if( t->mantissa[exponent] != 0 ){
                break;
            }
        }
        for( mantissaIndex = 0; mantissaIndex+exponent < 10 ;mantissaIndex++ ){
            t->mantissa[mantissaIndex] = t->mantissa[mantissaIndex+exponent];
        }
        for( ; mantissaIndex < (acp_sint16_t)sizeof(t->mantissa); mantissaIndex++ ){
            t->mantissa[mantissaIndex] = 0;
        }
        exponent = 10-exponent;
        MTA_TNUMERIC_SET_EXPVAL(t,exponent+64);
    }

    return ACI_SUCCESS;
}

acp_sint32_t mtaTNumeric3( mtaTNumericType* t1, const mtaNumericType* n2, acp_sint32_t n2Length )
{
    acp_sint16_t         mantissaIndex;
    acp_sint16_t         mantissaEnd;
    
    ACI_TEST_RAISE( n2Length < 0 || (size_t)n2Length > sizeof( mtaTNumericType ), ERR_INVALID_NUMERIC );
    if( n2Length == 0 )
    {
        *t1 = mtaTNumericNULL;
    }
    else
        if( MTA_NUMERIC_IS_NULL( n2 ) ){
            *t1 = mtaTNumericNULL;
        }else{
            if( MTA_NUMERIC_IS_ZERO( n2 ) ){
                *t1 = mtaTNumericZERO;
            }else{
                mantissaEnd = n2Length - 1;
                if( MTA_NUMERIC_IS_POSITIVE(n2) ){
                    MTA_TNUMERIC_SET_ABS_POSITIVE(t1);
                    MTA_TNUMERIC_SET_EXP(t1,n2);
                    for( mantissaIndex = 0; mantissaIndex < mantissaEnd; mantissaIndex++ ){
                        t1->mantissa[mantissaIndex] = n2->mantissa[mantissaIndex];
                    }
                    for( ; mantissaIndex < (acp_sint16_t)sizeof(t1->mantissa); mantissaIndex++ ){
                        t1->mantissa[mantissaIndex] = 0;
                    }
                }else{
                    MTA_TNUMERIC_SET_ABS_NEGATIVE(t1);
                    MTA_TNUMERIC_SET_COMP_EXP(t1,n2);	
                    for( mantissaIndex = 0; mantissaIndex < mantissaEnd; mantissaIndex++ ){
                        t1->mantissa[mantissaIndex] = 99-n2->mantissa[mantissaIndex];
                    }
                    for( ; mantissaIndex < (acp_sint16_t)sizeof(t1->mantissa); mantissaIndex++ ){
                        t1->mantissa[mantissaIndex] = 0;
                    }
                }
            }
        }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_NUMERIC );
    aciSetErrorCode( mtERR_ABORT_INVALID_NUMERIC );
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

acp_sint32_t mtaTNumeric4( mtaTNumericType* t1, const acp_uint8_t* str, acp_uint16_t length )
{
    mtnChar  c;
    mtnIndex index;
    acp_uint16_t   round;
    acp_sint16_t   period;
    acp_sint16_t   exponent;
    acp_sint16_t   exponentSign;
    acp_sint16_t   mantissaIndex;

    round         = 0;
    period        = 0;
    exponent      = 0;
    exponentSign  = 1;
    mantissaIndex = 0;
    mtnSkip( str, length, &index, 0 );
    do{
        if( mtnAsciiAt( &c, str, length, &index ) != ACI_SUCCESS ){
            ACI_RAISE( ERR_INVALID_NUMERIC );
        }
    }while( c == 0x20 ); // Space
    MTA_TNUMERIC_SET_ABS_POSITIVE(t1);
    if( c == 0x2B || c == 0x2D ){  // Plus & Minus sign
        if( c == 0x2D ){  // Minus sign
            MTA_TNUMERIC_SET_NEGATIVE(t1);
        }
        if( mtnAsciiAt( &c, str, length, &index ) != ACI_SUCCESS ){
            ACI_RAISE( ERR_INVALID_NUMERIC );
        }
    }
    if( c == 0x2E ){  // Period
        if( mtnAsciiAt( &c, str, length, &index ) != ACI_SUCCESS ){
            ACI_RAISE( ERR_INVALID_NUMERIC );
        }
        if( c < 0x30 || c > 0x39 ){ // Numeric
            ACI_RAISE( ERR_INVALID_NUMERIC );
        }
        while( c == 0x30 ){ // Zero
            period--;
            if( mtnAsciiAt( &c, str, length, &index ) != ACI_SUCCESS ){
                goto final;
            }
        }
        while( c >= 0x30 && c <= 0x39 ){ // Numeric
            MTA_SET_MANTISSA( t1->mantissa, mantissaIndex, c );
            if( mtnAsciiAt( &c, str, length, &index ) != ACI_SUCCESS ){
                goto final;
            }
        }
    }else{
        if( c < 0x30 || c > 0x39 ){ // Numeric
            ACI_RAISE( ERR_INVALID_NUMERIC );
        }
        while( c == 0x30 ){ // Zero
            if( mtnAsciiAt( &c, str, length, &index ) != ACI_SUCCESS ){
                goto final;
            }	    
        }
        if( c == 0x2E ){
            if( mtnAsciiAt( &c, str, length, &index ) != ACI_SUCCESS ){
                goto final;
            }
            while( c == 0x30 ){ // Zero
                period--;
                if( mtnAsciiAt( &c, str, length, &index ) != ACI_SUCCESS ){
                    goto final;
                }	    
            }
            while( c >= 0x30 && c <= 0x39 ){ // Numeric
                MTA_SET_MANTISSA( t1->mantissa, mantissaIndex, c );
                if( mtnAsciiAt( &c, str, length, &index ) != ACI_SUCCESS ){
                    goto final;
                }
            }
        }else{
            if( c < 0x30 || c > 0x39 ){ // Numeric
                ACI_RAISE( ERR_INVALID_NUMERIC );
            }
            while( c >= 0x30 && c <= 0x39 ){ // Numeric
                period++;
                MTA_SET_MANTISSA( t1->mantissa, mantissaIndex, c );
                if( mtnAsciiAt( &c, str, length, &index ) != ACI_SUCCESS ){
                    goto final;
                }
            }
            if( c == 0x2E ){ // Period
                if( mtnAsciiAt( &c, str, length, &index ) != ACI_SUCCESS ){
                    goto final;
                }
                while( c >= 0x30 && c <= 0x39 ){ // Numeric
                    MTA_SET_MANTISSA( t1->mantissa, mantissaIndex, c );
                    if( mtnAsciiAt( &c, str, length, &index ) != ACI_SUCCESS ){
                        goto final;
                    }
                }
            }
        }
    }
    if( c == 0x45 || c == 0x65 ){ // 'E' or 'e'
        if( mtnAsciiAt( &c, str, length, &index ) != ACI_SUCCESS ){
            ACI_RAISE( ERR_INVALID_NUMERIC );
        }
        if( c == 0x2B || c == 0x2D ){  // Plus & Minus sign
            if( c == 0x2D ){ // Minus sign
                exponentSign = -1;
            }
            if( mtnAsciiAt( &c, str, length, &index ) != ACI_SUCCESS ){
                ACI_RAISE( ERR_INVALID_NUMERIC );
            }
        }
        if( c < 0x30 || c > 0x39 ){ // Numeric
            ACI_RAISE( ERR_INVALID_NUMERIC );
        }
        while( c >= 0x30 && c <= 0x39 ){ // Numeric
            if( exponent <= 3276 )
            {
                exponent = (acp_sint16_t)( exponent * 10 + ( c - 0x30 ) );
            }
            if( mtnAsciiAt( &c, str, length, &index ) != ACI_SUCCESS ){
                goto final;
            }
        }
    }
    while( c == 0x20 ){
        if( mtnAsciiAt( &c, str, length, &index ) != ACI_SUCCESS ){
            goto final;
        }
    }
    ACI_RAISE( ERR_INVALID_NUMERIC );
    
  final:
    if( mantissaIndex == 0 ){
        MTA_TNUMERIC_SET_ABS_POSITIVE(t1);
        MTA_TNUMERIC_SET_EXPVAL(t1,0);
        acpMemSet( t1->mantissa, 0, sizeof(t1->mantissa) );
    }else{
        while( mantissaIndex < (acp_sint16_t)(sizeof(t1->mantissa)*2) ){
            MTA_SET_MANTISSA( t1->mantissa, mantissaIndex, 0x30 );
        }
        exponent *= exponentSign;
        exponent += period;
        if( exponent%2 ){
            round = 0;
            if( (t1->mantissa[sizeof(t1->mantissa)-1]%10) >= 5 ){
                round = 1;
            }
            for( mantissaIndex = (sizeof(t1->mantissa))-1; mantissaIndex > 0; mantissaIndex-- ){
                t1->mantissa[mantissaIndex] = (t1->mantissa[mantissaIndex-1]%10)*10+t1->mantissa[mantissaIndex]/10;
            }
            t1->mantissa[0] /= 10;
            exponent += 1;
        }
        if( round ){
            for( mantissaIndex = (sizeof(t1->mantissa))-1; mantissaIndex >= 0; mantissaIndex-- ){
                t1->mantissa[mantissaIndex]++;
                if( t1->mantissa[mantissaIndex] < 100 ){
                    break;
                }
                t1->mantissa[mantissaIndex] = 0;
            }
            if( t1->mantissa[0] == 0 ){
                for( mantissaIndex = (sizeof(t1->mantissa))-1; mantissaIndex > 0; mantissaIndex-- ){
                    t1->mantissa[mantissaIndex] = t1->mantissa[mantissaIndex-1];
                }
                t1->mantissa[0] = 1;
                exponent += 2;
            }
        }
        exponent /= 2;
        ACI_TEST_RAISE( exponent > 63, ERR_OVERFLOW );
        if( exponent < -63 ){
            MTA_TNUMERIC_SET_ABS_POSITIVE(t1);
            MTA_TNUMERIC_SET_EXPVAL(t1,0);
            acpMemSet( t1->mantissa, 0, sizeof(t1->mantissa) );
        }else{
            MTA_TNUMERIC_SET_EXPVAL(t1,exponent+64);
        }
    }

    ACI_CLEAR();    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_NUMERIC );
    aciSetErrorCode( mtERR_ABORT_INVALID_NUMERIC );
    
    ACI_EXCEPTION( ERR_OVERFLOW );
    aciSetErrorCode( mtERR_ABORT_OVERFLOW );
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}


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
 * Revision 1.2  2005/08/31 00:40:40  unclee
 * fix BUG-12731.
 *
 * Revision 1.1.98.1  2005/08/30 00:26:59  unclee
 * fix BUG-12731
 *
 * Revision 1.1  2002/12/17 05:43:23  jdlee
 * create
 *
 * Revision 1.1.1.1  2002/10/18 13:55:36  jdlee
 * create
 *
 * Revision 1.17  2002/05/21 01:29:17  assam
 * *** empty log message ***
 *
 * Revision 1.16  2001/06/21 03:15:19  jdlee
 * fix PR-1217
 *
 * Revision 1.15  2001/03/27 12:29:21  jdlee
 * remove dummy ACI_CLEAR
 *
 * Revision 1.14  2000/12/22 03:47:50  jdlee
 * fix
 *
 * Revision 1.13  2000/10/31 13:10:27  assam
 * *** empty log message ***
 *
 * Revision 1.12  2000/10/12 03:19:49  sjkim
 * func call fix
 *
 * Revision 1.11  2000/10/11 10:04:03  sjkim
 * trace info added
 *
 * Revision 1.10  2000/09/30 06:57:29  sjkim
 * error code fix
 *
 * Revision 1.9  2000/08/17 08:48:17  sjkim
 * hp fix
 *
 * Revision 1.8  2000/08/16 10:58:37  sjkim
 * fix hp
 *
 * Revision 1.6  2000/08/16 09:12:05  sjkim
 * fix hp
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
 * Revision 1.5  2000/05/22 10:12:26  sjkim
 * 에러 처리 루틴 수정 : ACTION 추가
 *
 * Revision 1.4  2000/04/19 06:40:23  assam
 * mod added.
 *
 * Revision 1.3  2000/04/11 01:30:41  assam
 * idaSign Fixed.
 *
 * Revision 1.2  2000/03/22 08:46:10  assam
 * *** empty log message ***
 *
 * Revision 1.1  2000/02/24 01:10:39  assam
 * *** empty log message ***
 *
 * Revision 1.2  2000/02/21 12:14:14  assam
 * *** empty log message ***
 *
 * Revision 1.1  2000/02/17 03:17:32  assam
 * Migration from QP.
 *
 **********************************************************************/

#include <mtca.h>
#include <mtce.h>

acp_sint32_t mtaSign( mtaTNumericType* t1, const mtaTNumericType* t2 )
{
    if( MTA_TNUMERIC_IS_NULL( t2 ) ){
        *t1 = mtaTNumericNULL;
    }else{
        *t1 = mtaTNumericZERO;
        if( !MTA_TNUMERIC_IS_ZERO( t2 ) ){
            if( MTA_TNUMERIC_IS_POSITIVE(t2) ){
                MTA_TNUMERIC_SET_POSITIVE(t1);
            }else{
                MTA_TNUMERIC_SET_NEGATIVE(t1);
            }
            MTA_TNUMERIC_SET_EXPVAL(t1,1+64);
            t1->mantissa[0] = 1;
        }
    }
    
    return ACI_SUCCESS;
}

acp_sint32_t mtaMinus( mtaTNumericType* t1, const mtaTNumericType* t2 )
{
    if( MTA_TNUMERIC_IS_NULL( t2 ) ){
        *t1 = mtaTNumericNULL;
    }else{
        *t1 = *t2;
        if( !MTA_TNUMERIC_IS_ZERO( t2 ) ){
            if( MTA_TNUMERIC_IS_POSITIVE(t1) ){
                MTA_TNUMERIC_SET_NEGATIVE(t1);
            }else{
                MTA_TNUMERIC_SET_POSITIVE(t1);
            }
        }
    }
    
    return ACI_SUCCESS;
}

acp_sint32_t mtaAbs( mtaTNumericType* t1, const mtaTNumericType* t2 )
{
    if( MTA_TNUMERIC_IS_NULL( t2 ) ){
        *t1 = mtaTNumericNULL;
    }else{
        *t1 = *t2;
        MTA_TNUMERIC_SET_POSITIVE(t1);
    }

    return ACI_SUCCESS;
}

acp_sint32_t mtaCeil( mtaTNumericType* t1, const mtaTNumericType* t2 )
{
    acp_sint16_t exponent;
    acp_sint16_t mantissaIndex;
    acp_uint16_t carry;
    acp_bool_t   sHasFraction;
    acp_sint32_t i;
    
    if( MTA_TNUMERIC_IS_NULL( t2 ) ){
        *t1 = mtaTNumericNULL;
    }else{
        *t1 = *t2;
        if( !MTA_TNUMERIC_IS_ZERO( t2 ) ){
            exponent = ( ((acp_sint16_t)(MTA_TNUMERIC_EXP(t1))) - 64 );
            if( MTA_TNUMERIC_IS_POSITIVE(t1) ){
                if( exponent < 0 ){
                    *t1 = mtaTNumericZERO;
                    exponent = 1;
                    t1->mantissa[0] = 1;
                }
                else
                {
                    if( exponent < (acp_sint16_t)sizeof(t1->mantissa) )
                    {
                        // To Fix BUG-19869
                        // Mantissa의 fraction에 해당하는 모든 영역이 0인지 검사해야 한다.

                        sHasFraction = ACP_FALSE;

                        for ( i = exponent; i < (acp_sint16_t)sizeof(t1->mantissa); i++ )
                        {
                            if ( t1->mantissa[i] > 0 )
                            {
                                sHasFraction = ACP_TRUE;
                                break;
                            }
                            else
                            {
                                // Nothing To Do
                            }
                        }
                
                        if( sHasFraction == ACP_TRUE )
                        {
                            carry = 1;
                            for( mantissaIndex = exponent - 1;
                                 mantissaIndex >= 0;
                                 mantissaIndex-- )
                            {
                                t1->mantissa[mantissaIndex] += carry;
                                carry = 0;
                                if( t1->mantissa[mantissaIndex] >= 100 )
                                {
                                    t1->mantissa[mantissaIndex] -= 100;
                                    carry = 1;
                                }
                            }
                            if( carry )
                            {
                                for( mantissaIndex = exponent;
                                     mantissaIndex > 0;
                                     mantissaIndex-- )
                                {
                                    t1->mantissa[mantissaIndex]
                                        = t1->mantissa[mantissaIndex-1];
                                }
                                t1->mantissa[0] = 1;
                                exponent++;
                            }
                        }
                        else
                        {
                            // No Fraction
                        }
                
                        for( mantissaIndex = exponent;
                             mantissaIndex < (acp_sint16_t)sizeof(t1->mantissa);
                             mantissaIndex++ )
                        {
                            t1->mantissa[mantissaIndex] = 0;
                        }
                        ACI_TEST_RAISE( exponent > 63, ERR_OVERFLOW );
                    }
                }
                MTA_TNUMERIC_SET_EXPVAL(t1,exponent+64);
            }else{
                if( exponent <= 0 ){
                    *t1 = mtaTNumericZERO;
                }else{
                    for( mantissaIndex = exponent; mantissaIndex < (acp_sint16_t)sizeof(t1->mantissa); mantissaIndex++ ){
                        t1->mantissa[mantissaIndex] = 0;
                    }
                }
            }
        }
    }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_OVERFLOW );
    aciSetErrorCode( mtERR_ABORT_OVERFLOW );
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

acp_sint32_t mtaFloor( mtaTNumericType* t1, const mtaTNumericType* t2 )
{
    acp_sint16_t exponent;
    acp_sint16_t mantissaIndex;
    acp_uint16_t carry;
    
    if( MTA_TNUMERIC_IS_NULL( t2 ) ){
        *t1 = mtaTNumericNULL;
    }else{
        *t1 = *t2;
        if( !MTA_TNUMERIC_IS_ZERO( t2 ) ){
            exponent = ( ((acp_sint16_t)(MTA_TNUMERIC_EXP(t1))) - 64 );
            if( MTA_TNUMERIC_IS_POSITIVE(t1) ){
                if( exponent <= 0 ){
                    *t1 = mtaTNumericZERO;
                }else{
                    for( mantissaIndex = exponent; mantissaIndex < (acp_sint16_t)sizeof(t1->mantissa); mantissaIndex++ ){
                        t1->mantissa[mantissaIndex] = 0;
                    }
                }
            }else{
                if( exponent < 0 ){
                    *t1 = mtaTNumericZERO;
                    MTA_TNUMERIC_SET_NEGATIVE(t1);
                    exponent = 1;
                    t1->mantissa[0] = 1;
                }else{
                    if( exponent < (acp_sint16_t)sizeof(t1->mantissa) ){
                        if( t1->mantissa[exponent] > 0 ){
                            carry = 1;
                            for( mantissaIndex = exponent - 1; mantissaIndex >= 0; mantissaIndex-- ){
                                t1->mantissa[mantissaIndex] += carry;
                                carry = 0;
                                if( t1->mantissa[mantissaIndex] >= 100 ){
                                    t1->mantissa[mantissaIndex] -= 100;
                                    carry = 1;
                                }
                            }
                            if( carry ){
                                for( mantissaIndex = exponent; mantissaIndex > 0; mantissaIndex-- ){
                                    t1->mantissa[mantissaIndex] = t1->mantissa[mantissaIndex-1];
                                }
                                t1->mantissa[0] = 1;
                                exponent++;
                            }
                        }
                        for( mantissaIndex = exponent; mantissaIndex < (acp_sint16_t)sizeof(t1->mantissa); mantissaIndex++ ){
                            t1->mantissa[mantissaIndex] = 0;
                        }
                        ACI_TEST_RAISE( exponent > 63, ERR_OVERFLOW );
                    }
                }
                MTA_TNUMERIC_SET_EXPVAL(t1,exponent+64);
            }
        }
    }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_OVERFLOW );
    aciSetErrorCode( mtERR_ABORT_OVERFLOW );
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

acp_sint32_t mtaRound( mtaTNumericType* t1, const mtaTNumericType* t2, const mtaTNumericType* t3 )
{
    acp_sint16_t exponent;
    acp_sint16_t mantissaIndex;
    acp_sint64_t round;
    acp_uint16_t carry;

    if( MTA_TNUMERIC_IS_NULL( t2 ) ){
        *t1 = mtaTNumericNULL;
    }else{
        if( mtaSint64( &round, t3 ) != ACI_SUCCESS ){
            ACI_TEST_RAISE( aciGetErrorCode() != mtERR_ABORT_NULL_VALUE, ERR_THROUGH );
            *t1 = mtaTNumericNULL;
        }else{
            round = -round;
            *t1 = *t2;
            if( !MTA_TNUMERIC_IS_ZERO( t2 ) ){
                exponent = ( ((acp_sint16_t)(MTA_TNUMERIC_EXP(t1))) - 64 );
                if( exponent*2 <= round ){
                    if( exponent*2 == round && t1->mantissa[0] >= 50 ){
                        acpMemSet( t1->mantissa, 0, sizeof(t1->mantissa) );
                        t1->mantissa[0] = 1;
                        exponent++;
                        ACI_TEST_RAISE( exponent > 63, ERR_OVERFLOW );
                        MTA_TNUMERIC_SET_EXPVAL(t1,exponent-64);
                    }else{
                        *t1 = mtaTNumericZERO;
                    }
                }else{
#if defined(HP_HPUX) || defined(IA64_HP_HPUX)		   
                    switch( (acp_sint32_t)(round%2) ){ // BUGBUG : temporary fix for HP
#else
                    switch( round % 2 ){
#endif		       
                        case 1:
                            if( exponent-round/2-1 < (acp_sint16_t)sizeof(t1->mantissa) ){
                                if( t1->mantissa[exponent-round/2-1]%10 >= 5 ){
                                    t1->mantissa[exponent-round/2-1] -= t1->mantissa[exponent-round/2-1]%10;
                                    t1->mantissa[exponent-round/2-1] += 10;
                                }else{
                                    t1->mantissa[exponent-round/2-1] -= t1->mantissa[exponent-round/2-1]%10;
                                }
                                for( mantissaIndex = exponent-round/2; mantissaIndex < (acp_sint16_t)sizeof(t1->mantissa); mantissaIndex++ ){
                                    t1->mantissa[mantissaIndex] = 0;
                                }
                            }
                            break;
                        case -1:
                            if( exponent-round/2 < (acp_sint16_t)sizeof(t1->mantissa) ){
                                if( t1->mantissa[exponent-round/2]%10 >= 5 ){
                                    t1->mantissa[exponent-round/2] -= t1->mantissa[exponent-round/2]%10;
                                    t1->mantissa[exponent-round/2] += 10;
                                }else{
                                    t1->mantissa[exponent-round/2] -= t1->mantissa[exponent-round/2]%10;
                                }
                                for( mantissaIndex = exponent-round/2+1; mantissaIndex < (acp_sint16_t)sizeof(t1->mantissa); mantissaIndex++ ){
                                    t1->mantissa[mantissaIndex] = 0;
                                }
                            }
                            break;
                        default:
                            if( exponent-round/2 < (acp_sint16_t)sizeof(t1->mantissa) ){
                                if( t1->mantissa[exponent-round/2] >= 50 ){
                                    t1->mantissa[exponent-round/2-1]++;
                                }
                                for( mantissaIndex = exponent-round/2; mantissaIndex < (acp_sint16_t)sizeof(t1->mantissa); mantissaIndex++ ){
                                    t1->mantissa[mantissaIndex] = 0;
                                }
                            }
                    }
                    carry = 0;
                    for( mantissaIndex = sizeof(t1->mantissa)-1; mantissaIndex >= 0; mantissaIndex-- ){
                        t1->mantissa[mantissaIndex] += carry;
                        carry = 0;
                        if( t1->mantissa[mantissaIndex] >= 100 ){
                            t1->mantissa[mantissaIndex] -= 100;
                            carry = 1;
                        }
                    }
                    if( carry ){
                        for( mantissaIndex = sizeof(t1->mantissa)-1; mantissaIndex > 0; mantissaIndex-- ){
                            t1->mantissa[mantissaIndex] = t1->mantissa[mantissaIndex-1];
                        }
                        t1->mantissa[0] = 1;
                        exponent++;
                        ACI_TEST_RAISE( exponent > 63, ERR_OVERFLOW );
                        MTA_TNUMERIC_SET_EXPVAL(t1,exponent-64);
                    }
                    if( t1->mantissa[0] == 0 ){
                        *t1 = mtaTNumericZERO;
                    }
                }
            }
        }
    }
        
    ACI_CLEAR();
    return ACI_SUCCESS;
        
    ACI_EXCEPTION( ERR_OVERFLOW );
    aciSetErrorCode( mtERR_ABORT_OVERFLOW );
        
    ACI_EXCEPTION( ERR_THROUGH );

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_sint32_t mtaTrunc( mtaTNumericType* t1, const mtaTNumericType* t2, const mtaTNumericType* t3 )
{
    acp_sint16_t exponent;
    acp_sint16_t mantissaIndex;
    acp_sint64_t trunc;
    
    if( MTA_TNUMERIC_IS_NULL( t2 ) ){
        *t1 = mtaTNumericNULL;
    }else{
        if( mtaSint64( &trunc, t3 ) != ACI_SUCCESS ){
            ACI_TEST_RAISE( aciGetErrorCode() != mtERR_ABORT_NULL_VALUE, ERR_THROUGH );
            *t1 = mtaTNumericNULL;
        }else{
            trunc = -trunc;
            *t1 = *t2;
            if( !MTA_TNUMERIC_IS_ZERO( t1 ) ){
                exponent = ( ((acp_sint16_t)(MTA_TNUMERIC_EXP(t1))) - 64 );
                if( exponent*2 <= trunc ){
                    *t1 = mtaTNumericZERO;
                }else{
#if defined(HP_HPUX) || defined(IA64_HP_HPUX)		   
                    switch( (acp_sint32_t)(trunc%2) ){ // BUGBUG : temporary fix for HP
#else
                    switch( trunc%2 ){
#endif		       
                        case 1:
                            if( exponent-trunc/2-1 < (acp_sint16_t)sizeof(t1->mantissa) ){
                                t1->mantissa[exponent-trunc/2-1] -= t1->mantissa[exponent-trunc/2-1]%10;
                                for( mantissaIndex = exponent-trunc/2; mantissaIndex < (acp_sint16_t)sizeof(t1->mantissa); mantissaIndex++ ){
                                    t1->mantissa[mantissaIndex] = 0;
                                }
                            }
                            break;
                        case -1:
                            if( exponent-trunc/2 < (acp_sint16_t)sizeof(t1->mantissa) ){
                                t1->mantissa[exponent-trunc/2] -= t1->mantissa[exponent-trunc/2]%10;
                                for( mantissaIndex = exponent-trunc/2+1; mantissaIndex < (acp_sint16_t)sizeof(t1->mantissa); mantissaIndex++ ){
                                    t1->mantissa[mantissaIndex] = 0;
                                }
                            }
                            break;
                        default:
                            for( mantissaIndex = exponent-trunc/2; mantissaIndex < (acp_sint16_t)sizeof(t1->mantissa); mantissaIndex++ ){
                                t1->mantissa[mantissaIndex] = 0;
                            }
                    }
                    if( t1->mantissa[0] == 0 ){
                        *t1 = mtaTNumericZERO;
                    }
                }
            }
        }
    }
    
    ACI_CLEAR();
    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_THROUGH );

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

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
 * Revision 1.4  2003/12/10 04:10:11  jdlee
 * remove unused codes
 *
 * Revision 1.3  2003/04/10 07:53:04  assam
 * change tab to space
 *
 * Revision 1.2  2003/01/15 11:10:15  mylee
 * fix 3771
 *
 * Revision 1.1  2002/12/17 05:43:23  jdlee
 * create
 *
 * Revision 1.2  2002/11/19 05:21:36  khshim
 * Alpha linux sqrt fix
 *
 * Revision 1.1.1.1  2002/10/18 13:55:36  jdlee
 * create
 *
 * Revision 1.20  2001/08/01 12:18:25  jdlee
 * fix PR-1343
 *
 * Revision 1.19  2001/07/25 03:03:27  jdlee
 * fix HP porting of finite() function
 *
 * Revision 1.18  2001/07/20 09:21:20  jdlee
 * fix
 *
 * Revision 1.17  2001/07/19 09:44:21  jdlee
 * fix PR-1284, implement ln,exp,sqrt,log,power,stddev,variance functions
 *
 * Revision 1.16  2001/07/07 06:24:39  jdlee
 * fix PR-1222
 *
 * Revision 1.15  2001/06/21 03:15:19  jdlee
 * fix PR-1217
 *
 * Revision 1.14  2001/03/27 12:29:21  jdlee
 * remove dummy ACI_CLEAR
 *
 * Revision 1.13  2000/12/22 03:47:50  jdlee
 * fix
 *
 * Revision 1.12  2000/10/31 13:10:27  assam
 * *** empty log message ***
 *
 * Revision 1.11  2000/10/12 03:19:49  sjkim
 * func call fix
 *
 * Revision 1.10  2000/10/11 10:04:03  sjkim
 * trace info added
 *
 * Revision 1.9  2000/09/30 06:57:29  sjkim
 * error code fix
 *
 * Revision 1.8  2000/07/13 08:25:26  sjkim
 * error code fatal->abort
 *
 * Revision 1.7  2000/06/30 04:52:58  assam
 * *** empty log message ***
 *
 * Revision 1.6  2000/06/27 11:17:42  assam
 * *** empty log message ***
 *
 * Revision 1.5  2000/06/16 02:50:56  jdlee
 * fix naming
 *
 * Revision 1.4  2000/06/15 03:57:30  assam
 * ideErrorMgr changed.
 *
 * Revision 1.3  2000/06/14 03:46:43  assam
 * *** empty log message ***
 *
 * Revision 1.2  2000/06/07 13:19:23  jdlee
 * fix
 *
 * Revision 1.1.1.1  2000/06/07 11:44:06  jdlee
 * init
 *
 * Revision 1.3  2000/06/05 07:45:05  assam
 * *** empty log message ***
 *
 * Revision 1.2  2000/06/02 04:09:56  assam
 * *** empty log message ***
 *
 * Revision 1.1  2000/05/31 11:14:32  jdlee
 * change naming to id
 *
 * Revision 1.1  2000/05/31 07:33:57  
 * id 라이브러리 생성
 *
 * Revision 1.9  2000/05/22 10:12:26  sjkim
 * 에러 처리 루틴 수정 : ACTION 추가
 *
 * Revision 1.8  2000/04/20 07:25:43  assam
 * idaDate added.
 *
 * Revision 1.7  2000/04/19 11:48:26  assam
 * idaFloat,idaDouble,idaMod is fixed
 *
 * Revision 1.6  2000/04/19 06:40:23  assam
 * mod added.
 *
 * Revision 1.5  2000/03/22 08:46:10  assam
 * *** empty log message ***
 *
 * Revision 1.4  2000/02/25 03:42:10  assam
 * *** empty log message ***
 *
 * Revision 1.3  2000/02/24 01:10:38  assam
 * *** empty log message ***
 *
 * Revision 1.2  2000/02/21 12:14:14  assam
 * *** empty log message ***
 *
 **********************************************************************/

#include <mtca.h>
#include <mtce.h>

acp_sint32_t mtaAdd( mtaTNumericType* t1, const mtaTNumericType* t2, const mtaTNumericType* t3 )
{
    acp_sint16_t exponent1;
    acp_sint16_t exponent2;
    acp_sint16_t mantissaIndex;
    acp_uint8_t  mantissa1[sizeof(t1->mantissa)+1];
    acp_uint8_t  mantissa2[sizeof(t1->mantissa)+1];
    acp_uint16_t carry;
    acp_uint8_t* s1;
    acp_uint8_t* s2;
    acp_uint8_t* d1;
    acp_uint8_t* d2;
    acp_uint8_t* e1;
    acp_uint8_t* e2;
    
    if( MTA_TNUMERIC_IS_NULL( t2 ) || MTA_TNUMERIC_IS_NULL( t3 ) ){
        *t1 = mtaTNumericNULL;
    } else if( MTA_TNUMERIC_IS_ZERO( t2 ) ){
        *t1 = *t3;
    }else if( MTA_TNUMERIC_IS_ZERO( t3 ) ){
        *t1 = *t2;
    }else{
        if( MTA_TNUMERIC_EQUAL_SIGN(t2,t3) ){
            exponent1 = ( ((acp_sint16_t)(MTA_TNUMERIC_EXP(t2))) - 64 );
            exponent2 = ( ((acp_sint16_t)(MTA_TNUMERIC_EXP(t3))) - 64 );
            s1 = (acp_uint8_t*)(t2->mantissa);
            s2 = (acp_uint8_t*)(t3->mantissa);
            d1 = mantissa1;
            d2 = mantissa2;
            e1 = s1 + sizeof(t2->mantissa);
            e2 = s2 + sizeof(t3->mantissa);
            for( ; s1 < e1; s1++, d1++ ) *d1 = *s1;
            for( ; s2 < e2; s2++, d2++ ) *d2 = *s2;
            *d1 = 0;
            *d2 = 0;
            if( exponent1 < exponent2 ){
                s1 = mantissa1 + sizeof(mantissa1) - 1 - ( exponent2 - exponent1 );
                d1 = mantissa1 + sizeof(mantissa1) - 1;
                e1 = mantissa1 + exponent2 - exponent1;
                for( ; d1 >= e1; s1--, d1-- )  *d1 = *s1;
                for( ; d1 >= mantissa1; d1-- ) *d1 = 0;
                exponent1 = exponent2;
            }
            if( exponent1 > exponent2 ){
                s2 = mantissa2 + sizeof(mantissa2) - 1 - ( exponent1 - exponent2 );
                d2 = mantissa2 + sizeof(mantissa2) - 1;
                e2 = mantissa2 + exponent1 - exponent2;
                for( ; d2 >= e2; s2--, d2-- )  *d2 = *s2;
                for( ; d2 >= mantissa2; d2-- ) *d2 = 0;
                exponent2 = exponent1;
            }
            
            carry = 0;
            
            s1 = mantissa2 + sizeof(mantissa2) - 1;
            d1 = mantissa1 + sizeof(mantissa1) - 1;
            e1 = mantissa1;
            for( ; d1 >= e1; d1--, s1-- ){
                *d1 += *s1 + carry;
                if( *d1 >= 100 ){
                    *d1 -= 100;
                    carry = 1;
                }else{
                    carry = 0;
                }
            }
            if( carry ){
                for( mantissaIndex = sizeof(mantissa1)-1; mantissaIndex > 0; mantissaIndex-- ){
                    mantissa1[mantissaIndex] = mantissa1[mantissaIndex-1];
                }
                mantissa1[0] = 1;
                exponent1++;
            }
            if( mantissa1[sizeof(mantissa1)-1] >= 50){
                carry = 1;
                for( mantissaIndex = sizeof(mantissa1)-2; mantissaIndex >= 0 && carry; mantissaIndex-- ){
                    mantissa1[mantissaIndex] += carry;
                    carry = 0;
                    if( mantissa1[mantissaIndex] >= 100 ){
                        mantissa1[mantissaIndex] -= 100;
                        carry = 1;
                    }
                }
                if( carry ){
                    for( mantissaIndex = sizeof(mantissa1)-1; mantissaIndex > 0; mantissaIndex-- ){
                        mantissa1[mantissaIndex] = mantissa1[mantissaIndex-1];
                    }
                    mantissa1[0] = 1;
                    exponent1++;
                }
            }
            ACI_TEST_RAISE( exponent1 > 63, ERR_OVERFLOW );
            MTA_TNUMERIC_SET_SEBYTEVAL(t1,t2,exponent1+64);
            acpMemCpy( t1->mantissa, mantissa1, sizeof(t1->mantissa) );
        }else{
            mtaTNumericType t4;
            t4 = *t3;
            if( MTA_TNUMERIC_IS_POSITIVE(&t4) ){
                MTA_TNUMERIC_SET_NEGATIVE(&t4);
            }else{
                MTA_TNUMERIC_SET_POSITIVE(&t4);
            }
            ACI_TEST_RAISE( mtaSubtract( t1, t2, &t4 ) != ACI_SUCCESS, ERR_THROUGH );
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_OVERFLOW );
    aciSetErrorCode( mtERR_ABORT_OVERFLOW );

    ACI_EXCEPTION( ERR_THROUGH );

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_sint32_t mtaSubtract( mtaTNumericType* t1, const mtaTNumericType* t2, const mtaTNumericType* t3 )
{
    acp_sint32_t   compare;
    acp_sint16_t exponent1;
    acp_sint16_t exponent2;
    acp_sint16_t mantissaIndex1;
    acp_sint16_t mantissaIndex2;
    acp_uint8_t  mantissa1[sizeof(t1->mantissa)+2];
    acp_uint8_t  mantissa2[sizeof(t1->mantissa)+2];
    acp_uint16_t borrow;
    acp_uint16_t carry;

    if( MTA_TNUMERIC_IS_NULL( t2 ) || MTA_TNUMERIC_IS_NULL( t3 ) ){
        *t1 = mtaTNumericNULL;
    }else if( MTA_TNUMERIC_IS_ZERO( t2 ) && MTA_TNUMERIC_IS_ZERO( t3 ) ){
        *t1 = mtaTNumericZERO;
    }else if( MTA_TNUMERIC_IS_ZERO( t2 ) ){
        *t1 = *t3;
        if( MTA_TNUMERIC_IS_POSITIVE(t1) ){
            MTA_TNUMERIC_SET_NEGATIVE(t1);
        }else{
            MTA_TNUMERIC_SET_POSITIVE(t1);
        }
    }else if( MTA_TNUMERIC_IS_ZERO( t3 ) ){
        *t1 = *t2;
    }else{
        if( MTA_TNUMERIC_EQUAL_SIGN(t2,t3) ){
            exponent1 = ( ((acp_sint16_t)(MTA_TNUMERIC_EXP(t2))) - 64 );
            exponent2 = ( ((acp_sint16_t)(MTA_TNUMERIC_EXP(t3))) - 64 );
            for( mantissaIndex1 = 0; mantissaIndex1 < (acp_sint16_t)sizeof(t2->mantissa); mantissaIndex1++ ){
                mantissa1[mantissaIndex1] = t2->mantissa[mantissaIndex1];
                mantissa2[mantissaIndex1] = t3->mantissa[mantissaIndex1];
            }
            for( ; mantissaIndex1 < (acp_sint16_t)sizeof(mantissa1); mantissaIndex1++ ){
                mantissa1[mantissaIndex1] = 0;
                mantissa2[mantissaIndex1] = 0;
            }
            if( exponent1 < exponent2 ){
                for( mantissaIndex1 = sizeof(mantissa1)-1; mantissaIndex1 >= ( exponent2 - exponent1 ); mantissaIndex1-- ){
                    mantissa1[mantissaIndex1] = mantissa1[mantissaIndex1-(exponent2-exponent1)];
                }
                for( ; mantissaIndex1 >= 0; mantissaIndex1-- ){
                    mantissa1[mantissaIndex1] = 0;
                }
                exponent1 = exponent2;
            }
            if( exponent1 > exponent2 ){
                for( mantissaIndex1 = sizeof(mantissa2)-1; mantissaIndex1 >= ( exponent1 - exponent2 ); mantissaIndex1-- ){
                    mantissa2[mantissaIndex1] = mantissa2[mantissaIndex1-(exponent1-exponent2)];
                }
                for( ; mantissaIndex1 >= 0; mantissaIndex1-- ){
                    mantissa2[mantissaIndex1] = 0;
                }
                exponent2 = exponent1;
            }
            compare = acpMemCmp( mantissa1, mantissa2, sizeof(mantissa1) );
            if( compare != 0 ){
                borrow = 0;
                if( compare > 0 ){
                    for( mantissaIndex1 = sizeof(mantissa2)-1; mantissaIndex1 >= 0; mantissaIndex1-- ){
                        mantissa2[mantissaIndex1] += borrow;
                        borrow = 0;
                        if( mantissa1[mantissaIndex1] < mantissa2[mantissaIndex1] ){
                            mantissa1[mantissaIndex1] += 100;
                            borrow = 1;
                        }
                        mantissa1[mantissaIndex1] = mantissa1[mantissaIndex1] - mantissa2[mantissaIndex1];
                    }
                    MTA_TNUMERIC_SET_ABS_SIGN(t1,t2);
                }else{
                    for( mantissaIndex1 = sizeof(mantissa2)-1; mantissaIndex1 >= 0; mantissaIndex1-- ){
                        mantissa1[mantissaIndex1] += borrow;
                        borrow = 0;
                        if( mantissa2[mantissaIndex1] < mantissa1[mantissaIndex1] ){
                            mantissa2[mantissaIndex1] += 100;
                            borrow = 1;
                        }
                        mantissa1[mantissaIndex1] = mantissa2[mantissaIndex1] - mantissa1[mantissaIndex1];
                    }
                    if( MTA_TNUMERIC_IS_POSITIVE(t2) ){
                        MTA_TNUMERIC_SET_ABS_NEGATIVE(t1);
                    }else{
                        MTA_TNUMERIC_SET_ABS_POSITIVE(t1);
                    }
                }
                for( mantissaIndex1 = 0; mantissaIndex1 < (acp_sint16_t)sizeof(t1->mantissa); mantissaIndex1++ ){
                    if( mantissa1[mantissaIndex1] != 0 ){
                        break;
                    } 
                }
                exponent1 -= mantissaIndex1;
                for( mantissaIndex2 = 0; mantissaIndex2 < (acp_sint16_t)sizeof(t1->mantissa) - mantissaIndex1; mantissaIndex2++ ){
                    mantissa1[mantissaIndex2] = mantissa1[mantissaIndex1+mantissaIndex2];
                }
                for( ; mantissaIndex2 < (acp_sint16_t)sizeof(t1->mantissa); mantissaIndex2++ )
                {
                    mantissa1[mantissaIndex2] = 0;
                }
                
                if( mantissa1[sizeof(t1->mantissa)] >= 50){
                    carry = 1;
                    for( mantissaIndex1 = sizeof(t1->mantissa)-1; mantissaIndex1 >= 0 && carry; mantissaIndex1-- ){
                        mantissa1[mantissaIndex1] += carry;
                        carry = 0;
                        if( mantissa1[mantissaIndex1] >= 100 ){
                            mantissa1[mantissaIndex1] -= 100;
                            carry = 1;
                        }
                    }
                    if( carry ){
                        for( mantissaIndex1 = sizeof(mantissa1)-1; mantissaIndex1 > 0; mantissaIndex1-- ){
                            mantissa1[mantissaIndex1] = mantissa1[mantissaIndex1-1];
                        }
                        mantissa1[0] = 1;
                        exponent1++;
                    }
                }
                ACI_TEST_RAISE( exponent1 > 63, ERR_OVERFLOW );
                MTA_TNUMERIC_SET_EXPVAL(t1,exponent1+64);
                acpMemCpy( t1->mantissa, mantissa1, sizeof(t1->mantissa) );
            }else{
                *t1 = mtaTNumericZERO;
            }
        }else{
            mtaTNumericType t4;
            t4 = *t3;
            if( MTA_TNUMERIC_IS_POSITIVE(&t4) )
            {
                MTA_TNUMERIC_SET_NEGATIVE(&t4);
            }
            else
            {
                MTA_TNUMERIC_SET_POSITIVE(&t4);
            }
            ACI_TEST_RAISE( mtaAdd( t1, t2, &t4 ) != ACI_SUCCESS, ERR_THROUGH );
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_OVERFLOW );
    aciSetErrorCode( mtERR_ABORT_OVERFLOW );

    ACI_EXCEPTION( ERR_THROUGH );

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_sint32_t mtaMultiply( mtaTNumericType* t1, const mtaTNumericType* t2, const mtaTNumericType* t3 )
{
    acp_sint16_t exponent;
    acp_sint16_t mantissaIndex1;
    acp_sint16_t mantissaIndex2;
    acp_sint16_t mantissaIndex3;
    acp_uint8_t  mantissa[sizeof(t1->mantissa)*2];
    acp_uint16_t carry;

    if( MTA_TNUMERIC_IS_NULL( t2 ) || MTA_TNUMERIC_IS_NULL( t3 ) ){
        *t1 = mtaTNumericNULL;
    }else if( MTA_TNUMERIC_IS_ZERO( t2 ) || MTA_TNUMERIC_IS_ZERO( t3 ) ){
        *t1 = mtaTNumericZERO;
    }else{
        exponent = ( ((acp_sint16_t)(MTA_TNUMERIC_EXP(t2))) - 64 ) + ( ((acp_sint16_t)(MTA_TNUMERIC_EXP(t3))) - 64 );
        acpMemSet( mantissa, 0, sizeof(mantissa) );
        for( mantissaIndex2 = 0; mantissaIndex2 < (acp_sint16_t)sizeof(t3->mantissa); mantissaIndex2++ ){
            for( mantissaIndex1 = 0; mantissaIndex1 < (acp_sint16_t)sizeof(t2->mantissa); mantissaIndex1++ ){
                mantissa[mantissaIndex1+mantissaIndex2]   += ((acp_uint16_t)t2->mantissa[mantissaIndex1])*((acp_uint16_t)t3->mantissa[mantissaIndex2])/100;
                mantissa[mantissaIndex1+mantissaIndex2+1] += ((acp_uint16_t)t2->mantissa[mantissaIndex1])*((acp_uint16_t)t3->mantissa[mantissaIndex2])%100;
                if( mantissa[mantissaIndex1+mantissaIndex2+1] >= 100 ){
                    mantissa[mantissaIndex1+mantissaIndex2+1] -= 100;
                    mantissa[mantissaIndex1+mantissaIndex2]   += 1;
                }
                for( mantissaIndex3 = mantissaIndex1+mantissaIndex2; mantissaIndex3 > 0 && mantissa[mantissaIndex3] >= 100; mantissaIndex3-- ){
                    mantissa[mantissaIndex3]   -= 100;
                    mantissa[mantissaIndex3-1] += 1;
                }
            }
        }
        if( mantissa[0] == 0 ){
            for( mantissaIndex1 = 0; mantissaIndex1 < (acp_sint16_t)(sizeof(mantissa)-1); mantissaIndex1++ ){
                mantissa[mantissaIndex1] = mantissa[mantissaIndex1+1];
            }
            exponent--;
        }
        acpMemCpy( t1->mantissa, mantissa, sizeof(t1->mantissa) );
        if( mantissa[sizeof(t1->mantissa)] >= 50){
            carry = 1;
            for( mantissaIndex1 = sizeof(t1->mantissa)-1; mantissaIndex1 >= 0 && carry; mantissaIndex1-- ){
                t1->mantissa[mantissaIndex1] += carry;
                carry = 0;
                if( t1->mantissa[mantissaIndex1] >= 100 ){
                    t1->mantissa[mantissaIndex1] -= 100;
                    carry = 1;
                }
            }
            if( carry ){
                for( mantissaIndex1 = sizeof(t1->mantissa)-1; mantissaIndex1 > 0; mantissaIndex1-- ){
                    t1->mantissa[mantissaIndex1] = t1->mantissa[mantissaIndex1-1];
                }
                t1->mantissa[0] = 1;
                exponent++;
            }
        }
        ACI_TEST_RAISE( exponent > 63, ERR_OVERFLOW );
        if( exponent < -63 ){
            *t1 = mtaTNumericZERO;
        }else{
            if( MTA_TNUMERIC_EQUAL_SIGN(t2,t3) ){
                MTA_TNUMERIC_SET_ABS_POSITIVE(t1);
            }else{
                MTA_TNUMERIC_SET_ABS_NEGATIVE(t1);
            }
            MTA_TNUMERIC_SET_EXPVAL(t1,exponent+64);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_OVERFLOW );
    aciSetErrorCode( mtERR_ABORT_OVERFLOW );

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_sint32_t mtaDivide( mtaTNumericType* t1, const mtaTNumericType* t2, const mtaTNumericType* t3 )
{
    acp_sint16_t exponent;
    acp_sint16_t mantissaIndex1;
    acp_sint16_t mantissaIndex2;
    acp_uint8_t  mantissa1[sizeof(t1->mantissa)+2];
    acp_uint8_t  mantissa2[sizeof(t1->mantissa)+1];
    acp_uint8_t  mantissa3[sizeof(t1->mantissa)+1];
    acp_uint8_t  mantissa4[sizeof(t1->mantissa)+1];
    acp_uint16_t mininum;
    acp_uint16_t borrow;
    acp_uint16_t carry;

    if( MTA_TNUMERIC_IS_NULL( t2 ) || MTA_TNUMERIC_IS_NULL( t3 ) ){
        *t1 = mtaTNumericNULL;
    }else if( MTA_TNUMERIC_IS_ZERO( t3 ) ){
        ACI_RAISE( ERR_DIVACI_BY_ZERO );
    }else if( MTA_TNUMERIC_IS_ZERO( t2 ) ){
        *t1 = mtaTNumericZERO;
    }else{
        exponent = ( ((acp_sint16_t)(MTA_TNUMERIC_EXP(t2))) - 64 ) - ( ((acp_sint16_t)(MTA_TNUMERIC_EXP(t3))) - 64 ) + 1;
        acpMemSet( mantissa1, 0, sizeof(mantissa1) );
        mantissa2[0] = mantissa3[0] = 0;
        for( mantissaIndex1 = 1 ; mantissaIndex1 < (acp_sint16_t)sizeof(mantissa2); mantissaIndex1++ ){
            mantissa1[mantissaIndex1] = 0;
            mantissa2[mantissaIndex1] = t2->mantissa[mantissaIndex1-1];
            mantissa3[mantissaIndex1] = t3->mantissa[mantissaIndex1-1];
        }
        for( mantissaIndex1 = 0; mantissaIndex1 < (acp_sint16_t)sizeof(mantissa1); mantissaIndex1++ ){
            while( mantissa2[0] != 0 ||  mantissa2[1] >= mantissa3[1] ){
                mininum = (mantissa2[0]*100+mantissa2[1]) / (mantissa3[1]+1) ;
                if( mininum ){
                    mantissa4[0] = 0;
                    for( mantissaIndex2 = 1; mantissaIndex2 < (acp_sint16_t)sizeof(mantissa4); mantissaIndex2++ ){
                        mantissa4[mantissaIndex2-1] += ((acp_uint16_t)mantissa3[mantissaIndex2])*mininum/100;
                        mantissa4[mantissaIndex2]    = ((acp_uint16_t)mantissa3[mantissaIndex2])*mininum%100;
                    }
                    for( mantissaIndex2 = sizeof(mantissa4)-1; mantissaIndex2 > 0; mantissaIndex2-- ){
                        if( mantissa4[mantissaIndex2] >= 100 ){
                            mantissa4[mantissaIndex2]   -= 100;
                            mantissa4[mantissaIndex2-1] += 1;
                        }
                    }
                    borrow = 0;
                    for( mantissaIndex2 = sizeof(mantissa4)-1; mantissaIndex2 >= 0; mantissaIndex2-- ){
                        mantissa4[mantissaIndex2] += borrow;
                        borrow = 0;
                        if( mantissa2[mantissaIndex2] < mantissa4[mantissaIndex2] ){
                            mantissa2[mantissaIndex2] += 100;
                            borrow = 1;
                        }
                        mantissa2[mantissaIndex2] -= mantissa4[mantissaIndex2];
                    }
                    mantissa1[mantissaIndex1] += mininum;
                }else{
                    if( acpMemCmp( mantissa2, mantissa3, sizeof(mantissa3) ) >= 0 ){
                        borrow = 0;
                        for( mantissaIndex2 = sizeof(mantissa4)-1; mantissaIndex2 >= 0; mantissaIndex2-- ){
                            mantissa4[mantissaIndex2] = mantissa3[mantissaIndex2]+borrow;
                            borrow = 0;
                            if( mantissa2[mantissaIndex2] < mantissa4[mantissaIndex2] ){
                                mantissa2[mantissaIndex2] += 100;
                                borrow = 1;
                            }
                            mantissa2[mantissaIndex2] -= mantissa4[mantissaIndex2];
                        }
                        mantissa1[mantissaIndex1]++;
                    }
                    break;
                }
            }
            for( mantissaIndex2 = 0; mantissaIndex2 < (acp_sint16_t)sizeof(mantissa2)-1; mantissaIndex2++ ){
                mantissa2[mantissaIndex2] = mantissa2[mantissaIndex2+1];
            }
            mantissa2[mantissaIndex2] = 0;
        }
        if( mantissa1[0] == 0 )
        {
            for( mantissaIndex1 = 0; mantissaIndex1 < (acp_sint16_t)(sizeof(mantissa1) - 1); mantissaIndex1++ ){
                mantissa1[mantissaIndex1] = mantissa1[mantissaIndex1+1];
            }
            exponent--;
        }
        acpMemCpy( t1->mantissa, mantissa1, sizeof(t1->mantissa) );
        if( mantissa1[sizeof(t1->mantissa)] >= 50){
            carry = 1;
            for( mantissaIndex1 = sizeof(t1->mantissa)-1; mantissaIndex1 >= 0 && carry; mantissaIndex1-- ){
                t1->mantissa[mantissaIndex1] += carry;
                carry = 0;
                if( t1->mantissa[mantissaIndex1] >= 100 ){
                    t1->mantissa[mantissaIndex1] -= 100;
                    carry = 1;
                }
            }
            if( carry ){
                for( mantissaIndex1 = sizeof(t1->mantissa)-1; mantissaIndex1 > 0; mantissaIndex1-- ){
                    t1->mantissa[mantissaIndex1] = t1->mantissa[mantissaIndex1-1];
                }
                t1->mantissa[0] = 1;
                exponent++;
            }
        }
        ACI_TEST_RAISE( exponent > 63, ERR_OVERFLOW );
        if( exponent < -63 ){
            *t1 = mtaTNumericZERO;
        }else{
            if( MTA_TNUMERIC_EQUAL_SIGN(t2,t3) ){
                MTA_TNUMERIC_SET_ABS_POSITIVE(t1);
            }else{
                MTA_TNUMERIC_SET_ABS_NEGATIVE(t1);
            }
            MTA_TNUMERIC_SET_EXPVAL(t1,exponent+64);
        }        
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_DIVACI_BY_ZERO );
    aciSetErrorCode( mtERR_ABORT_DIVIDE_BY_ZERO );

    ACI_EXCEPTION( ERR_OVERFLOW );
    aciSetErrorCode( mtERR_ABORT_OVERFLOW );

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_sint32_t mtaMod( mtaTNumericType* t1, const mtaTNumericType* t2, const mtaTNumericType* t3 )
{
    acp_sint16_t exponent1;
    acp_sint16_t exponent2;
    acp_sint16_t mantissaIndex1;
    acp_sint16_t mantissaIndex2;
    acp_uint8_t  mantissa1[sizeof(t1->mantissa)+1];
    acp_uint8_t  mantissa2[sizeof(t1->mantissa)+1];
    acp_uint8_t  mantissa3[sizeof(t1->mantissa)+1];
    acp_uint16_t mininum;
    acp_uint16_t borrow;

    if( MTA_TNUMERIC_IS_NULL( t2 ) || MTA_TNUMERIC_IS_NULL( t3 ) ){
        *t1 = mtaTNumericNULL;
    }else{
        if( MTA_TNUMERIC_IS_ZERO( t2 ) ){
            *t1 = mtaTNumericZERO;
        }else if( MTA_TNUMERIC_IS_ZERO( t3 ) ){
            ACI_RAISE( ERR_DIVACI_BY_ZERO );
        }else{
            exponent1 = ((acp_sint16_t)(MTA_TNUMERIC_EXP(t2))) - 64;
            exponent2 = ((acp_sint16_t)(MTA_TNUMERIC_EXP(t3))) - 64;
            mantissa1[0] = mantissa2[0] = 0;
            for( mantissaIndex1 = 1 ; mantissaIndex1 < (acp_sint16_t)sizeof(mantissa1); mantissaIndex1++ ){
                mantissa1[mantissaIndex1] = t2->mantissa[mantissaIndex1-1];
                mantissa2[mantissaIndex1] = t3->mantissa[mantissaIndex1-1];
            }
            while( exponent1 >= exponent2 ){
                while( mantissa1[0] != 0 ||  mantissa1[1] >= mantissa2[1] ){
                    mininum = (mantissa1[0]*100+mantissa1[1]) / (mantissa2[1]+1) ;
                    if( mininum ){
                        mantissa3[0] = 0;
                        for( mantissaIndex2 = 1; mantissaIndex2 < (acp_sint16_t)sizeof(mantissa3); mantissaIndex2++ ){
                            mantissa3[mantissaIndex2-1] += ((acp_uint16_t)mantissa2[mantissaIndex2])*mininum/100;
                            mantissa3[mantissaIndex2]    = ((acp_uint16_t)mantissa2[mantissaIndex2])*mininum%100;
                        }
                        for( mantissaIndex2 = sizeof(mantissa3)-1; mantissaIndex2 > 0; mantissaIndex2-- ){
                            if( mantissa3[mantissaIndex2] >= 100 ){
                                mantissa3[mantissaIndex2]   -= 100;
                                mantissa3[mantissaIndex2-1] += 1;
                            }
                        }
                        borrow = 0;
                        for( mantissaIndex2 = sizeof(mantissa3)-1; mantissaIndex2 >= 0; mantissaIndex2-- ){
                            mantissa3[mantissaIndex2] += borrow;
                            borrow = 0;
                            if( mantissa1[mantissaIndex2] < mantissa3[mantissaIndex2] ){
                                mantissa1[mantissaIndex2] += 100;
                                borrow = 1;
                            }
                            mantissa1[mantissaIndex2] -= mantissa3[mantissaIndex2];
                        }
                    }else{
                        if( acpMemCmp( mantissa1, mantissa2, sizeof(mantissa2) ) >= 0 ){
                            borrow = 0;
                            for( mantissaIndex2 = sizeof(mantissa3)-1; mantissaIndex2 >= 0; mantissaIndex2-- ){
                                mantissa3[mantissaIndex2] = mantissa2[mantissaIndex2]+borrow;
                                borrow = 0;
                                if( mantissa1[mantissaIndex2] < mantissa3[mantissaIndex2] ){
                                    mantissa1[mantissaIndex2] += 100;
                                    borrow = 1;
                                }
                                mantissa1[mantissaIndex2] -= mantissa3[mantissaIndex2];
                            }
                        }
                        break;
                    }
                }
                if( exponent1 <= exponent2 ){
                    break;
                }
                for( mantissaIndex2 = 0; mantissaIndex2 < (acp_sint16_t)sizeof(mantissa1)-1; mantissaIndex2++ ){
                    mantissa1[mantissaIndex2] = mantissa1[mantissaIndex2+1];
                }
                mantissa1[mantissaIndex2] = 0;
                exponent1--;
            }

            mantissaIndex2 = sizeof(mantissa1);
            for( mantissaIndex1 = 1; mantissaIndex1 < (acp_sint16_t)sizeof(mantissa1); mantissaIndex1++ ){
                if( mantissa1[mantissaIndex1] != 0 ){
                    mantissaIndex2 = mantissaIndex1;
                    break;
                }
            }
            *t1 = mtaTNumericZERO;
            if( mantissaIndex2 < (acp_sint16_t)sizeof(mantissa1) ){
                exponent1 -= mantissaIndex2-1;
                ACI_TEST_RAISE( exponent1 > 63, ERR_OVERFLOW );
                if( exponent1 >= -63 ){
                    MTA_TNUMERIC_SET_SEBYTEVAL(t1,t2,exponent1+64);
                    for( mantissaIndex1 = 0; mantissaIndex2 < (acp_sint16_t)sizeof(mantissa1); mantissaIndex1++, mantissaIndex2++ ){
                        t1->mantissa[mantissaIndex1] = mantissa1[mantissaIndex2];
                    }
                }
            }
            
        }
    }
        
    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_DIVACI_BY_ZERO );
    aciSetErrorCode( mtERR_ABORT_DIVIDE_BY_ZERO );

    ACI_EXCEPTION( ERR_OVERFLOW );
    aciSetErrorCode( mtERR_ABORT_OVERFLOW );
    
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


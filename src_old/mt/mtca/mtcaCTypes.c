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

#include <mtca.h>
#include <mtce.h>

acp_sint32_t mtaSint64( acp_sint64_t* l, const mtaTNumericType* t )
{
    acp_uint64_t    ul;
    mtaTNumericType tt;
    
    ACI_TEST_RAISE( MTA_TNUMERIC_IS_NULL( t ), ERR_NULL_VALUE );
    tt = *t;
    if( MTA_TNUMERIC_IS_POSITIVE(&tt) ){
        ACI_TEST_RAISE( mtaUint64( &ul, &tt ) != ACI_SUCCESS, ERR_THROUGH );
        ACI_TEST_RAISE( ul > ACP_UINT64_LITERAL(9223372036854775807), ERR_OVERFLOW );	
        *l = (acp_sint64_t)ul;
    }else{
        MTA_TNUMERIC_SET_POSITIVE(&tt);
        ACI_TEST_RAISE( mtaUint64( &ul, &tt ) != ACI_SUCCESS, ERR_THROUGH );
        ACI_TEST_RAISE( ul > ACP_UINT64_LITERAL(9223372036854775808), ERR_OVERFLOW );
        if( ul == ACP_UINT64_LITERAL(9223372036854775808) ){
            *l = ACP_SINT64_LITERAL(-9223372036854775807) - ACP_SINT64_LITERAL(1);
        }else{
            *l = -(acp_sint64_t)ul;
        }
    }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_NULL_VALUE );
    aciSetErrorCode( mtERR_ABORT_NULL_VALUE );
    
    ACI_EXCEPTION( ERR_OVERFLOW );
    aciSetErrorCode( mtERR_ABORT_OVERFLOW );
    
    ACI_EXCEPTION( ERR_THROUGH );
    
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_sint32_t mtaUint64( acp_uint64_t* l, const mtaTNumericType* t )
{
    static const mtaTNumericType max = {0x80+10+64,{18,44,67,44,7,37,9,55,16,15,0,0,0,0,0,0,0,0,0,0}};
    static const acp_uint64_t power[] = { ACP_UINT64_LITERAL(1),
                                   ACP_UINT64_LITERAL(100),
                                   ACP_UINT64_LITERAL(10000),
                                   ACP_UINT64_LITERAL(1000000),
                                   ACP_UINT64_LITERAL(100000000),
                                   ACP_UINT64_LITERAL(10000000000),
                                   ACP_UINT64_LITERAL(1000000000000),
                                   ACP_UINT64_LITERAL(100000000000000),
                                   ACP_UINT64_LITERAL(10000000000000000),
                                   ACP_UINT64_LITERAL(1000000000000000000)};
    mtaBooleanType boolean;
    acp_sint16_t   exponent;
    acp_sint16_t   mantissaIndex;

    ACI_TEST_RAISE( MTA_TNUMERIC_IS_NULL( t ), ERR_NULL_VALUE );
    ACI_TEST_RAISE( mtaLessEqual( &boolean, t, &max ) != ACI_SUCCESS, ERR_THROUGH );
    ACI_TEST_RAISE( boolean.value == MTA_FALSE, ERR_OVERFLOW );
    ACI_TEST_RAISE( mtaGreaterEqual( &boolean, t, &mtaTNumericZERO ) != ACI_SUCCESS, ERR_THROUGH );
    ACI_TEST_RAISE( boolean.value == MTA_FALSE, ERR_OVERFLOW );
    *l = 0;
    if( !MTA_TNUMERIC_IS_ZERO( t ) ){
	exponent = MTA_TNUMERIC_EXP(t)-64;
        if (exponent > 0)
        {
            ACI_TEST_RAISE( (acp_uint32_t)exponent > (sizeof(power) / sizeof(power[0])), ERR_OVERFLOW );
            for( mantissaIndex = 0; mantissaIndex < exponent; mantissaIndex++ ){
	        *l += t->mantissa[mantissaIndex]*power[exponent-mantissaIndex-1];
            }
        }
    }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_NULL_VALUE );
    aciSetErrorCode( mtERR_ABORT_NULL_VALUE );
    
    ACI_EXCEPTION( ERR_OVERFLOW );
    aciSetErrorCode( mtERR_ABORT_OVERFLOW );
    
    ACI_EXCEPTION( ERR_THROUGH );
    
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


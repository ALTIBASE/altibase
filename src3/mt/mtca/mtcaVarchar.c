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
 * Revision 1.3  2003/12/10 04:01:32  jdlee
 * remove unused codes
 *
 * Revision 1.2  2003/11/17 07:31:14  jdlee
 * fix gcc warnings
 *
 * Revision 1.1  2002/12/17 05:43:23  jdlee
 * create
 *
 * Revision 1.2  2002/11/26 07:32:14  jhseong
 * fix IBM p690 problem
 *
 * Revision 1.1.1.1  2002/10/18 13:55:36  jdlee
 * create
 *
 * Revision 1.27  2002/05/15 08:27:52  jdlee
 * fix PR-2186
 *
 * Revision 1.26  2001/12/12 01:39:19  jdlee
 * fix
 *
 * Revision 1.25  2001/12/11 08:43:24  jdlee
 * fix
 *
 * Revision 1.24  2001/10/19 02:26:27  jdlee
 * fix PR-1574
 *
 * Revision 1.23  2001/10/18 05:50:41  jdlee
 * fix PR-1570,1575
 *
 * Revision 1.22  2001/06/21 03:15:19  jdlee
 * fix PR-1217
 *
 * Revision 1.21  2001/06/20 04:53:48  jdlee
 * fix
 *
 * Revision 1.20  2001/03/27 12:29:21  jdlee
 * remove dummy ACI_CLEAR
 *
 * Revision 1.19  2001/03/06 10:52:00  jdlee
 * porting to aix
 *
 * Revision 1.18  2000/12/22 03:47:50  jdlee
 * fix
 *
 * Revision 1.17  2000/10/31 13:10:27  assam
 * *** empty log message ***
 *
 * Revision 1.16  2000/10/17 03:47:12  jdlee
 * fix
 *
 * Revision 1.15  2000/10/16 02:47:38  jdlee
 * fix
 *
 * Revision 1.14  2000/10/13 10:45:13  assam
 * *** empty log message ***
 *
 * Revision 1.13  2000/10/13 10:30:48  assam
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
 * Revision 1.9  2000/09/28 10:42:51  assam
 * *** empty log message ***
 *
 * Revision 1.8  2000/09/27 11:38:28  assam
 * *** empty log message ***
 *
 * Revision 1.7  2000/09/27 06:20:08  assam
 * *** empty log message ***
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
 * Revision 1.11  2000/05/22 10:12:26  sjkim
 * 에러 처리 루틴 수정 : ACTION 추가
 *
 * Revision 1.10  2000/04/21 03:53:34  assam
 * *** empty log message ***
 *
 * Revision 1.9  2000/03/22 08:46:10  assam
 * *** empty log message ***
 *
 * Revision 1.8  2000/03/15 01:52:19  assam
 * *** empty log message ***
 *
 * Revision 1.7  2000/03/14 11:26:47  assam
 * *** empty log message ***
 *
 * Revision 1.6  2000/03/13 08:03:46  assam
 * *** empty log message ***
 *
 * Revision 1.5  2000/03/09 02:14:28  assam
 * *** empty log message ***
 *
 * Revision 1.4  2000/02/24 01:10:39  assam
 * *** empty log message ***
 *
 * Revision 1.3  2000/02/21 07:11:20  assam
 * *** empty log message ***
 *
 * Revision 1.2  2000/02/21 02:16:44  assam
 * *** empty log message ***
 *
 * Revision 1.1  2000/02/17 03:17:32  assam
 * Migration from QP.
 *
 **********************************************************************/

#include <mtca.h>
#include <mtce.h>
#include <mtcn.h>

#define MTA_VARCHAR_DISPLAY_ROUND( index )                                             \
        if( mantissaTemp[(index)] >= 5 ){                                              \
            carry = 1;                                                                 \
            for( mantissaIndex = (index)-1; mantissaIndex >= 0; mantissaIndex-- ){     \
                mantissaTemp[mantissaIndex] += carry;                                  \
                carry = 0;                                                             \
                if( mantissaTemp[mantissaIndex] >= 10 ){                               \
                    mantissaTemp[mantissaIndex] -= 10;                                 \
                    carry = 1;                                                         \
                }                                                                      \
            }                                                                          \
            if( carry ){                                                               \
                for( mantissaIndex = (index); mantissaIndex > 0; mantissaIndex-- ){    \
                    mantissaTemp[mantissaIndex] = mantissaTemp[mantissaIndex-1];       \
                }                                                                      \
                mantissaTemp[0] = 1;                                                   \
                exponent2++;                                                           \
            }                                                                          \
        }

acp_sint32_t mtaVarcharExact( mtaVarcharType* v1, acp_uint16_t v1Max, const mtaNumericType* n2, acp_sint32_t n2Length )
{
    mtaTNumericType t2;
    
    ACI_TEST_RAISE( mtaTNumeric3( &t2, n2, n2Length ) != ACI_SUCCESS, ERR_THROUGH );
    ACI_TEST_RAISE( mtaVarcharExact2( v1, v1Max, &t2 ) != ACI_SUCCESS, ERR_THROUGH );

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_THROUGH );

    ACI_EXCEPTION_END;

    return ACI_FAILURE;    
}

acp_sint32_t mtaVarcharExact2( mtaVarcharType* v1, acp_uint16_t v1Max, const mtaTNumericType* t2 )
{
    acp_sint16_t exponent;
    acp_uint16_t v1Length;
    acp_sint16_t mantissaIndex;
    acp_sint16_t mantissaEnd = 0;
    
    v1Length = 0;
    if( !MTA_TNUMERIC_IS_NULL( t2 ) ){
        if( MTA_TNUMERIC_IS_ZERO( t2 ) ){
            ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x30 ) != ACI_SUCCESS, ERR_THROUGH );
        }else{
            exponent = ( ((acp_sint16_t)(MTA_TNUMERIC_EXP(t2))) - 64 )*2;
            if( MTA_TNUMERIC_IS_NEGATIVE(t2) ){
                ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x2D ) != ACI_SUCCESS, ERR_THROUGH );            
            }
            for( mantissaIndex = 0; mantissaIndex < (acp_sint16_t)sizeof(t2->mantissa); mantissaIndex++ ){
                if( t2->mantissa[mantissaIndex] ){
                    mantissaEnd = mantissaIndex+1;
                }
            }
            if( t2->mantissa[0] >= 10 ){
                exponent--;
                ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x30+(t2->mantissa[0]/10) ) != ACI_SUCCESS, ERR_THROUGH );
                if( mantissaEnd > 1 || t2->mantissa[0]%10 ){
                    ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x2E ) != ACI_SUCCESS, ERR_THROUGH );
                    ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x30+(t2->mantissa[0]%10) ) != ACI_SUCCESS, ERR_THROUGH );
                }
            }else{
                exponent -= 2;
                ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x30+(t2->mantissa[0]) ) != ACI_SUCCESS, ERR_THROUGH );
                if( mantissaEnd > 1 ){
                    ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x2E ) != ACI_SUCCESS, ERR_THROUGH );
                }
            }
            for( mantissaIndex = 1; mantissaIndex < mantissaEnd; mantissaIndex++ ){
                ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x30+(t2->mantissa[mantissaIndex]/10) ) != ACI_SUCCESS, ERR_THROUGH );
                if( mantissaEnd > mantissaIndex+1 || t2->mantissa[mantissaIndex]%10 ){
                    ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x30+(t2->mantissa[mantissaIndex]%10) ) != ACI_SUCCESS, ERR_THROUGH );
                }            
            }
            if( exponent != 0 ){
                ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x45 ) != ACI_SUCCESS, ERR_THROUGH );
                if( exponent > 0 ){
                    ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x2B ) != ACI_SUCCESS, ERR_THROUGH );
                }else{
                    ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x2D ) != ACI_SUCCESS, ERR_THROUGH );
                    exponent *= -1;
                }
                if( exponent >= 100 ){
                    ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x30+(exponent/100) ) != ACI_SUCCESS, ERR_THROUGH );
                    ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x30+(exponent/10%10) ) != ACI_SUCCESS, ERR_THROUGH );
                    ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x30+(exponent%10) ) != ACI_SUCCESS, ERR_THROUGH );
                }else if( exponent >= 10 ){
                    ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x30+(exponent/10) ) != ACI_SUCCESS, ERR_THROUGH );
                    ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x30+(exponent%10) ) != ACI_SUCCESS, ERR_THROUGH );
                }else{
                    ACI_TEST_RAISE( mtnAddAscii( v1->value, v1Max, &v1Length, 0x30+(exponent%10) ) != ACI_SUCCESS, ERR_THROUGH );
                }
            }
        }
    }
    MTA_SET_LENGTH( v1->length, v1Length );
    
    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_THROUGH );
    MTA_SET_LENGTH( v1->length, v1Length );

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

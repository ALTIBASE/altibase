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

acp_sint32_t mtaGreaterEqual( mtaBooleanType* b1, const mtaTNumericType* t2, const mtaTNumericType* t3 )
{
    acp_sint32_t   compare;
    
    if( MTA_TNUMERIC_IS_NULL( t2 ) || MTA_TNUMERIC_IS_NULL( t3 ) ){
        b1->value = MTA_NULL;
    }else if( MTA_TNUMERIC_IS_ZERO( t2 ) && MTA_TNUMERIC_IS_ZERO( t3 ) ){
        b1->value = MTA_TRUE;
    }else if( MTA_TNUMERIC_IS_ZERO( t2 ) ){
        if( MTA_TNUMERIC_IS_NEGATIVE(t3) ){
            b1->value = MTA_TRUE;
        }else{
            b1->value = MTA_FALSE;
        }
    }else if( MTA_TNUMERIC_IS_ZERO( t3 ) ){
        if( MTA_TNUMERIC_IS_POSITIVE(t2) ){
            b1->value = MTA_TRUE;
        }else{
            b1->value = MTA_FALSE;
        }
    }else{
        if( MTA_TNUMERIC_IS_POSITIVE(t2) ){
            if( MTA_TNUMERIC_IS_NEGATIVE(t3) ){
                b1->value = MTA_TRUE;
            }else{
                if( MTA_TNUMERIC_EXP(t2) > MTA_TNUMERIC_EXP(t3) ){
                    b1->value = MTA_TRUE;
                }else if( MTA_TNUMERIC_EXP(t2) < MTA_TNUMERIC_EXP(t3) ){
                    b1->value = MTA_FALSE;
                }else{
                    compare = acpMemCmp( t2->mantissa, t3->mantissa, sizeof(t2->mantissa) );
                    if( compare >= 0 ){
                        b1->value = MTA_TRUE;
                    }else{
                        b1->value = MTA_FALSE;
                    }
                }
            }
        }else{
            if( MTA_TNUMERIC_IS_POSITIVE(t3) ){
                b1->value = MTA_FALSE;
            }else{
                if( MTA_TNUMERIC_EXP(t2) < MTA_TNUMERIC_EXP(t3) ){
                    b1->value = MTA_TRUE;
                }else if( MTA_TNUMERIC_EXP(t2) > MTA_TNUMERIC_EXP(t3) ){
                    b1->value = MTA_FALSE;
                }else{
                    compare = acpMemCmp( t2->mantissa, t3->mantissa, sizeof(t2->mantissa) );
                    if( compare <= 0 ){
                        b1->value = MTA_TRUE;
                    }else{
                        b1->value = MTA_FALSE;
                    }		    
                }
            }
        }
    }
    
    return ACI_SUCCESS;
}

acp_sint32_t mtaLessEqual( mtaBooleanType* b1, const mtaTNumericType* t2, const mtaTNumericType* t3 )
{
    acp_sint32_t   compare;
    
    if( MTA_TNUMERIC_IS_NULL( t2 ) || MTA_TNUMERIC_IS_NULL( t3 ) ){
        b1->value = MTA_NULL;
    }else if( MTA_TNUMERIC_IS_ZERO( t2 ) && MTA_TNUMERIC_IS_ZERO( t3 ) ){
        b1->value = MTA_TRUE;
    }else if( MTA_TNUMERIC_IS_ZERO( t2 ) ){
        if( MTA_TNUMERIC_IS_POSITIVE(t3) ){
            b1->value = MTA_TRUE;
        }else{
            b1->value = MTA_FALSE;
        }
    }else if( MTA_TNUMERIC_IS_ZERO( t3 ) ){
        if( MTA_TNUMERIC_IS_NEGATIVE(t2) ){
            b1->value = MTA_TRUE;
        }else{
            b1->value = MTA_FALSE;
        }
    }else{
        if( MTA_TNUMERIC_IS_POSITIVE(t2) ){
            if( MTA_TNUMERIC_IS_NEGATIVE(t3) ){
                b1->value = MTA_FALSE;
            }else{
                if( MTA_TNUMERIC_EXP(t2) < MTA_TNUMERIC_EXP(t3) ){
                    b1->value = MTA_TRUE;
                }else if( MTA_TNUMERIC_EXP(t2) > MTA_TNUMERIC_EXP(t3) ){
                    b1->value = MTA_FALSE;
                }else{
                    compare = acpMemCmp( t2->mantissa, t3->mantissa, sizeof(t2->mantissa) );
                    if( compare <= 0 ){
                        b1->value = MTA_TRUE;
                    }else{
                        b1->value = MTA_FALSE;
                    }
                }
            }
        }else{
            if( MTA_TNUMERIC_IS_POSITIVE(t3) ){
                b1->value = MTA_TRUE;
            }else{
                if( MTA_TNUMERIC_EXP(t2) > MTA_TNUMERIC_EXP(t3) ){
                    b1->value = MTA_TRUE;
                }else if( MTA_TNUMERIC_EXP(t2) < MTA_TNUMERIC_EXP(t3) ){
                    b1->value = MTA_FALSE;
                }else{
                    compare = acpMemCmp( t2->mantissa, t3->mantissa, sizeof(t2->mantissa) );
                    if( compare >= 0 ){
                        b1->value = MTA_TRUE;
                    }else{
                        b1->value = MTA_FALSE;
                    }		    
                }
            }
        }
    }
    
    return ACI_SUCCESS;
}

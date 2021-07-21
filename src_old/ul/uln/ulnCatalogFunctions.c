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

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnCatalogFunctions.h>

/*
 * Append single parameter by format
 */
// bug-25905: conn nls not applied to client lang module
// 인자 aFnContext 추가
acp_sint32_t ulnAppendFormatParameter(ulnFnContext     *aFnContext,
                                      acp_char_t       *aBuffer,
                                      acp_sint32_t      aBufferSize,
                                      acp_sint32_t      aBufferPos,
                                      const acp_char_t *aFormat,
                                      const acp_char_t *aName,
                                      acp_sint16_t     aLength)
{
    acp_char_t        sName[ULN_CATALOG_MAX_NAME_LEN];
    const acp_char_t *s        = aName;
    acp_sint32_t      sResult  = aBufferPos;
    ulnDbc*           sDbc     = NULL;
    acp_rc_t          sRet;

    // bug-25905: conn nls not applied to client lang module
    ACI_TEST(aFnContext->mHandle.mStmt == NULL);
    sDbc = aFnContext->mHandle.mStmt->mParentDbc;

    if( s != NULL )
    {
        /* precalculation */
        aBufferSize = ( aBufferSize - aBufferPos );
        aBuffer    += aBufferPos;

        /* no buffer's space left */
        ACI_TEST( aBufferSize <= 0 );


        switch(aLength)
        {
            case SQL_NULL_DATA:               //(-1)
                return 0;

            case SQL_NTS:                     //(-3)
                aLength = acpCStrLen( s, aBufferSize );
                break;

            case SQL_DATA_AT_EXEC:            //(-2)
            case SQL_IS_POINTER:              //(-4)
            case SQL_IS_UINTEGER:             //(-5)
            case SQL_IS_INTEGER :             //(-6)
            case SQL_IS_USMALLINT:            //(-7)
            case SQL_IS_SMALLINT:             //(-8)
            default:
                if( aLength < SQL_LEN_DATA_AT_EXEC_OFFSET)
                {
                    aLength = (-aLength) + SQL_LEN_BINARY_ATTR_OFFSET;
                }
        }


        ACI_TEST( aLength < 0 );

        /* remove lead and tail whaitespaces */
        while(aLength > 0 && s[ 0 ] == ' ')
        {
            s++;
            aLength--;
        }
        do
        {
            aLength--;
        }
        while(aLength > 0 && s[aLength] == ' ');
        ++aLength;

        ACI_TEST( aLength < 0 );

        if( aLength > 0 )
        {
            /* copy to the buffer */
            ACI_TEST( (acp_uint32_t)aLength > (ACI_SIZEOF( sName )-1) );//null

            // To Fix BUG-17430
            //
            // bug-25905: conn nls not applied to client lang module
            // 변경전: mtl::makeNameInSQL 호출
            // 변경후: 위의 함수 복사하여 새로 만든 ulnMakeNameInSQL 호출
            // why: mtl은 USASCII로 고정된 defModule만을 사용 (변경 불가)
            ACI_TEST( mtlMakeNameInSQL(sDbc->mClientCharsetLangModule,
                                       sName,
                                       (acp_char_t*)s,
                                       aLength)
                      != ACI_SUCCESS );

            sRet = acpSnprintf(aBuffer, aBufferSize, aFormat, sName);
            if (ACP_RC_IS_ETRUNC(sRet))
            {
                sResult = acpCStrLen(aFormat, ACI_SIZEOF(sName));
                sResult += aBufferPos;
            }
            else if (ACP_RC_IS_SUCCESS(sRet))
            {
                sResult = acpCStrLen(aBuffer, aBufferSize);
                sResult += aBufferPos;
            }
            else
            {
                ACI_TEST(1);
            }
        }
    }
    return sResult;

    ACI_EXCEPTION_END;

    return -1;
}


/*
 * old cli2 의 MAKE_NTS 매크로이다.
 * SQL 문장에서 사용될 Null Terminated Name을 생성한다.
 */

// bug-25905: conn nls not applied to client lang module
// 인자 aMtlModule 추가
ACI_RC ulnMakeNullTermNameInSQL(mtlModule        * aMtlModule,
                                acp_char_t       * aDest,
                                acp_uint32_t       aDestLen,
                                const acp_char_t * aSrc,
                                acp_sint32_t       aSrcLen,
                                const acp_char_t * aDefaultString )
{
    const acp_char_t* s = aSrc;

    if(s != NULL)
    {
        /* Length by Param */
        switch(aSrcLen)
        {
            case SQL_NULL_DATA:               //(-1)
                ACI_TEST(1);

            case SQL_NTS:                     //(-3)
                aSrcLen = acpCStrLen( s, aDestLen );
                break;

            case SQL_DATA_AT_EXEC:            //(-2)
            case SQL_IS_POINTER:              //(-4)
            case SQL_IS_UINTEGER:             //(-5)
            case SQL_IS_INTEGER :             //(-6)
            case SQL_IS_USMALLINT:            //(-7)
            case SQL_IS_SMALLINT:             //(-8)
            default:
                if( aSrcLen < SQL_LEN_DATA_AT_EXEC_OFFSET)
                {
                    aSrcLen = (-aSrcLen) + SQL_LEN_BINARY_ATTR_OFFSET;
                }
        }
    }
    else
    {
        s = aDefaultString;
        if( s != NULL )
        {
            aSrcLen = acpCStrLen( s, aDestLen );
        }
        else
        {
            aSrcLen = 0;
        }
    }

    ACI_TEST(aSrcLen < 0);

    // To Fix BUG-17430
    //   SQL 구문을 생성하기 위한 문자열로 사용됨
    // To Fix BUG-17803
    //   SQL_NTS 로 취급되서는 안됨.

    // bug-25905: conn nls not applied to client lang module
    // 변경전: mtl::makeNameInSQL 호출
    // 변경후: 위의 함수 복사하여 새로 만든 ulnMakeNameInSQL 호출
    // why: mtl은 USASCII로 고정된 defModule만을 사용
    ACI_TEST( mtlMakeNameInSQL(aMtlModule, aDest, (acp_char_t*) s, aSrcLen)
              != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnCheckStringLength(acp_sint32_t aLength, acp_sint32_t aMaxLength)
{
    if(aLength < 0)
    {
        ACI_TEST(aLength != SQL_NTS);
    }
    else
    {
        if(aMaxLength != 0)
        {
            ACI_TEST(aLength > aMaxLength);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

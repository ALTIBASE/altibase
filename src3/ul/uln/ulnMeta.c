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
#include <ulnTypes.h>
#include <ulnMeta.h>

void ulnMetaInitialize(ulnMeta *aMeta)
{
    aMeta->mCTYPE                = ULN_CTYPE_MAX;
    aMeta->mMTYPE                = ULN_MTYPE_MAX;

    aMeta->mTypeIdFromUser       = 0;

    aMeta->mLanguage             = 0;

    /*
     * ODBC 속성들
     */

    aMeta->mOdbcType             = 0;
    aMeta->mConciseType          = 0;
    aMeta->mDatetimeIntCode      = 0;
    aMeta->mDatetimeIntPrecision = 0;

    aMeta->mLength               = ULN_vULEN(0);
    aMeta->mOctetLength          = ULN_vLEN(0);

    aMeta->mOdbcPrecision        = 0;
    aMeta->mOdbcScale            = 0;
    aMeta->mFixedPrecScale       = 0;
    aMeta->mNumPrecRadix         = 0;

    aMeta->mNullable             = SQL_NULLABLE_UNKNOWN;
    aMeta->mUpdatable            = SQL_ATTR_READWRITE_UNKNOWN;
}

/*
 * Note : IRD 레코드의 타입 정보는 어차피 사용자에게 정보만 제공하는 용도이므로
 *        대충 넣어도 문제 없을 듯 하다. 실행시켜 가면서 버그는 수정하도록 한다.
 */
void ulnMetaBuild4Ird(ulnDbc       *aDbc,
                      ulnMeta      *aMeta,
                      ulnMTypeID    aMTYPE,
                      ulvSLen       aPrecision,
                      acp_sint32_t  aScale,
                      acp_uint8_t   aFlags)
{
    acp_sint16_t sSQL_TYPE;

    ulnMetaSetMTYPE(aMeta, aMTYPE);

    sSQL_TYPE = ulnTypeMap_MTYPE_SQL(aMTYPE);

    ulnMetaSetOdbcType(aMeta, ulnTypeGetOdbcVerboseType(sSQL_TYPE));
    ulnMetaSetOdbcConciseType(aMeta, sSQL_TYPE);
    ulnMetaSetOdbcDatetimeIntCode(aMeta, ulnTypeGetOdbcDatetimeIntCode(sSQL_TYPE));

    ulnMetaSetOdbcLength(aMeta, ULN_vULEN(0));
    ulnMetaSetOctetLength(aMeta, ULN_vLEN(0));

    ulnMetaSetPrecision(aMeta, 0);
    ulnMetaSetScale(aMeta, 0);

    if ((aFlags & CMP_DB_BIND_FLAGS_NULLABLE) == CMP_DB_BIND_FLAGS_NULLABLE)
    {
        ulnMetaSetNullable(aMeta, SQL_NULLABLE);
    }
    else
    {
        ulnMetaSetNullable(aMeta, SQL_NO_NULLS);
    }
    if ((aFlags & CMP_DB_BIND_FLAGS_UPDATABLE) == CMP_DB_BIND_FLAGS_UPDATABLE)
    {
        ulnMetaSetUpdatable(aMeta, SQL_ATTR_WRITE);
    }
    else
    {
        ulnMetaSetUpdatable(aMeta, SQL_ATTR_READONLY);
    }

    // BUG-18607
    ulnMetaSetFixedPrecScale(aMeta, SQL_FALSE);

    switch(aMTYPE)
    {
        case ULN_MTYPE_VARCHAR:
        case ULN_MTYPE_CHAR:
        case ULN_MTYPE_BINARY:
        case ULN_MTYPE_BYTE:
        case ULN_MTYPE_VARBYTE:
            ulnMetaSetOdbcLength(aMeta, aPrecision);
            ulnMetaSetOctetLength(aMeta, aPrecision);
            break;

        case ULN_MTYPE_NIBBLE:
            ulnMetaSetOdbcLength(aMeta, aPrecision);
            /*
             * 마지막의 + 1 은 길이 정보가 들어가는 NIBBLE 헤더이다.
             */
            ulnMetaSetOctetLength(aMeta, (aPrecision + 1) / 2 + ACI_SIZEOF(acp_uint8_t));
            break;

        case ULN_MTYPE_BIT:
        case ULN_MTYPE_VARBIT:
            ulnMetaSetOdbcLength(aMeta, aPrecision);
            /*
             * 마지막의 + ACI_SIZEOF(acp_uint32_t) 는 길이 정보가 들어가는 VARBIT 헤더이다.
             */
            ulnMetaSetOctetLength(aMeta, (aPrecision + 7) / 8 + ACI_SIZEOF(acp_uint32_t));
            ulnMetaSetPrecision(aMeta, aPrecision);
            ulnMetaSetScale(aMeta, aScale);
            break;

        case ULN_MTYPE_SMALLINT:
            ulnMetaSetOdbcLength(aMeta, ACI_SIZEOF(acp_uint16_t));
            ulnMetaSetOctetLength(aMeta, ACI_SIZEOF(acp_uint16_t));
            break;

        case ULN_MTYPE_INTEGER:
            ulnMetaSetOdbcLength(aMeta, ACI_SIZEOF(acp_uint32_t));
            ulnMetaSetOctetLength(aMeta, ACI_SIZEOF(acp_uint32_t));
            break;

        case ULN_MTYPE_NUMBER:
        case ULN_MTYPE_NUMERIC:
            ulnMetaSetOdbcLength(aMeta, ACI_SIZEOF(cmtNumeric));
            ulnMetaSetOctetLength(aMeta, ACI_SIZEOF(cmtNumeric));
            ulnMetaSetPrecision(aMeta, aPrecision);
            ulnMetaSetScale(aMeta, aScale);
            if (aScale != 0)
            {
                // BUG-18607
                ulnMetaSetFixedPrecScale(aMeta, SQL_TRUE);
            }
            break;

        case ULN_MTYPE_FLOAT:   // BUGBUG
            ulnMetaSetOdbcLength(aMeta, ACI_SIZEOF(cmtNumeric));
            ulnMetaSetOctetLength(aMeta, ACI_SIZEOF(cmtNumeric));
            ulnMetaSetPrecision(aMeta, aPrecision);
            ulnMetaSetScale(aMeta, aScale);
            break;

        case ULN_MTYPE_REAL:
            ulnMetaSetOdbcLength(aMeta, ACI_SIZEOF(acp_float_t));   // BUGBUG
            ulnMetaSetOctetLength(aMeta, ACI_SIZEOF(acp_float_t));   // BUGBUG
            break;

        case ULN_MTYPE_DATE:        // 서버에서 올라오지 않는 타입.
        case ULN_MTYPE_TIME:        // 서버에서 올라오지 않는 타입.
        case ULN_MTYPE_TIMESTAMP:
            // proj_2160 cm_type removal
            // mtdDateType(8bytes)  크기를 사용하지 않고
            // cmtDateTime(16bytes) 크기를 그대로 사용하기로 함
            // 이유: 크기가 적어지면 호환성에 문제가 생길까봐
            // cf) OLEDB에서 이 크기를 컬럼버퍼크기로 사용함
            ulnMetaSetOdbcLength(aMeta, ACI_SIZEOF(cmtDateTime));
            ulnMetaSetOctetLength(aMeta, ACI_SIZEOF(cmtDateTime));
            break;

        case ULN_MTYPE_DOUBLE:
            ulnMetaSetOdbcLength(aMeta, ACI_SIZEOF(acp_double_t));
            ulnMetaSetOctetLength(aMeta, ACI_SIZEOF(acp_double_t));
            break;

        case ULN_MTYPE_BIGINT:
            ulnMetaSetOdbcLength(aMeta, ACI_SIZEOF(acp_uint64_t));
            ulnMetaSetOctetLength(aMeta, ACI_SIZEOF(acp_uint64_t));
            break;

        case ULN_MTYPE_INTERVAL:
            ulnMetaSetOdbcLength(aMeta, ACI_SIZEOF(cmtInterval));
            ulnMetaSetOctetLength(aMeta, ACI_SIZEOF(cmtInterval));
            break;

        case ULN_MTYPE_NULL:
            ulnMetaSetOdbcLength(aMeta, 0);
            ulnMetaSetOctetLength(aMeta, 0);
            break;

        case ULN_MTYPE_CLOB:
        case ULN_MTYPE_BLOB:

            // To Fix BUG-21276
            // MSDN 에 의하면 다음과 같이 정의하고 있다.
            //   All binary types[a] :
            //      The number of bytes required to hold the defined
            //      (for fixed types)
            //      or maximum (for variable types) number of characters.
            //   If the driver cannot determine the column
            //      or parameter length for variable types,
            //      it returns SQL_NO_TOTAL(-4).
            // 그러나, ALTIBASE는 SQL_LONGVARBINARY 와 같은 형태를
            //   서버 내부적으로 지원하지 않고 있다.
            //   대안으로 BLOB 또는 SQL_BINARY 가 가질수 있는 최대값을 설정한다.

            ulnMetaSetOdbcLength(aMeta, ACP_SINT32_MAX);
            ulnMetaSetOctetLength(aMeta, ACP_SINT32_MAX);
            break;

        case ULN_MTYPE_GEOMETRY:
            ulnMetaSetOdbcLength(aMeta, aPrecision);
            ulnMetaSetOctetLength(aMeta, aPrecision);
            break;

        case ULN_MTYPE_NVARCHAR:
        case ULN_MTYPE_NCHAR:
            ulnMetaSetOdbcLength(aMeta, aPrecision);
            ulnMetaSetOctetLength(aMeta, aDbc->mNcharCharsetLangModule->maxPrecision(aPrecision));
            break;

            /*
             * 아래의 두 타입은 오로지 사용자가 lob locator 로 바인드 했을 경우의
             * out binding 을 위해서만 존재한다.
             */
        case ULN_MTYPE_CLOB_LOCATOR:
        case ULN_MTYPE_BLOB_LOCATOR:

        case ULN_MTYPE_MAX:
            ACE_ASSERT(0);
    }
}

void ulnMetaBuild4IpdByMeta(ulnMeta      *aMeta,
                            ulnMTypeID    aMTYPE,
                            acp_sint16_t  aSQL_TYPE,
                            ulvULen       aColumnSize,
                            acp_sint16_t  aDecimalDigits)
{
    acp_sint16_t sSQL_TYPE = 0;

    ulnMetaSetMTYPE(aMeta, aMTYPE);

    /*PROJ-2638 shard native linker*/
    if ( aSQL_TYPE >= ULSD_INPUT_RAW_MTYPE_NULL )
    {
        sSQL_TYPE = ulnTypeMap_MTYPE_SQL( aMTYPE );
    }
    else
    {
        sSQL_TYPE = aSQL_TYPE;
    }

    /*PROJ-2638 shard native linker*/
    ulnMetaSetOdbcType( aMeta, ulnTypeGetOdbcVerboseType( sSQL_TYPE ) );
    ulnMetaSetOdbcConciseType( aMeta, sSQL_TYPE );
    ulnMetaSetOdbcDatetimeIntCode( aMeta, ulnTypeGetOdbcDatetimeIntCode( sSQL_TYPE ) );

    ulnMetaSetOdbcLength(aMeta, ULN_vULEN(0));
    ulnMetaSetOctetLength(aMeta, ULN_vLEN(0));
    ulnMetaSetPrecision(aMeta, 0);
    ulnMetaSetScale(aMeta, 0);

    ulnMetaSetNullable(aMeta, SQL_NULLABLE_UNKNOWN);

    /*
     * Note : ColumnSize 의 의미
     * If ParamType is
     *      SQL_CHAR
     *      SQL_VARCHAR
     *      SQL_LONGVARCHAR
     *      SQL_BINARY
     *      SQL_VARBINARY
     *      SQL_LONGVARBINARY
     * the SQL_DESC_LENGTH field of the IPD is set to ColumnSize.
     *
     * If ParamType is
     *      SQL_DECIMAL
     *      SQL_NUMERIC
     *      SQL_FLOAT
     *      SQL_REAL
     *      SQL_DOUBLE
     * the SQL_DESC_PRECISION field of the IPD is set to ColumnSize
     *
     * For all other data types, the ColumnSize argument is ignored.
     *
     * Note : DecimalDigits 의 의미
     * If ParamType is
     *      SQL_TYPE_TIME,
     *      SQL_TYPE_TIMESTAMP,
     *      SQL_INTERVAL_SECOND,
     *      SQL_INTERVAL_DAY_TO_SECOND,
     *      SQL_INTERVAL_HOUR_TO_SECOND,
     *      SQL_INTERVAL_MINUTE_TO_SECOND,
     * the SQL_DESC_PRECISION field of the IPD is set to DecimalDigits.
     *
     * If ParamType is
     *      SQL_NUMERIC
     *      SQL_DECIMAL,
     * the SQL_DESC_SCALE field of the IPD is set to DecimalDigits.
     *
     * For all other data types, the DecimalDigits argument is ignored.
     *
     * Note : Summary
     *  1. SQL_NUMERIC, SQL_DECIMAL 의 서버측 타입
     *      ColumnSize 는 precision,
     *      DecimalDigits 는 scale 이다.
     *  2. SQL_FLOAT, SQL_REAL, SQL_DOUBLE 의 서버측 타입
     *      ColumnSize 가 precision.
     *  4. SQL_TYPE_TIME, SQL_TYPE_TIMESTAMP, SQL_INTERVAL_XXX 등의 서버측 타입
     *      DecimalDigits 가 precision
     *  5. SQL_CHAR, SQL_VARCHAR, SQL_BINARY, SQL_VARBINARY 등의 서버측 타입
     *      ColumnSize 는 길이.
     */
    // BUG-18607
    ulnMetaSetFixedPrecScale(aMeta, SQL_FALSE);

    switch(aMTYPE)
    {
        case ULN_MTYPE_VARCHAR:
        case ULN_MTYPE_CHAR:
        case ULN_MTYPE_BINARY:
        case ULN_MTYPE_NIBBLE:
        case ULN_MTYPE_VARBIT:
        case ULN_MTYPE_BIT:
        case ULN_MTYPE_GEOMETRY:
            ulnMetaSetOdbcLength(aMeta, aColumnSize);
            ulnMetaSetOctetLength(aMeta, aColumnSize);
            break;

        case ULN_MTYPE_BYTE:
        case ULN_MTYPE_VARBYTE:
            ulnMetaSetOdbcLength(aMeta, aColumnSize);
            /* bug-31397: large sql_byte params make buffer overflow.
             * sql_byte has double size of it's precision if sql_c_char type */
            ulnMetaSetOctetLength(aMeta, 2*aColumnSize);
            break;

        case ULN_MTYPE_NUMBER:
        case ULN_MTYPE_NUMERIC:
            aColumnSize = ACP_MIN( aColumnSize, ULN_MAX_NUMERIC_PRECISION );
            ulnMetaSetPrecision(aMeta, aColumnSize);
            ulnMetaSetScale(aMeta, aDecimalDigits);
            ulnMetaSetOdbcLength(aMeta, ulnTypeGetColumnSizeOfType(aMTYPE, aMeta));
            ulnMetaSetOctetLength(aMeta, ulnTypeGetColumnSizeOfType(aMTYPE, aMeta));
            if (aDecimalDigits != 0)
            {
                // BUG-18607
                ulnMetaSetFixedPrecScale(aMeta, SQL_TRUE);
            }
            break;

        case ULN_MTYPE_REAL:
        case ULN_MTYPE_FLOAT:
        case ULN_MTYPE_DOUBLE:
            ulnMetaSetPrecision(aMeta, aColumnSize);
            ulnMetaSetOdbcLength(aMeta, ulnTypeGetColumnSizeOfType(aMTYPE, aMeta));
            ulnMetaSetOctetLength(aMeta, ulnTypeGetColumnSizeOfType(aMTYPE, aMeta));
            break;

        case ULN_MTYPE_DATE:
        case ULN_MTYPE_TIME:
        case ULN_MTYPE_TIMESTAMP:
        case ULN_MTYPE_INTERVAL:
            ulnMetaSetPrecision(aMeta, aDecimalDigits);
            ulnMetaSetOdbcLength(aMeta, ulnTypeGetColumnSizeOfType(aMTYPE, aMeta));
            ulnMetaSetOctetLength(aMeta, ulnTypeGetColumnSizeOfType(aMTYPE, aMeta));
            break;

        case ULN_MTYPE_NULL:

        case ULN_MTYPE_BLOB:
        case ULN_MTYPE_CLOB:
            break;

        case ULN_MTYPE_SMALLINT:
        case ULN_MTYPE_INTEGER:
        case ULN_MTYPE_BIGINT:
            ulnMetaSetOdbcLength(aMeta, ulnTypeGetColumnSizeOfType(aMTYPE, aMeta));
            ulnMetaSetOctetLength(aMeta, ulnTypeGetColumnSizeOfType(aMTYPE, aMeta));
            break;

        case ULN_MTYPE_NVARCHAR:
        case ULN_MTYPE_NCHAR:
            ulnMetaSetOdbcLength(aMeta, aColumnSize);
            ulnMetaSetOctetLength(aMeta, aColumnSize * ULN_MAX_CONVERSION_RATIO);
            break;

            /*
             * 아래의 두 타입은 오로지 사용자가 lob locator 로 바인드 했을 경우의
             * out binding 을 위해서만 존재한다.
             * 아래의 두 타입이 쓰이는 경우는 ulnBindInfoBuild4Param() 에서 볼 수 있다.
             * 절대로 사용자가 지정한 타입이 이리로 매핑되는 경우는 없어야 한다.
             */
        case ULN_MTYPE_CLOB_LOCATOR:
        case ULN_MTYPE_BLOB_LOCATOR:
        case ULN_MTYPE_MAX:
            ACE_ASSERT(0);
            break;
    }
}

void ulnMetaBuild4IpdByStmt(ulnStmt      *aStmt,
                            acp_sint16_t  aRecNumber,
                            acp_sint16_t  aFieldIdentifier,
                            acp_uint32_t  aValue)
{
    ulnDescRec   *sDescRecIpd;
    ulnMTypeID    sMetaMTYPE;
    acp_sint16_t  sSTYPE;
    acp_ulong_t   sColumnSize;
    acp_sint16_t  sDecimalDigits;

    /*
     * BUG-28623 [CodeSonar]Null Pointer Dereference
     * ACE_DASSERT로 검사할 경우 release mode에서 NULL이 될 수 있음
     * 따라서 ACE_ASSERT로 검사
     */
    ACE_ASSERT( aStmt != NULL );

    sDescRecIpd = ulnStmtGetIpdRec(aStmt, aRecNumber);
    ACE_ASSERT( sDescRecIpd != NULL );

    if (&sDescRecIpd->mMeta != NULL)
    {
        sMetaMTYPE = ulnMetaGetMTYPE(&sDescRecIpd->mMeta);

        sColumnSize = ulnTypeGetColumnSizeOfType(sMetaMTYPE,
                                                 &sDescRecIpd->mMeta);
        sDecimalDigits = ulnTypeGetDecimalDigitsOfType(sMetaMTYPE,
                                                       &sDescRecIpd->mMeta);

        sSTYPE = ulnMetaGetOdbcConciseType( &sDescRecIpd->mMeta );

        switch(aFieldIdentifier)
        {
            case SQL_DESC_PRECISION:
                sColumnSize = (acp_ulong_t)aValue;
                break;
            case SQL_DESC_SCALE:
                sDecimalDigits = (acp_sint16_t)aValue;
                break;
            case SQL_DESC_TYPE:
                sSTYPE = (acp_sint16_t)aValue;
                break;
            default:
                break;
        }

        ulnMetaBuild4IpdByMeta( &sDescRecIpd->mMeta,
                                sMetaMTYPE,
                                sSTYPE,
                                sColumnSize,
                                sDecimalDigits );
    }
}

void ulnMetaAdjustIpdByMeta(ulnDbc       *aDbc,
                            ulnMeta      *aMeta,
                            acp_ulong_t   aColumnSize,
                            ulnMTypeID    aMTYPE)
{
    if( aColumnSize == ULN_DEFAULT_PRECISION )
    {
        switch( aMTYPE )
        {
            case ULN_MTYPE_BINARY:
                ulnMetaSetOdbcLength(aMeta, ULN_MAX_BINARY_PRECISION);
                ulnMetaSetOctetLength(aMeta, ULN_MAX_BINARY_PRECISION);
                break;
            case ULN_MTYPE_BYTE:
            case ULN_MTYPE_VARBYTE:
                ulnMetaSetOdbcLength(aMeta, ULN_MAX_BYTE_PRECISION);
                ulnMetaSetOctetLength(aMeta, ULN_MAX_BYTE_PRECISION);
                break;
            case ULN_MTYPE_NIBBLE:
                ulnMetaSetOdbcLength(aMeta, ULN_MAX_NIBBLE_PRECISION);
                ulnMetaSetOctetLength(aMeta, ULN_MAX_NIBBLE_PRECISION);
                break;
            case ULN_MTYPE_GEOMETRY:
                ulnMetaSetOdbcLength(aMeta, ULN_MAX_GEOMETRY_PRECISION);
                ulnMetaSetOctetLength(aMeta, ULN_MAX_GEOMETRY_PRECISION);
                break;
            case ULN_MTYPE_VARBIT:
            case ULN_MTYPE_BIT:
                ulnMetaSetOdbcLength(aMeta, ULN_MAX_VARBIT_PRECISION);
                ulnMetaSetOctetLength(aMeta, ULN_MAX_VARBIT_PRECISION);
                break;
            case ULN_MTYPE_NUMBER:
            case ULN_MTYPE_NUMERIC:
                ulnMetaSetPrecision(aMeta, ULN_MAX_NUMERIC_PRECISION);
                ulnMetaSetScale(aMeta, ULN_MAX_NUMERIC_SCALE);
                ulnMetaSetOdbcLength(aMeta, ulnTypeGetColumnSizeOfType(aMTYPE, aMeta));
                ulnMetaSetOctetLength(aMeta, ulnTypeGetColumnSizeOfType(aMTYPE, aMeta));
                break;
            case ULN_MTYPE_VARCHAR:
                ulnMetaSetOdbcLength(aMeta, ULN_MAX_VARCHAR_PRECISION);
                ulnMetaSetOctetLength(aMeta, ULN_MAX_VARCHAR_PRECISION);
                break;
            case ULN_MTYPE_CHAR:
                ulnMetaSetOdbcLength(aMeta, ULN_MAX_CHAR_PRECISION);
                ulnMetaSetOctetLength(aMeta, ULN_MAX_CHAR_PRECISION);
                break;
            case ULN_MTYPE_NCHAR:
                ulnMetaSetOdbcLength(aMeta,
                                     ULN_MAX_VARCHAR_PRECISION / aDbc->mNcharCharsetLangModule->maxPrecision(1));
                ulnMetaSetOctetLength(aMeta, ULN_MAX_VARCHAR_PRECISION);
                break;
            case ULN_MTYPE_NVARCHAR:
                ulnMetaSetOdbcLength(aMeta,
                                     ULN_MAX_VARCHAR_PRECISION / aDbc->mNcharCharsetLangModule->maxPrecision(1));
                ulnMetaSetOctetLength(aMeta, ULN_MAX_VARCHAR_PRECISION);
                break;
            default:
                break;
        }
    }
}

void ulnMetaAdjustIpdByStmt(ulnDbc       *aDbc,
                            ulnStmt      *aStmt,
                            acp_sint16_t  aRecNumber,
                            acp_ulong_t   aPrecision)
{
    ulnDescRec *sDescRecIpd;
    ulnMTypeID  sMetaMTYPE;

     /*
     * BUG-28623 [CodeSonar]Null Pointer Dereference
     * ACE_DASSERT로 검사할 경우 release mode에서 NULL이 될 수 있음
     * 따라서 ACE_ASSERT로 검사
     */
    ACE_ASSERT( aStmt != NULL );
    sDescRecIpd = ulnStmtGetIpdRec(aStmt, aRecNumber);

    ACE_ASSERT( sDescRecIpd != NULL );

    if (&sDescRecIpd->mMeta != NULL)
    {
        sMetaMTYPE = ulnMetaGetMTYPE(&sDescRecIpd->mMeta);

        ulnMetaAdjustIpdByMeta( aDbc,
                                &sDescRecIpd->mMeta,
                                aPrecision,
                                sMetaMTYPE );
    }
}

void ulnMetaBuild4ArdApd(ulnMeta      *aAppMeta,
                         ulnCTypeID    aCTYPE,
                         acp_sint16_t  aSQL_C_TYPE,
                         ulvSLen       aBufferLength)
{
    acp_sint16_t  sSQL_C_TYPE = 0;

    /*PROJ-2638 shard native linker*/
    if ( aSQL_C_TYPE >= ULSD_INPUT_RAW_MTYPE_NULL )
    {
        sSQL_C_TYPE = ulnTypeMap_CTYPE_SQLC( aCTYPE );
    }
    else
    {
        sSQL_C_TYPE = aSQL_C_TYPE;
    }

    ulnMetaSetCTYPE(aAppMeta, aCTYPE);

    ulnMetaSetOdbcType(aAppMeta, ulnTypeGetOdbcVerboseType(aSQL_C_TYPE));
    ulnMetaSetOdbcConciseType(aAppMeta, sSQL_C_TYPE);
    ulnMetaSetOdbcDatetimeIntCode(aAppMeta, ulnTypeGetOdbcDatetimeIntCode(sSQL_C_TYPE));

    ulnMetaSetOdbcLength(aAppMeta, ULN_vULEN(0));
    ulnMetaSetOctetLength(aAppMeta, ULN_vLEN(0));
    ulnMetaSetPrecision(aAppMeta, 0);
    ulnMetaSetScale(aAppMeta, 0);

    ulnMetaSetNullable(aAppMeta, SQL_NULLABLE_UNKNOWN);

    switch(aCTYPE)
    {
        case ULN_CTYPE_NULL:
            break;

        case ULN_CTYPE_BIT:
        case ULN_CTYPE_STINYINT:
        case ULN_CTYPE_UTINYINT:
            ulnMetaSetOctetLength(aAppMeta, ACI_SIZEOF(acp_uint8_t));
            break;

        case ULN_CTYPE_SSHORT:
        case ULN_CTYPE_USHORT:
            ulnMetaSetOctetLength(aAppMeta, ACI_SIZEOF(acp_uint16_t));
            break;

        case ULN_CTYPE_SLONG:
        case ULN_CTYPE_ULONG:
            ulnMetaSetOctetLength(aAppMeta, ACI_SIZEOF(acp_uint32_t));
            break;

        case ULN_CTYPE_FLOAT:
            ulnMetaSetOctetLength(aAppMeta, ACI_SIZEOF(acp_float_t));
            break;

        case ULN_CTYPE_DOUBLE:
            ulnMetaSetOctetLength(aAppMeta, ACI_SIZEOF(acp_double_t));
            break;

        case ULN_CTYPE_SBIGINT:
        case ULN_CTYPE_UBIGINT:
        case ULN_CTYPE_BLOB_LOCATOR:
        case ULN_CTYPE_CLOB_LOCATOR:
            /*
             * BUGBUG : BIGINT 구조체에 대한 고려를 해 줘야 한다.
             */
            ulnMetaSetOctetLength(aAppMeta, ACI_SIZEOF(acp_uint64_t));
            break;

        case ULN_CTYPE_DATE:
            ulnMetaSetOctetLength(aAppMeta, ACI_SIZEOF(SQL_DATE_STRUCT));
            break;

        case ULN_CTYPE_TIME:
            ulnMetaSetOctetLength(aAppMeta, ACI_SIZEOF(SQL_TIME_STRUCT));
            break;

        case ULN_CTYPE_TIMESTAMP:
            ulnMetaSetOctetLength(aAppMeta, ACI_SIZEOF(SQL_TIMESTAMP_STRUCT));
            break;

        case ULN_CTYPE_INTERVAL:
            ulnMetaSetOctetLength(aAppMeta, ACI_SIZEOF(SQL_INTERVAL_STRUCT));
            break;

        case ULN_CTYPE_NUMERIC:
            ulnMetaSetOctetLength(aAppMeta, ACI_SIZEOF(SQL_NUMERIC_STRUCT));
            break;

            /*
             * Note : CTYPE_DEFAULT 의 경우는 아직은 메타 정보를 결정할 단계가 아니다.
             *        단지, 사용자가 정확한 길이를 줘야만 하므로 사용자가 제공한
             *        길에에만 의존한다.
             */
        case ULN_CTYPE_DEFAULT:
        case ULN_CTYPE_BINARY:
        case ULN_CTYPE_CHAR:
        case ULN_CTYPE_WCHAR:
        case ULN_CTYPE_FILE:
            /*
             * FILE 의 경우에는 mSize, mPrecision 이 사용자 버퍼, 즉, 파일네임에 관한 정보임.
             */
            ulnMetaSetOctetLength(aAppMeta, aBufferLength);
            break;

        case ULN_CTYPE_MAX:
            ACE_ASSERT(0);
            break;
    }
}


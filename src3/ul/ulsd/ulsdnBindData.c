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
 

/**********************************************************************
 * $Id: ulsdnBindData.c 00000 2017-02-27 10:26:00Z swhors $
 **********************************************************************/
#include <uln.h>
#include <ulnPrivate.h>
#include <ulnBindParameter.h>
#include <ulnBindData.h>
#include <ulnPDContext.h>
#include <ulnConv.h>
#include <ulnCharSet.h>

/*
 * PROJ-2638 shard native linker
 * SQL_C_DEFAULT 로 바인딩시 어떤 타입을 가정해야 하는지 결정하는 함수.
 */
ulnCTypeID ulsdTypeGetDefault_UserType( ulnMTypeID aMTYPE )
{
    return ulnTypeMap_SQLC_CTYPE( ulnTypeGetDefault_SQL_C_TYPE( aMTYPE ) );
}

ulnCTypeID ulsdTypeMap_MTYPE_CTYPE( acp_sint16_t aMTYPE )
{
    switch (aMTYPE)
    {
        case ULN_MTYPE_NULL:
            return ULN_CTYPE_NULL;
        case ULN_MTYPE_NCHAR:
        case ULN_MTYPE_NVARCHAR:
        case ULN_MTYPE_VARCHAR:
        case ULN_MTYPE_CHAR:
        case ULN_MTYPE_BYTE:
        case ULN_MTYPE_VARBYTE:
            return ULN_CTYPE_CHAR;
        case ULN_MTYPE_SMALLINT:
            return ULN_CTYPE_SSHORT;
        case ULN_MTYPE_INTEGER:
            return ULN_CTYPE_SLONG;
        case ULN_MTYPE_BIGINT:
            return ULN_CTYPE_SBIGINT;
        case ULN_MTYPE_DOUBLE:
            return ULN_CTYPE_DOUBLE;
        case ULN_MTYPE_BIT:
            return ULN_CTYPE_BIT;
        case ULN_MTYPE_DATE:
            return ULN_CTYPE_DATE;
        case ULN_MTYPE_TIME:
            return ULN_CTYPE_TIME;
        case ULN_MTYPE_TIMESTAMP:
            return ULN_CTYPE_TIMESTAMP;
        case ULN_MTYPE_INTERVAL:
            return ULN_CTYPE_CHAR;
        case ULN_MTYPE_FLOAT:
        case ULN_MTYPE_NUMBER:
        case ULN_MTYPE_NUMERIC:
            return ULN_CTYPE_NUMERIC;
        case ULN_MTYPE_VARBIT:
        case ULN_MTYPE_NIBBLE:
        case ULN_MTYPE_BINARY:
        case ULN_MTYPE_REAL:
            return ULN_CTYPE_BINARY;
        case ULN_MTYPE_BLOB:
        case ULN_MTYPE_BLOB_LOCATOR:
            return ULN_CTYPE_BLOB_LOCATOR;
        case ULN_MTYPE_CLOB:
        case ULN_MTYPE_CLOB_LOCATOR:
            return ULN_CTYPE_CLOB_LOCATOR;
        default:
            return ULN_CTYPE_MAX;
    }
}

/*************************************************************
 * PROJ-2638 shard native linker
 * ulnParamMTDDataCopyToStmt
 *
 * Desc:
 *    mt 타입의 사용자 버퍼의 데이터를 STMT안의 DataPtr로 복사하는 함수.
 *
 * Argument:
 *   aStmt  [IN] - ulnStmt
 *   aDescRecApd [IN] - Application parameter Descriptor
 *   aStcPtr     [IN] - Data Buffer
 *
 * Return :
 *  ACP_SUCCESS, ACP_FAILURE
 *************************************************************/
ACI_RC ulsdParamMTDDataCopyToStmt( ulnStmt     * aStmt,
                                   ulnDescRec  * aDescRecApd,
                                   ulnDescRec  * aDescRecIpd,
                                   acp_uint8_t * aSrcPtr )
{
    acp_uint32_t sMTDType = 0;
    acp_uint32_t sColumnSize = 0;

    ACP_UNUSED( aDescRecApd );

    ACI_TEST( aStmt == NULL );
    ACI_TEST( aDescRecIpd == NULL );
    ACI_TEST( aSrcPtr == NULL );

    /* PROJ-2638 Native Shard Linker */
    if ( aDescRecIpd->mMeta.mMTYPE >= ULSD_INPUT_RAW_MTYPE_NULL )
    {
        sMTDType = ulnTypeMap_MTYPE_MTD( aDescRecIpd->mMeta.mMTYPE - ULSD_INPUT_RAW_MTYPE_NULL );
    }
    else
    {
        sMTDType = ulnTypeMap_MTYPE_MTD( aDescRecIpd->mMeta.mMTYPE );
    }

    switch ( sMTDType )
    {
        case MTD_NULL_ID:
            break;
        case MTD_NVARCHAR_ID:
        case MTD_NCHAR_ID:
        case MTD_CHAR_ID:
        case MTD_VARCHAR_ID:
        {
            mtdCharType *sChar = (mtdCharType *)aSrcPtr;
            ULN_CHUNK_WR2( aStmt, &sChar->length );
            if ( sChar->length > 0 )
            {
                ULN_CHUNK_WCP( aStmt, sChar->value, sChar->length );
            }
            else
            {
                /* Do Nothing. */
            }
            break;
        }
        case MTD_NUMBER_ID:
        case MTD_FLOAT_ID:
        case MTD_NUMERIC_ID:
        {
            mtdNumericType *sNumeric = (mtdNumericType *)aSrcPtr;
            ULN_CHUNK_WR1( aStmt, sNumeric->length );
            if ( sNumeric->length > 0 )
            {
                ULN_CHUNK_WR1( aStmt, sNumeric->signExponent );
                ULN_CHUNK_WCP( aStmt, sNumeric->mantissa, sNumeric->length - 1 );
            }
            break;
        }
        case MTD_SMALLINT_ID:
        {
            mtdSmallintType *sSmallIntValue = (mtdSmallintType *)aSrcPtr;
            ULN_CHUNK_WR2( aStmt, sSmallIntValue );
            break;
        }
        case MTD_INTEGER_ID:
        {
            mtdIntegerType *sIntegerValue = (mtdIntegerType *)aSrcPtr;
            ULN_CHUNK_WR4( aStmt, sIntegerValue );
            break;
        }
        case MTD_BIGINT_ID:
        {
            mtdBigintType *sBigIntValue = (mtdBigintType *)aSrcPtr;
            ULN_CHUNK_WR8( aStmt, sBigIntValue );
            break;
        }
        case MTD_REAL_ID:
        {
            mtdRealType *sRealValue = (mtdRealType *)aSrcPtr;
            ULN_CHUNK_WR4( aStmt, sRealValue );
            break;
        }
        case MTD_DOUBLE_ID:
        {
            mtdDoubleType *sDoubleType = (mtdDoubleType *)aSrcPtr;
            ULN_CHUNK_WR8( aStmt, sDoubleType );
            break;
        }
        case MTD_BINARY_ID:
        {
            mtdBinaryType *sBinaryType = (mtdBinaryType *)aSrcPtr;
            acp_uint32_t   sPadding    = 0;
            ULN_CHUNK_WR4( aStmt, &sBinaryType->mLength );
            ULN_CHUNK_WR4( aStmt, &sPadding ); // meaningless
            if ( sBinaryType->mLength > 0 )
            {
                ULN_CHUNK_WCP( aStmt, sBinaryType->mValue, sBinaryType->mLength );
            }
            else
            {
                /*Null Data*/
            }
            break;
        }
        case MTD_NIBBLE_ID:
        {
            mtdNibbleType *sNibble = (mtdNibbleType *)aSrcPtr;
            ULN_CHUNK_WR1( aStmt, sNibble->length );
            if( (sNibble->length) != MTD_NIBBLE_NULL_LENGTH )
            {
                sColumnSize = ((sNibble->length + 1) >> 1);
                ULN_CHUNK_WCP( aStmt, sNibble->value, sColumnSize );
            }
            else
            {
                /*NULL Data*/
            }
            break;
        }
        case MTD_VARBYTE_ID:
        case MTD_BYTE_ID:
        {
            mtdByteType *sByte = (mtdByteType *)aSrcPtr;
            ULN_CHUNK_WR2( aStmt, &sByte->length );
            if ( sByte->length > 0 )
            {
                ULN_CHUNK_WCP( aStmt, sByte->value, sByte->length );
            }
            else
            {
                /*Null Data*/
            }
            break;
        }
        case MTD_VARBIT_ID:
        case MTD_BIT_ID:
        {
            mtdBitType *sBit = (mtdBitType *)aSrcPtr;
            sColumnSize = BIT_TO_BYTE( sBit->length );
            ULN_CHUNK_WR4( aStmt, &sBit->length );
            ULN_CHUNK_WCP( aStmt, sBit->value, sColumnSize );
            break;
        }
        case MTD_DATE_ID:
        {
            mtdDateType *sDate = (mtdDateType *)aSrcPtr;
            ULN_CHUNK_WR2( aStmt, &sDate->year );
            ULN_CHUNK_WR2( aStmt, &sDate->mon_day_hour );
            ULN_CHUNK_WR4( aStmt, &sDate->min_sec_mic );
            break;
        }
        case MTD_INTERVAL_ID:
        {
            mtdIntervalType *sInterval = (mtdIntervalType *)aSrcPtr;
            ULN_CHUNK_WR8( aStmt, &sInterval->second );
            ULN_CHUNK_WR8( aStmt, &sInterval->microsecond );
            break;
        }
        case MTD_GEOMETRY_ID:
        case MTD_BLOB_LOCATOR_ID:
        case MTD_CLOB_LOCATOR_ID:
        default:
            ACI_RAISE(LEBEL_NOT_SUPPORT_TYPE);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LEBEL_NOT_SUPPORT_TYPE)
    {

    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*************************************************************
 * PROJ-2638 shard native linker
 * ulsdParamProcess_DATAs_ShardCore
 *
 * Desc:
 *    mt 타입의 사용자 버퍼의 데이터를 STMT안의 DataPtr로 복사하는 함수.
 *
 * Argument:
 *   aFnContext  [IN]  - ulnFnContext
 *   aStmt       [IN]  - ulnStmt
 *   aDescRecIpd [IN]  - Implemented parameter Descriptor
 *   aParamNumber[IN]  - acp_uint32_t
 *   aRowNumber  [IN]  - acp_uint32_t
 *   aUserDataPtr[OUT] - Data Buffer
 *
 * Return :
 *  ACP_SUCCESS, ACP_FAILURE
 *************************************************************/
ACI_RC ulsdParamProcess_DATAs_ShardCore( ulnFnContext * aFnContext,
                                         ulnStmt      * aStmt,
                                         ulnDescRec   * aDescRecApd,
                                         ulnDescRec   * aDescRecIpd,
                                         acp_uint32_t   aParamNumber,
                                         acp_uint32_t   aRowNumber,
                                         void         * aUserDataPtr )
{
	ACI_TEST_RAISE( ulsdParamMTDDataCopyToStmt( aStmt,
	                                            aDescRecApd,
	                                            aDescRecIpd,
	                                            aUserDataPtr ) != ACI_SUCCESS,
	                LABEL_COPY_ERR );

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_COPY_ERR)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         aParamNumber,
                         ulERR_ABORT_ACS_INVALID_PARAMETER_RANGE,
                         "ulnParamProcess_DATAs");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**************************************************************
 * ulsdDataCopyFromMT
 *  - PROJ-2638
 *  - CM 버퍼의 데이터를 MT 형태로 사용자 영역으로 복사한다.
 *  - 서버에서 반환된 데이터를 MT 타입 그대로 SDL에 넘기기 위해서 사용 된다.
 *  - ulsdCacheRowCopyToUserBufferShardCore에서 사용된다.
 *
 * aFnContext [IN] - ulnFnContext
 * aSrc       [IN] - CM 버퍼
 * aDes       [OUT]- ulnCache 안의 버퍼
 * aDesLen    [IN] - 버퍼의 크기
 * aColumn    [OUT]- 컬럼 정보
 * aMaxLen    [IN] - 데이터 컬럼의 최대 길이
 **************************************************************/
ACI_RC ulsdDataCopyFromMT( ulnFnContext * aFnContext,
                           acp_uint8_t  * aSrc,
                           acp_uint8_t  * aDes,
                           acp_uint32_t   aDesLen,
                           ulnColumn    * aColumn,
                           acp_uint32_t   aMaxLen )
{
    acp_uint32_t   sStructSize = 0;

    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    ulnLob        *sLob;
    acp_uint64_t   sLobLocatorId;
    acp_uint64_t   sLobSize;

    ulnStmt       *sStmt = aFnContext->mHandle.mStmt;

    switch ( aColumn->mMtype )
    {
        case ULN_MTYPE_NULL :
            {
                *(aDes + 0) = *(aSrc + 0);
                aColumn->mDataLength = SQL_NULL_DATA;
                aColumn->mMTLength   = 1;
                aColumn->mPrecision  = 0;
            }
            break;

        case ULN_MTYPE_BINARY :
            {
                mtdBinaryType * sData = NULL;
                ACI_TEST(aDesLen < aMaxLen);
                sData = (mtdBinaryType *) aDes;
                CM_ENDIAN_ASSIGN4( &sData->mLength, aSrc );
                sStructSize = MTD_BINARY_TYPE_STRUCT_SIZE( sData->mLength );
                ACI_TEST(sStructSize > aMaxLen);
                acpMemCpy( &sData->mPadding,
                           (void*)&sData->mLength,
                           ACI_SIZEOF(acp_uint8_t) * 4 );
                acpMemCpy( &sData->mValue,
                           aSrc+8,
                           (sStructSize-8) ); /* value = aSrc + length(4) + padding(4) */
                aColumn->mDataLength = sData->mLength;
                aColumn->mMTLength   = aColumn->mDataLength + 8;
                aColumn->mPrecision  = 0;
                if ( aColumn->mDataLength == 0 )
                {
                    aColumn->mDataLength = SQL_NULL_DATA;
                }
            }
            break;

        case ULN_MTYPE_CHAR :
        case ULN_MTYPE_VARCHAR :
        case ULN_MTYPE_NCHAR :
        case ULN_MTYPE_NVARCHAR :
            {
                mtdCharType * sData = NULL;
                ACI_TEST( aDesLen < aMaxLen );
                sData = (mtdCharType *)aDes;
                CM_ENDIAN_ASSIGN2( &sData->length, aSrc );
                sStructSize = MTD_CHAR_TYPE_STRUCT_SIZE( sData->length );
                ACI_TEST( sStructSize > aMaxLen );
                acpMemSet( sData->value, ' ', (aMaxLen - 2) );
                acpMemCpy( &sData->value, aSrc+2, sData->length );
                aColumn->mBuffer     = NULL;
                aColumn->mDataLength = sData->length;
                aColumn->mMTLength   = aColumn->mDataLength + 2;
                aColumn->mPrecision  = 0;

                if ( aColumn->mDataLength == 0)
                {
                    aColumn->mDataLength = SQL_NULL_DATA;
                }
            }
            break;
        case ULN_MTYPE_NUMBER :
        case ULN_MTYPE_FLOAT :
        case ULN_MTYPE_NUMERIC :
            {
                mtdNumericType * sData  = NULL;
                ACI_TEST( aDesLen < aMaxLen );
                sData = (mtdNumericType *)aDes;
                CM_ENDIAN_ASSIGN1( sData->length, *aSrc );

                if ( sData->length != 0 )
                {
                    ACI_TEST( sData->length > aMaxLen );
                    sData->signExponent = *((acp_uint8_t*)aSrc + 1);
                    acpMemCpy( sData->mantissa,
                               ( aSrc + 2 ),
                               ( sData->length - sizeof(acp_uint8_t) ) );
                    aColumn->mDataLength = ACI_SIZEOF( cmtNumeric );
                    aColumn->mMTLength   = sData->length + 1;
                    aColumn->mPrecision  = 0;
                }
                else
                {
                    aColumn->mDataLength = SQL_NULL_DATA;
                    aColumn->mMTLength   = 1;
                    aColumn->mPrecision  = 0;
                }
            }
            break;

        case ULN_MTYPE_SMALLINT :
            {
                mtdSmallintType *sValue  = NULL;
                ACI_TEST( aDesLen < 2 );
                sValue = (mtdSmallintType *)aDes;
                CM_ENDIAN_ASSIGN2( sValue, aSrc );
                aColumn->mDataLength = 2;
                aColumn->mMTLength   = 2;
                aColumn->mPrecision  = 0;
            }
            break;

        case ULN_MTYPE_INTEGER :
            {
                mtdIntegerType *sValue  = NULL;
                ACI_TEST( aDesLen < 4 );
                sValue = (mtdIntegerType *)aDes;
                CM_ENDIAN_ASSIGN4( sValue, aSrc );
                aColumn->mDataLength = 4;
                aColumn->mMTLength   = 4;
                aColumn->mPrecision  = 0;

                if (*((acp_sint32_t*)sValue) == MTD_INTEGER_NULL )
                {
                    aColumn->mDataLength = SQL_NULL_DATA;
                }
            }
            break;

        case ULN_MTYPE_BIGINT :
            {
                mtdBigintType *sValue = NULL;
                ACI_TEST( aDesLen < 8 );
                sValue = (mtdBigintType *)aDes;
                CM_ENDIAN_ASSIGN8( sValue, aSrc );
                aColumn->mDataLength = 8;
                aColumn->mMTLength   = 8;
                aColumn->mPrecision  = 0;
            }
            break;

        case ULN_MTYPE_REAL :
            {
                mtdRealType *sValue = NULL;
                ACI_TEST( aDesLen < 4 );
                sValue = (mtdRealType *)aDes;
                CM_ENDIAN_ASSIGN4( sValue, aSrc );
                aColumn->mDataLength = 4;
                aColumn->mMTLength   = 4;
                aColumn->mPrecision  = 0;
            }
            break;

        case ULN_MTYPE_DOUBLE :
            {
                mtdDoubleType *sValue = NULL;
                ACI_TEST( aDesLen < 8 );
                sValue = (mtdDoubleType *)aDes;
                CM_ENDIAN_ASSIGN8( sValue, aSrc );
                aColumn->mDataLength = 8;
                aColumn->mMTLength   = 8;
                aColumn->mPrecision  = 0;
            }
            break;

        case ULN_MTYPE_BLOB :
        case ULN_MTYPE_CLOB :
        case ULN_MTYPE_BLOB_LOCATOR :
        case ULN_MTYPE_CLOB_LOCATOR :

            /* PROJ-2728 Sharding LOB */
            /* 
             * PROJ-2047 Strengthening LOB - LOBCACHE
             *
             * LOB_LOCATOR(8) + lobsize(8) + HasData(1) + LobData(?)
             */
            CM_ENDIAN_ASSIGN8(&sLobLocatorId, aSrc);
            ACI_TEST( aDesLen < 8 );
            *(mtdClobLocatorType *)aDes = sLobLocatorId;

            CM_ENDIAN_ASSIGN8(&sLobSize, aSrc + 8);

            sLob = (ulnLob *)aColumn->mBuffer;
            ulnLobInitialize(sLob, (ulnMTypeID)aColumn->mMtype);

            sLob->mOp->mSetLocator(aFnContext, sLob, sLobLocatorId);

            /* PROJ-2728 Sharding LOB */
            if ( sLobSize == ACP_ULONG_MAX )
            {
                sLob->mIsNull = ACP_TRUE;
                sLob->mSize = 0;
            }
            else
            {
                sLob->mIsNull = ACP_FALSE;;
                sLob->mSize = sLobSize;
            }

            aColumn->mDataLength = ACI_SIZEOF(ulnLob);
            aColumn->mMTLength   = LOB_MT_SIZE;
            aColumn->mPrecision  = 0;

            if (sLobLocatorId == MTD_LOCATOR_NULL )
            {
                aColumn->mDataLength = SQL_NULL_DATA;
            }
            else
            {
                /* 
                 * PROJ-2047 Strengthening LOB - LOBCACHE
                 *
                 * HasData가 True이면 LOB Caching을 한다.
                 */
                if (aSrc[LOB_MT_HASDATA_OFFSET] == ACP_TRUE)
                {
                    sLob->mData = aSrc + LOB_MT_SIZE;
                    aColumn->mMTLength += sLob->mSize;
                }
                else
                {
                    sLob->mData = NULL;
                }

                /* BUG-36966 */
                ACI_TEST(ulnLobCacheAdd(sStmt->mLobCache,
                                        sLobLocatorId,
                                        sLob->mData,
                                        sLob->mSize,
                                        sLob->mIsNull)
                         != ACI_SUCCESS);
            }
            break;

        case ULN_MTYPE_TIMESTAMP :
            {
                mtdDateType *sData = NULL;
                ACI_TEST( aDesLen < 8 );
                sData = (mtdDateType *)aDes;
                CM_ENDIAN_ASSIGN2( &sData->year,
                                   aSrc );
                CM_ENDIAN_ASSIGN2( &sData->mon_day_hour,
                                   (aSrc + 2) );
                CM_ENDIAN_ASSIGN4( &sData->min_sec_mic,
                                   (aSrc + 4) );
                aColumn->mDataLength = 8;
                aColumn->mMTLength   = 8;
                aColumn->mPrecision  = 0;
            }
            break;

        case ULN_MTYPE_INTERVAL :
            {
                mtdIntervalType *sData = NULL;
                ACI_TEST( aDesLen < 16 );
                sData = (mtdIntervalType *)aDes;
                CM_ENDIAN_ASSIGN8( &sData->second     , aSrc );
                CM_ENDIAN_ASSIGN8( &sData->microsecond, (aSrc+8) );
                aColumn->mDataLength = 16;
                aColumn->mMTLength   = 16;
                aColumn->mPrecision  = 0;
            }
            break;

        case ULN_MTYPE_BIT :
        case ULN_MTYPE_VARBIT :
            {
                mtdBitType * sData = NULL;
                ACI_TEST( aDesLen < aMaxLen );
                sData = (mtdBitType*)aDes;
                CM_ENDIAN_ASSIGN4( &sData->length, aSrc );
                if ( sData->length != 0 )
                {
                    sStructSize = MTD_BIT_TYPE_STRUCT_SIZE( sData->length );
                }
                else
                {
                    sStructSize = ACI_SIZEOF( acp_uint32_t );
                }
                ACI_TEST( sStructSize > aMaxLen );
                acpMemCpy( &sData->value, (aSrc+4), (sStructSize-4) );
                aColumn->mDataLength = BIT_TO_BYTE( sData->length );
                aColumn->mMTLength   = aColumn->mDataLength + 4;
                aColumn->mPrecision  = sData->length;
                if ( aColumn->mDataLength == 0)
                {
                    aColumn->mDataLength = SQL_NULL_DATA;
                }
            }
            break;

        case ULN_MTYPE_NIBBLE :
            {
                mtdNibbleType *sData = NULL;
                ACI_TEST( aDesLen < aMaxLen );
                sData = (mtdNibbleType*)aDes;
                CM_ENDIAN_ASSIGN1( sData->length, *aSrc );
                if ( sData->length != MTD_NIBBLE_NULL_LENGTH )
                {
                    sStructSize = MTD_NIBBLE_TYPE_STRUCT_SIZE( sData->length );
                }
                else
                {
                    sStructSize = ACI_SIZEOF( acp_char_t );
                }
                ACI_TEST( sStructSize > aMaxLen );
                acpMemCpy( &sData->value, (aSrc + 1), (sStructSize - 1) );
                if ( sData->length == MTD_NIBBLE_NULL_LENGTH )
                {
                    aColumn->mDataLength = SQL_NULL_DATA;
                    aColumn->mMTLength   = 1;
                    aColumn->mPrecision  = 0;
                }
                else
                {
                    aColumn->mDataLength = (sData->length + 1) >> 1;
                    aColumn->mMTLength   = aColumn->mDataLength + 1;
                    aColumn->mPrecision  = sData->length;
                }
            }
            break;

        case ULN_MTYPE_BYTE :
        case ULN_MTYPE_VARBYTE :
            {
                mtdByteType *sData = NULL;
                ACI_TEST( aDesLen < aMaxLen );
                sData = (mtdByteType *)aDes;
                CM_ENDIAN_ASSIGN2( &sData->length, aSrc );
                sStructSize = MTD_BYTE_TYPE_STRUCT_SIZE( sData->length );
                ACI_TEST( sStructSize > aMaxLen );
                acpMemCpy( &sData->value, (aSrc + 2), (sStructSize - 2) );
                aColumn->mDataLength = sData->length;
                aColumn->mMTLength   = aColumn->mDataLength + 2;
                aColumn->mPrecision  = 0;
                if ( sData->length == 0)
                {
                    aColumn->mDataLength = SQL_NULL_DATA;
                }
            }
            break;

        default :
            ACE_ASSERT(0);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**************************************************************
 * ulsdCacheRowCopyToUserBufferShardCore
 *  - PROJ-2638
 *  - ulsdCacheRowCopyToUserBuffer에서 호출된다.
 *
 * aFnContext [IN] - ulnFnContext
 * aStmt      [IN] - ulnStmt
 * aSrc       [IN] - CM 버퍼
 * aColumn    [IN] - ulnColumn
 * aColNum    [IN] - 컬럼의 번호
 **************************************************************/
ACI_RC ulsdCacheRowCopyToUserBufferShardCore( ulnFnContext * aFnContext,
                                              ulnStmt      * aStmt,
                                              acp_uint8_t  * aSrc,
                                              ulnColumn    * aColumn,
                                              acp_uint32_t   aColNum)
{
    acp_uint32_t      sColumnOffSet   = 0;
    acp_uint8_t     * sDes;

    ACI_TEST( ( aColNum>aStmt->mShardStmtCxt.mColumnOffset.mColumnCnt ) || ( aColNum == 0 ) );
    sColumnOffSet = aStmt->mShardStmtCxt.mColumnOffset.mOffSet[ aColNum - 1 ];
    ACI_TEST( ( aStmt->mShardStmtCxt.mRowDataBufLen - sColumnOffSet) <= 0 );

    sDes = aStmt->mShardStmtCxt.mRowDataBuffer + sColumnOffSet;
    ACI_TEST( ulsdDataCopyFromMT( aFnContext,
                                  aSrc,
                                  sDes,
                                  aStmt->mShardStmtCxt.mRowDataBufLen - sColumnOffSet,
                                  aColumn,
                                  aStmt->mShardStmtCxt.mColumnOffset.mMaxByte[ aColNum - 1 ] )
              != ACI_SUCCESS );

    /* BUG-32474 */
    if ((aColumn->mMtype == ULN_MTYPE_BLOB)
     || (aColumn->mMtype == ULN_MTYPE_CLOB)
     || (aColumn->mMtype == ULN_MTYPE_BLOB_LOCATOR)
     || (aColumn->mMtype == ULN_MTYPE_CLOB_LOCATOR))
    {
        ACI_TEST( ulnCacheAddReadLobLocator(aStmt->mCache,
                                            (acp_uint64_t*)sDes)
                  != ACI_SUCCESS );
    }
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

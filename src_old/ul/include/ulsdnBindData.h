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
 * $Id: ulsdnBindData.h 00000 2017-02-27 10:26:00Z swhors $
 **********************************************************************/
#ifndef _O_ULSDN_BINDDATA_H_
#define _O_ULSDN_BINDDATA_H_ 1

#define ULSD_INPUT_RAW_MTYPE_NULL    30000
#define ULSD_INPUT_RAW_MTYPE_MAX     ULSD_INPUT_RAW_MTYPE_NULL + ULN_MTYPE_MAX

ACI_RC ulsdParamMTDDataCopyToStmt( ulnStmt     * aStmt,
                                   ulnDescRec  * aDescRecApd,
                                   ulnDescRec  * aDescRecIpd,
                                   acp_uint8_t * aSrcPtr );

ACI_RC ulsdParamProcess_DATAs_ShardCore( ulnFnContext * aFnContext,
                                         ulnStmt      * aStmt,
                                         ulnDescRec   * aDescRecApd,
                                         ulnDescRec   * aDescRecIpd,
                                         acp_uint32_t   aParamNumber,
                                         acp_uint32_t   aRowNumber,
                                         void         * aUserDataPtr );

/* 사용자 데이터 타입이 MT타입인지를 확인한다.*/
ACP_INLINE acp_bool_t ulsdIsInputMTData( acp_sint16_t aType )
{
    acp_bool_t sRet = ACP_FALSE;

    if ((aType >= ULSD_INPUT_RAW_MTYPE_NULL) &&
        (aType < ULSD_INPUT_RAW_MTYPE_MAX))
    {
        sRet = ACP_TRUE;
    }
    else
    {
        /* Do Nothing. */
    }
    return sRet;
}

/* PROJ-2638 shard native linker                      */
/* 사용자 컬럼의 데이터 MT타입이 Fixed_Precision인지를 확인한다. */
ACP_INLINE acp_bool_t ulsdIsFixedPrecision( acp_uint32_t aUlnMtType )
{
    acp_bool_t sRet = ACP_FALSE;
    switch (aUlnMtType)
    {
        case ULN_MTYPE_BIGINT:
        case ULN_MTYPE_SMALLINT:
        case ULN_MTYPE_INTEGER:
        case ULN_MTYPE_DOUBLE:
        case ULN_MTYPE_REAL:
        case ULN_MTYPE_DATE:
        case ULN_MTYPE_TIME:
        case ULN_MTYPE_TIMESTAMP:
            sRet = ACP_TRUE;
        default:
            break;
    }
    return sRet;
}

/***************************************************
 * PROJ-2638 shard native linker
 * SQL_C_DEFAULT 로 바인딩시 어떤 타입을 가정해야 하는지 결정하는 함수.
 ***************************************************/
ulnCTypeID ulsdTypeGetDefault_UserType( ulnMTypeID aMTYPE );

/***************************************************
 * PROJ-2638 shard native linker
 * 사용자 데이터의 MTYPE에 해당하는 CTYPE을 반환한다.
 * ulnBindParamApdPart과 ulnBindParamBody에서 호출 된다.
 *
 * ulnBindParamBody에서의 사용 예 :
 *  sMTYPE = aParamType - ULSD_INPUT_RAW_MTYPE_NULL;
 *  sCTYPE = ulsdTypeMap_MTYPE_CTYPE(sMTYPE);
 ***************************************************/
ulnCTypeID ulsdTypeMap_MTYPE_CTYPE( acp_sint16_t aMTYPE );

/* PROJ-2638 shard native linker */
/* 서버에서 반환된 데이터를 MT 타입 그대로 SDL에 넘기기 위해서
 * 사용 된다.
 * ulsdCacheRowCopyToUserBufferShardCore에서 호출 된다.
 */
ACI_RC ulsdDataCopyFromMT(ulnFnContext * aFnContext,
                          acp_uint8_t  * aSrc,
                          acp_uint8_t  * aDes,
                          acp_uint32_t   aDesLen,
                          ulnColumn    * aColumn,
                          acp_uint32_t   aMaxLen);

/* PROJ-2638 shard native linker */
/* 서버에서 반환된 데이터를 MT 타입 그대로 SDL에 넘기기 위해서
 * 사용 된다.
 * ulsdCacheRowCopyToUserBuffer에서 호출 된다.
 */
ACI_RC ulsdCacheRowCopyToUserBufferShardCore( ulnFnContext * aFnContext,
                                              ulnStmt      * aStmt,
                                              acp_uint8_t  * aSrc,
                                              ulnColumn    * aColumn,
                                              acp_uint32_t   aColNum);
#endif // _O_ULSDN_BINDDATA_H_

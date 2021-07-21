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

#ifndef _O_ULN_BINDINFO_H_
#define _O_ULN_BINDINFO_H_ 1

typedef struct ulnBindInfo ulnBindInfo;

#define ULN_PARAM_TYPE_MEM_BOUND_LOB 0x04
#define ULN_PARAM_TYPE_DATA_AT_EXEC  0x02
#define ULN_PARAM_TYPE_OUT_PARAM     0x01
#define ULN_PARAM_TYPE_MAX           ((ULN_PARAM_TYPE_MEM_BOUND_LOB |    \
                                       ULN_PARAM_TYPE_DATA_AT_EXEC  |    \
                                       ULN_PARAM_TYPE_OUT_PARAM) + 1)

#include <ulnBindParamDataIn.h>
/* must same order gUlnParamDataInBuildAnyFuncArray */
typedef enum ulnParamDataInFuncID
{
    ULN_BIND_PARAMDATA_IN_FUNC_OLD = 0,
    ULN_BIND_PARAMDATA_IN_FUNC_CHAR_CHAR,
    ULN_BIND_PARAMDATA_IN_FUNC_CHAR_SMALLINT,
    ULN_BIND_PARAMDATA_IN_FUNC_CHAR_INTEGER,
    ULN_BIND_PARAMDATA_IN_FUNC_CHAR_BIGINT,
    ULN_BIND_PARAMDATA_IN_FUNC_CHAR_NUMERIC,
    ULN_BIND_PARAMDATA_IN_FUNC_CHAR_BIT,
    ULN_BIND_PARAMDATA_IN_FUNC_CHAR_REAL,
    ULN_BIND_PARAMDATA_IN_FUNC_CHAR_FLOAT,
    ULN_BIND_PARAMDATA_IN_FUNC_CHAR_DOUBLE,
    ULN_BIND_PARAMDATA_IN_FUNC_CHAR_BINARY,
    ULN_BIND_PARAMDATA_IN_FUNC_CHAR_NIBBLE,
    ULN_BIND_PARAMDATA_IN_FUNC_CHAR_BYTE,
    ULN_BIND_PARAMDATA_IN_FUNC_CHAR_TIMESTAMP,
    ULN_BIND_PARAMDATA_IN_FUNC_CHAR_INTERVAL,
    ULN_BIND_PARAMDATA_IN_FUNC_CHAR_NCHAR,

    ULN_BIND_PARAMDATA_IN_FUNC_NUMERIC_NUMERIC,

    ULN_BIND_PARAMDATA_IN_FUNC_BIT_CHAR,
    ULN_BIND_PARAMDATA_IN_FUNC_BIT_NCHAR,
    ULN_BIND_PARAMDATA_IN_FUNC_BIT_NUMERIC,
    ULN_BIND_PARAMDATA_IN_FUNC_BIT_BIT,
    ULN_BIND_PARAMDATA_IN_FUNC_BIT_SMALLINT,
    ULN_BIND_PARAMDATA_IN_FUNC_BIT_INTEGER,
    ULN_BIND_PARAMDATA_IN_FUNC_BIT_BIGINT,
    ULN_BIND_PARAMDATA_IN_FUNC_BIT_REAL,
    ULN_BIND_PARAMDATA_IN_FUNC_BIT_DOUBLE,

    ULN_BIND_PARAMDATA_IN_FUNC_TO_SMALLINT,
    ULN_BIND_PARAMDATA_IN_FUNC_TO_INTEGER,
    ULN_BIND_PARAMDATA_IN_FUNC_TO_BIGINT,

    ULN_BIND_PARAMDATA_IN_FUNC_FLOAT_REAL,
    ULN_BIND_PARAMDATA_IN_FUNC_DOUBLE_DOUBLE,

    ULN_BIND_PARAMDATA_IN_FUNC_BINARY_CHAR,
    ULN_BIND_PARAMDATA_IN_FUNC_BINARY_NCHAR,
    ULN_BIND_PARAMDATA_IN_FUNC_BINARY_BINARY,
    ULN_BIND_PARAMDATA_IN_FUNC_BINARY_NUMERIC,
    ULN_BIND_PARAMDATA_IN_FUNC_BINARY_BIT,
    ULN_BIND_PARAMDATA_IN_FUNC_BINARY_NIBBLE,
    ULN_BIND_PARAMDATA_IN_FUNC_BINARY_BYTE,
    ULN_BIND_PARAMDATA_IN_FUNC_BINARY_SMALLINT,
    ULN_BIND_PARAMDATA_IN_FUNC_BINARY_INTEGER,
    ULN_BIND_PARAMDATA_IN_FUNC_BINARY_BIGINT,
    ULN_BIND_PARAMDATA_IN_FUNC_BINARY_DOUBLE,
    ULN_BIND_PARAMDATA_IN_FUNC_BINARY_REAL,
    ULN_BIND_PARAMDATA_IN_FUNC_BINARY_INTERVAL,
    ULN_BIND_PARAMDATA_IN_FUNC_BINARY_TIMESTAMP,
    ULN_BIND_PARAMDATA_IN_FUNC_BINARY_DATE,

    ULN_BIND_PARAMDATA_IN_FUNC_DATE_DATE,
    ULN_BIND_PARAMDATA_IN_FUNC_TIME_TIME,
    ULN_BIND_PARAMDATA_IN_FUNC_TIMESTAMP_DATE,
    ULN_BIND_PARAMDATA_IN_FUNC_TIMESTAMP_TIME,
    ULN_BIND_PARAMDATA_IN_FUNC_TIMESTAMP_TIMESTAMP,

    ULN_BIND_PARAMDATA_IN_FUNC_MAX

} ulnParamDataInFuncID;

typedef enum ulnPrecisionFuncID
{
    ULN_PREC_FUNC_0 = 0,
    ULN_PREC_FUNC_1,
    ULN_PREC_FUNC_COLSIZE,
    ULN_PREC_FUNC_COLSIZE2,
    ULN_PREC_FUNC_APD,
    ULN_PREC_FUNC_IPD,
    ULN_PREC_FUNC_BUFFERSIZE,
    ULN_PREC_FUNC_MAX
} ulnPrecisionFuncID;

typedef enum ulnScaleFuncID
{
    ULN_SCALE_FUNC_0 = 0,
    ULN_SCALE_FUNC_APD,
    ULN_SCALE_FUNC_IPD,
    ULN_SCALE_FUNC_MAX
} ulnScaleFuncID;

typedef acp_sint32_t ulnPrecisionFunc(ulnMeta          *aAppMeta,
                                      ulnMeta          *aImpMeta,
                                      void             *aUserBuffer,
                                      ulnIndLenPtrPair *aUserIndLenPair);

typedef acp_sint32_t ulnScaleFunc(ulnMeta          *aAppMeta,
                                  ulnMeta          *aImpMeta,
                                  void             *aUserBuffer,
                                  ulnIndLenPtrPair *aUserIndLenPair);

ulnScaleFunc     *ulnBindInfoGetScaleFunc(ulnCTypeID aCTYPE, ulnMTypeID aMTYPE);
ulnPrecisionFunc *ulnBindInfoGetPrecisionFunc(ulnCTypeID aCTYPE, ulnMTypeID aMTYPE);

typedef enum ulnParamInOutType
{
    ULN_PARAM_INOUT_TYPE_INIT = 0,
    ULN_PARAM_INOUT_TYPE_INPUT,
    ULN_PARAM_INOUT_TYPE_OUTPUT,
    ULN_PARAM_INOUT_TYPE_IN_OUT,
    ULN_PARAM_INOUT_TYPE_MAX
} ulnParamInOutType;

struct ulnBindInfo
{
    ulnMTypeID                      mMTYPE;
    acp_sint32_t                    mPrecision;
    acp_sint32_t                    mScale;
    acp_uint32_t                    mLanguage;
    ulnParamInOutType               mInOutType;
    acp_uint8_t                     mArguments;

    ulnParamDataInBuildAnyFunc *mParamDataInBuildAnyFunc;
};

/*
 * ulnBindInfoMap 에서 서버로 전송할 MTYPE 을 찾아내는 함수
 */
ulnMTypeID ulnBindInfoGetMTYPEtoSet(ulnCTypeID aCTYPE, ulnMTypeID aMTYPE);

/*
 * ulnBindInfo 구조체 초기화 함수.
 */
void ulnBindInfoInitialize(ulnBindInfo *aBindInfo);

/*
 * ulnBindInfo 의 멤버들을 set/get 하는 함수들
 */

void       ulnBindInfoSetType(ulnBindInfo *aBindInfo, ulnMTypeID aMTYPE);
ulnMTypeID ulnBindInfoGetType(ulnBindInfo *aBindInfo);

void              ulnBindInfoSetInOutType(ulnBindInfo *aBindInfo, ulnParamInOutType aInOutType);
ulnParamInOutType ulnBindInfoGetInOutType(ulnBindInfo *aBindInfo);

void         ulnBindInfoSetLanguage(ulnBindInfo *aBindInfo, acp_uint32_t aLanguage);
acp_uint32_t ulnBindInfoGetLanguage(ulnBindInfo *aBindInfo);

void        ulnBindInfoSetArguments(ulnBindInfo *aBindInfo, acp_uint8_t aArguments);
acp_uint8_t ulnBindInfoGetArguments(ulnBindInfo *aBindInfo);

void         ulnBindInfoSetPrecision(ulnBindInfo *aBindInfo, acp_sint32_t aPrecision);
acp_sint32_t ulnBindInfoGetPrecision(ulnBindInfo *aBindInfo);

void         ulnBindInfoSetScale(ulnBindInfo *aBindInfo, acp_sint32_t aScale);
acp_sint32_t ulnBindInfoGetScale(ulnBindInfo *aBindInfo);

acp_uint8_t  ulnBindInfoCmParamInOutType( ulnBindInfo *aBindInfo );

ulnParamInOutType ulnBindInfoConvUlnParamInOutType(acp_uint8_t aCmParamInOutType);

/*
 * ========================
 */
ACI_RC ulnCallbackParamInfoGetResult(cmiProtocolContext *aProtocolContext,
                                     cmiProtocol        *aProtocol,
                                     void               *aServiceSession,
                                     void               *aUserContext);

ACI_RC ulnCallbackColumnInfoGetResult(cmiProtocolContext *aProtocolContext,
                                      cmiProtocol        *aProtocol,
                                      void               *aServiceSession,
                                      void               *aUserContext);

ACI_RC ulnCallbackColumnInfoGetListResult(cmiProtocolContext *aProtocolContext,
                                          cmiProtocol        *aProtocol,
                                          void               *aServiceSession,
                                          void               *aUserContext);

ACI_RC ulnCallbackParamInfoSetListResult(cmiProtocolContext *aProtocolContext,
                                         cmiProtocol        *aProtocol,
                                         void               *aServiceSession,
                                         void               *aUserContext);

ACI_RC ulnBindInfoBuild4Param(ulnFnContext      *aFnContext,
                              acp_uint32_t       aRowNumber,
                              ulnDescRec        *aDescRecApd,
                              ulnDescRec        *aDescRecIpd,
                              ulnParamInOutType  aInOutType,
                              acp_bool_t        *aIsChanged);

ACI_RC ulnParamProcess_INFOs(ulnFnContext      *aFnContext,
                             ulnPtContext      *aPtContext,
                             acp_uint32_t       aRowNumber ); /* 0 베이스 */

ACI_RC ulnParamProcess_DATAs(ulnFnContext      *aFnContext,
                             ulnPtContext      *aPtContext,
                             acp_uint32_t       aRowNumber ); /* 0 베이스 */

ACI_RC ulnParamProcess_IPCDA_DATAs(ulnFnContext *aFnContext,
                                   ulnPtContext *aPtContext,
                                   acp_uint32_t  aRowNumber,
                                   acp_uint64_t *aDataSize ); /* 0 베이스 */

#endif /* _O_ULN_BINDINFO_H_ */

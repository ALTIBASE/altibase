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
 * $Id: aciErrorMgr.h 11319 2010-06-23 02:39:42Z djin $
 **********************************************************************/

#ifndef _O_ACIERRORMGR_H_
#define _O_ACIERRORMGR_H_ 1

#include <acp.h>


#include <aciTypes.h>
#include <aciVa.h>

ACP_EXTERN_C_BEGIN

/* ------------------------------------------------------------
 *                  에러 코드의 구성
 *
 *      [MODULE]ERR_[ACTION]_[NamingSpace]_[Description]
 *          1           2          3            4
 *
 *  [MODULE] :  sm | qp | cm
 *  [ACTION] :  FATAL | ABORT | IGNORE
 *  [NamingSpace] : qex | qpf | smd | smc ...
 *  [Description] : FileWriteError | ....
 *
 *  Ex) idERR_FATAL_smd_InvalidOidAddress
 * -----------------------------------------------------------*/

#define ACI_E_MODULE_MASK    0xF0000000   /* CM, QP, SM */
#define ACI_E_ACTION_MASK    0x0F000000   /* FATAL, ABORT, IGNORE */

#define ACI_E_ERRORCODE_MASK 0xFFFFF000   /* real error code mask */
#define ACI_E_SUBCODE_MASK   0x00FFF000   /* subcode mask */
#define ACI_E_INDEX_MASK     0x00000FFF   /* internal index area */

#define ACI_E_ERROR_CODE(a)  ( ((a) & ACI_E_ERRORCODE_MASK) >> 12) /* get real error code */

/* PROJ-1335 user-defined error */
/* real error code -> server error code */
#define ACI_E_SERVER_ERROR(a)   ( ( (a) << 12 ) & E_ERRORCODE_MASK )

/* PROJ-1477 recover original error code for client error manager */
#define ACI_E_RECOVER_ERROR(a)   ( ( (a) << 12 ) )

/* check errorcode is user-defined ( 990000 ~ 991000 ) */
#define ACI_E_CHECK_UDE(a)      ( ( ( (a) >= 0x000F1B30 ) && ( (a) <= 0x000F1F18 ) ) \
                              ? ACP_TRUE : ACP_FALSE )

#define ACI_E_MESSAGE_MASK   0x00FFFFFF   /* real Error message  useless... */

#define ACI_E_MODULE_COUNT   16           /* 모듈 갯수 */
#define ACI_E_MODULE_ID      0x00000000
#define ACI_E_MODULE_SM      0x10000000
#define ACI_E_MODULE_MT      0x20000000
#define ACI_E_MODULE_QP      0x30000000
#define ACI_E_MODULE_MM      0x40000000
#define ACI_E_MODULE_RP      0x60000000
#define ACI_E_MODULE_CM      0x70000000
#define ACI_E_MODULE_SD      0xE0000000

/* PROJ-1335 user-defined error */
#define ACI_E_MODULE_USER    0xF0000000   /* user-defined error */

#define ACI_E_ACTION_FATAL   0x00000000
#define ACI_E_ACTION_ABORT   0x01000000
#define ACI_E_ACTION_IGNORE  0x02000000
#define ACI_E_ACTION_RETRY   0x03000000
#define ACI_E_ACTION_REBUILD 0x04000000

#define ACI_MAX_ERROR_ACTION  5

/* ----------------------------------------------------------------------
 *
 *  에러 가변인자 처리를 위한 데이타 구조
 *
 * ----------------------------------------------------------------------*/

#define ACI_MAX_ARGUMENT  32  /* 에러 메시지 생성시 최대 가변인자 갯수 */

typedef enum
{
    ACI_ERR_NONE  = 0,
    ACI_ERR_SCHAR,
    ACI_ERR_STRING,
    ACI_ERR_STRING_N,
    ACI_ERR_SINT,
    ACI_ERR_UINT,
    ACI_ERR_SLONG,
    ACI_ERR_ULONG,
    ACI_ERR_VSLONG,
    ACI_ERR_VULONG,
    ACI_ERR_HEX32,
    ACI_ERR_HEX64,
    ACI_ERR_HEX_V
} ACI_ERR_DATATYPE;

typedef struct aci_err_type_info_t
{
    ACI_ERR_DATATYPE    type;
    const acp_char_t       * tmpSpecStr; /* 에러 메시지에서 쓰임 */
    const acp_char_t       * realSpecStr; /* 실제 printf에서 쓰임 */
    acp_char_t               len;
} aci_err_type_info_t;

typedef struct aci_arg_info_t
{
    aci_err_type_info_t *type_info;
    struct aci_arg_info_t         *outputOrder; /* 출력할 때의 인자 가리킴 */
    union
    {
        acp_char_t *string;
        acp_sint8_t  schar;
        acp_uint8_t  uchar;
        acp_sint32_t   sint;
        acp_uint32_t   uint;
        acp_sint64_t  slong;
        acp_uint64_t  ulong;
        acp_slong_t vslong;
        acp_ulong_t vulong;
        acp_uint32_t   hex32;
        acp_uint64_t  hex64;
        acp_ulong_t hexv;
    } data;
    acp_sint32_t stringLen;
} aci_arg_info_t;


/* ----------------------------------------------------------------------
 *
 *  SM 에러 코드 데이타 구조 및 전역변수
 *
 * ----------------------------------------------------------------------*/

#define ACI_MAX_ACI_ERROR_STACK  50
#define ACI_MAX_ERROR_FILE_LEN   256
#define ACI_MAX_ERROR_TEST_LEN   1024
/*
  NOTICE!!
  Becuase stored procedure stored error message to a varchar variable,
  MAX_ERROR_MSGLEN should not be larger than varchar size (4096)
*/
#define ACI_MAX_ERROR_MSG_LEN    2048
#define ACI_MAX_FORMAT_ITEM_LEN   128

typedef struct aci_error_mgr_t
{
    acp_uint32_t  LastError;
    acp_sint32_t  LastSystemErrno;
    acp_char_t LastErrorMsg[ACI_MAX_ERROR_MSG_LEN+256];
    acp_char_t LastErrorFile[ACI_MAX_ERROR_FILE_LEN];
    acp_uint32_t  LastErrorLine;

/*#ifdef DEBUG BUG-29630 */
    acp_uint32_t  ErrorIndex;
    acp_uint32_t  ErrorTested;
    acp_char_t ErrorFile[ACI_MAX_ACI_ERROR_STACK][ACI_MAX_ERROR_FILE_LEN];
    acp_uint32_t  ErrorLine[ACI_MAX_ACI_ERROR_STACK];
    acp_char_t TestLine[ACI_MAX_ACI_ERROR_STACK][ACI_MAX_ERROR_TEST_LEN];
/* #endif */
} aci_error_mgr_t;

typedef struct aci_msb_header_t
{
    acp_uint32_t  AltiVersionId; /* MSB파일에 알티베이스 버전 기록 (2003/10/31, by kumdory) */
    acp_uint32_t  Section;    /* 에러 구분 번호 */
    acp_uint64_t ErrorCount; /* 에러 메시지 갯수, 플랫폼간 상이한 align문제때문에 ErrorCount를 Section뒤로 옮김 */
} aci_msb_header_t;

/* 설명 : genErrMsg에 의해 생성된 *.msb 화일의 헤더 구조 */
typedef struct aci_error_msb_type_t
{
    union
    {
        acp_uint8_t dummy[256];
        aci_msb_header_t header;
    } value_;

} aci_error_msb_type_t;

/* formatted string 저장 */
typedef struct aci_error_factory_t
{
    aci_error_msb_type_t MsbHeader;
    acp_char_t **MsgBuf;
} aci_error_factory_t;


/* ----------------------------------------------------------------------
 *  ID 에러 코드 Get, Set
 * ----------------------------------------------------------------------*/
/* 에러 정보 검색 */
ACP_EXPORT struct  aci_error_mgr_t* aciGetErrorMgr(); /* 에러 구조체 얻기 */

ACP_EXPORT acp_uint32_t   aciGetErrorCode();
ACP_EXPORT acp_uint32_t   aciGetErrorLine();
ACP_EXPORT acp_char_t *aciGetErrorFile();
ACP_EXPORT acp_sint32_t   aciGetSystemErrno();
ACP_EXPORT acp_sint32_t   aciFindErrorCode(acp_uint32_t ErrorCode);
ACP_EXPORT acp_sint32_t   aciFindErrorAction(acp_uint32_t ErrorAction);
ACP_EXPORT acp_sint32_t   aciSetDebugInfo(acp_char_t *File, acp_uint32_t   Line, acp_char_t *testline);

ACP_EXPORT acp_sint32_t   aciIsAbort();
ACP_EXPORT acp_sint32_t   aciIsFatal();
ACP_EXPORT acp_sint32_t   aciIsIgnore();
ACP_EXPORT acp_sint32_t   aciIsRetry();
ACP_EXPORT acp_sint32_t   aciIsRebuild();

/* 에러 코드 설정 */
ACP_EXPORT aci_error_mgr_t* aciSetErrorCode(acp_uint32_t ErrorCode, ...);

/* PROJ-1335 errorcode, errmsg 직접 설정 */
ACP_EXPORT aci_error_mgr_t* aciSetErrorCodeAndMsg( acp_uint32_t   ErrorCode,
                                    acp_char_t* ErrorMsg );

ACP_EXPORT void aciClearError(); /* 인덱스 초기화 */
ACP_EXPORT acp_sint32_t aciPopErrorCode();
ACP_EXPORT void aciDump();

/* ------------------------------------------------------------------
 * 에러 메시지 처리 (MSB)
 * ----------------------------------------------------------------*/

ACP_EXPORT acp_sint32_t aciRegistErrorMsb(acp_char_t *fn);
ACP_EXPORT acp_sint32_t aciRegistErrorFromBuffer( acp_uint64_t aSection, 
                                                  acp_uint32_t aVersionID,
                                                  acp_uint64_t aErrorCount,
                                                  acp_char_t **aErrorBuffer );
ACP_EXPORT acp_char_t  *aciGetErrorMsg(acp_uint32_t ErrorCode);

/* BUG-15166 */
ACP_EXPORT acp_uint32_t   aciGetErrorArgCount(acp_uint32_t ErrorCode);

/* ----------------------------------------------------------------------
 *
 *  ACI 에러 Handling 매크로
 *
 * ----------------------------------------------------------------------*/

#ifdef DEBUG
#define ACI_CLEAR() aciClearError()
#else
#define ACI_CLEAR()
#endif /* DEBUG */


#ifdef DEBUG
# define ACI_RAISE( b ) {                                                           \
    aci_error_mgr_t* error = aciGetErrorMgr();				                            \
    if( error->ErrorIndex < ACI_MAX_ACI_ERROR_STACK ){				                    \
        acpSnprintf( error->TestLine [error->ErrorIndex], ACI_MAX_ERROR_TEST_LEN, "ACI_RAISE( %s );", #b ); \
        acpSnprintf( error->ErrorFile[error->ErrorIndex], ACI_MAX_ERROR_FILE_LEN, "%s", aciVaBasename(__FILE__) ); \
        error->ErrorLine[error->ErrorIndex] = __LINE__;                     \
        error->ErrorTested++;                                               \
    }                                                                       \
    goto b;                                                                 \
}
#else
#define ACI_RAISE(b)    goto b;
#endif

#ifdef DEBUG
# define ACI_TEST_RAISE( a, b ) {                                                                   \
    if( a ){                                                                                        \
	aci_error_mgr_t* error = aciGetErrorMgr();                                                          \
	if( error->ErrorIndex < ACI_MAX_ACI_ERROR_STACK ){                                                  \
	    acpSnprintf( error->TestLine [error->ErrorIndex], ACI_MAX_ERROR_TEST_LEN, "ACI_TEST_RAISE( %s, %s );", #a, #b ); \
	    acpSnprintf( error->ErrorFile[error->ErrorIndex], ACI_MAX_ERROR_FILE_LEN, "%s", aciVaBasename(__FILE__) ); \
	    error->ErrorLine[error->ErrorIndex] = __LINE__;				                                \
	    error->ErrorTested++;	                    						                        \
	}                                                                                               \
	goto b;                                                                                         \
    }                                                                                               \
}
#else
# define ACI_TEST_RAISE( a, b ) { \
    if( a ){                      \
        goto b;                   \
    }                             \
    else {}                       \
}
#endif

#ifdef DEBUG
#define ACI_TEST( a )                                    \
    if( a ){                                                     \
	aci_error_mgr_t* error = aciGetErrorMgr();                   \
	if( error->ErrorIndex < ACI_MAX_ACI_ERROR_STACK ){           \
	    acpSnprintf( error->TestLine [error->ErrorIndex], ACI_MAX_ERROR_TEST_LEN, \
                                "ACI_TEST( %s );", #a ); \
	    acpSnprintf( error->ErrorFile[error->ErrorIndex], ACI_MAX_ERROR_FILE_LEN, \
                                "%s", aciVaBasename(__FILE__) );		 \
	    error->ErrorLine[error->ErrorIndex] = __LINE__;	 \
	    error->ErrorTested++;	                    	 \
	}                                                        \
	goto ACI_EXCEPTION_END_LABEL;                            \
    }
#else
#define ACI_TEST( a )         \
    if( a ){                          \
        goto ACI_EXCEPTION_END_LABEL; \
    }
#endif

#define ACI_EXCEPTION(a) goto ACI_EXCEPTION_END_LABEL; a:

#define ACI_EXCEPTION_CONT(a) a:

#ifdef DEBUG
# define ACI_EXCEPTION_END ACI_EXCEPTION_END_LABEL: { \
    aci_error_mgr_t* error = aciGetErrorMgr();            \
    if( error->ErrorTested ){                         \
	  error->ErrorIndex++;                            \
	  error->ErrorTested = 0;                         \
    }                                                 \
}
#else
# define ACI_EXCEPTION_END       ACI_EXCEPTION_END_LABEL:;
#endif

#ifdef DEBUG

#define ACI_SET(setFunc) \
{\
     aci_error_mgr_t* sErrMgr = setFunc; \
     sErrMgr->LastErrorLine = __LINE__; \
     (void)acpCStrCpy(sErrMgr->LastErrorFile, ACI_MAX_ERROR_FILE_LEN, aciVaBasename(__FILE__), ACI_MAX_ERROR_FILE_LEN); \
     sErrMgr->LastErrorFile[ACI_MAX_ERROR_FILE_LEN - 1] = 0; \
     (void)aciSetDebugInfo((acp_char_t *)aciVaBasename(__FILE__), __LINE__, (acp_char_t *)#setFunc); \
}

#else

#define ACI_SET(setFunc) \
{\
     aci_error_mgr_t* sErrMgr = setFunc; \
     sErrMgr->LastErrorLine = __LINE__; \
     (void)acpCStrCpy(sErrMgr->LastErrorFile, ACI_MAX_ERROR_FILE_LEN, aciVaBasename(__FILE__), ACI_MAX_ERROR_FILE_LEN); \
     sErrMgr->LastErrorFile[ACI_MAX_ERROR_FILE_LEN - 1] = 0; \
}

#endif

#define ACI_TEST_CONT( a, b ) { if( a ) { goto b; } }
#define ACI_CONT( b ) { goto b;}
#define ACI_EXCEPTION_CONT(a) a:

/* ------------------------------------------------------------------
 *          에러 코드 Conversion Matrix
 *       (에러코드가 변경되었을 경우 동작)
 *       (X => 변경필요 없음 O=> 변경요망)
 *   --------------+-------------------------------------------------------------
 *                 |                         NEW CODE
 *     OLD CODE    +-------------------------------------------------------------
 *                 |   IGNORE  |   RETRY   | REBUILD   |   ABORT   |    FATAL
 *   --------------+-----------+-----------+-----------+-----------+-------------
 *     IGNORE      |     X     |     O     |     O     |     O     |      O
 *   --------------+-----------+-----------+-----------+-----------+-------------
 *     RETRY       |     X     |     X     |     O     |     O     |      O
 *   --------------+-----------+-----------+-----------+-----------+-------------
 *     REBUILD     |     X     |     X     |     X     |     O     |      O
 *   --------------+-----------+-----------+-----------+-----------+-------------
 *     ABORT       |     X     |     X     |     X     |     X     |      O
 *   --------------+-----------+-----------+-----------+-----------+-------------
 *     FATAL       |     X     |     X     |     X     |     X     |      X
 *   --------------+-----------+-----------+-----------+-----------+-------------
 *
 *   ACI_PUSH() 와 ACI_POP()은 pair로 사용해야 함.
 * ----------------------------------------------*/

extern const acp_bool_t aciErrorConversionMatrix[ACI_MAX_ERROR_ACTION][ACI_MAX_ERROR_ACTION];

/* ------------------------------------------------
 *  Client Part Error Managing
 * ----------------------------------------------*/

typedef struct aci_client_error_factory_t
{
    const acp_char_t *mErrorState;
    const acp_char_t *mErrorMsg;
}aci_client_error_factory_t;

typedef struct aci_client_error_mgr_t
{
    acp_uint32_t   mErrorCode;     /* CODE  */
    acp_char_t  mErrorState[6]; /* STATE */
    acp_char_t  mErrorMessage[ACI_MAX_ERROR_MSG_LEN+256];
} aci_client_error_mgr_t;

ACP_EXPORT void aciSetClientErrorCode(aci_client_error_mgr_t     *aErrorMgr,
                                      aci_client_error_factory_t *aFactory,
                                      acp_uint32_t                   aErrorCode,
                                      va_list                aArgs);

#if defined(ITRON) || defined(WIN32_ODBC_CLI2)
extern struct aci_error_mgr_t *toppers_error;
#endif

ACP_EXTERN_C_END

#endif /* _O_ACIERRORMGR_H_ */


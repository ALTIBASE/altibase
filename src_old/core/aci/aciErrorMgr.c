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
 * $Id: aciErrorMgr.c 11319 2010-06-23 02:39:42Z djin $
 **********************************************************************/

/***********************************************************************
 *	File Name 		:	ideErrorMgr.cpp
 *
 *	Author			:	Sung-Jin, Kim
 *
 *	Related Files	:
 *
 *	Description		:	에러 처리 모듈
 *
 *
 **********************************************************************/

/* ----------------------------------------------------------------------------
 * 성능 고려로 인해 class 대신 struct 및 plain function 으로 구현
 * ---------------------------------------------------------------------------*/

#include <aciErrorMgr.h>
#include <aciMsgLog.h>
#include <aciVa.h>
#include <aciProperty.h>
#include <aciCallback.h>
#include <aciVersion.h>
#include <aciMsg.h>

#define aciERR_IGNORE_NoError 0x42000000

/* ----------------------------------------------------------------------------
 *
 *  PDL 에러 수정
 *
 *  Sparc Solaris 2.7에서 64비트 컴파일 모드일 경우
 *  전역 시스템 에러코드번호 변수인 sys_nerr이 없어진다.
 *  그러나, PDL에서는 이것을 고려하지 않고 사용하기 때문에
 *  링크에러를 발생한다. 이것을 방지하기 위해 임시로 sys_nerr을 정의한다.
 *
 * --------------------------------------------------------------------------*/

aci_err_type_info_t gAciTypeInfo[] = /* 에러 메시지 화일에 사용되는 데이타 타입 종류 */
{
    { ACI_ERR_SCHAR   , "%c"  , "%c"              , 2 },
    { ACI_ERR_STRING  , "%s"  , "%s"              , 2 },
    { ACI_ERR_STRING_N, "%.*s", "%.*s"            , 4 }, /* BUG-45567 */
    { ACI_ERR_SINT    , "%d"  , "%d"              , 2 },
    { ACI_ERR_UINT    , "%u"  , "%d"              , 2 },
    { ACI_ERR_SLONG   , "%ld" , "%"ACI_INT64_FMT  , 3 },
    { ACI_ERR_ULONG   , "%lu" , "%"ACI_UINT64_FMT , 3 },
    { ACI_ERR_VSLONG  , "%vd" , "%"ACI_vSLONG_FMT , 3 },
    { ACI_ERR_VULONG  , "%vu" , "%"ACI_vULONG_FMT , 3 },
    { ACI_ERR_HEX32   , "%x"  , "%"ACI_xINT32_FMT , 2 },
    { ACI_ERR_HEX64   , "%lx" , "%"ACI_xINT64_FMT , 3 },
    { ACI_ERR_HEX_V   , "%vx" , "%"ACI_vxULONG_FMT, 3 },
    { ACI_ERR_NONE    , ""    , ""                , 0 }
};

/* ----------------------------------------------------------------------
 *
 *  Error Code Conversion Matrix
 *
 * ---------------------------------------------------------------------- */

const acp_bool_t aciErrorConversionMatrix[ACI_MAX_ERROR_ACTION][ACI_MAX_ERROR_ACTION] = /* [OLD][NEW] */
{
    /*              OLD : FATAL */
    /*  NEW : FATAL     ABORT     IGNORE    RETRY    REBUILD */
    {        ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE },
    /*             OLD : ABORT */
    /*  NEW : FATAL     ABORT     IGNORE    RETRY    REBUILD */
    {        ACP_TRUE,  ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE },
    /*             OLD : IGNORE */
    /*  NEW : FATAL     ABORT     IGNORE    RETRY    REBUILD */
    {        ACP_TRUE,  ACP_TRUE,  ACP_FALSE, ACP_TRUE,  ACP_TRUE  },
    /*             OLD : RETRY */
    /*  NEW : FATAL     ABORT     IGNORE    RETRY    REBUILD */
    {        ACP_TRUE,  ACP_TRUE,  ACP_FALSE, ACP_FALSE, ACP_TRUE  },
    /*             OLD : REBUILD */
    /*  NEW : FATAL     ABORT     IGNORE    RETRY    REBUILD */
    {        ACP_TRUE,  ACP_TRUE,  ACP_FALSE, ACP_FALSE, ACP_FALSE }
};

/* ----------------------------------------------------------------------
 *
 *  MT 클라이언트를 위한 컴파일 - shore storage manager 참조
 *
 * ---------------------------------------------------------------------- */

/*  fix BUG-17287 */

ACP_EXPORT
void  aciClearTSDError(aci_error_mgr_t* aIdErr)
{
    assert(aIdErr != NULL);
    aIdErr->LastError       = aciERR_IGNORE_NoError;
    aIdErr->LastSystemErrno = 0;
    acpMemSet(aIdErr->LastErrorMsg, 0, ACI_MAX_ERROR_MSG_LEN);
/* #ifdef DEBUG     *//* BUG-26930 */
    aIdErr->ErrorTested = 0;
    aIdErr->ErrorIndex  = 0;
/* #endif     */
    
}

/* ----------------------------------------------------------------------
 *
 *  ID 에러 코드값 얻기 : SERVER, CLIENT 공통 함수
 *
 * ----------------------------------------------------------------------*/

ACP_EXPORT
acp_uint32_t aciGetErrorCode()
{
    aci_error_mgr_t *iderr = aciGetErrorMgr();
    return iderr->LastError;
}

ACP_EXPORT
acp_sint32_t aciGetSystemErrno()
{
    aci_error_mgr_t *iderr = aciGetErrorMgr();
    return iderr->LastSystemErrno;
}

ACP_EXPORT
acp_uint32_t   aciGetErrorLine()
{
    aci_error_mgr_t *iderr = aciGetErrorMgr();
    return iderr->LastErrorLine;
}

ACP_EXPORT
acp_char_t *aciGetErrorFile()
{
    aci_error_mgr_t *iderr = aciGetErrorMgr();
    return iderr->LastErrorFile;
}

ACP_EXPORT
acp_sint32_t aciFindErrorCode(acp_uint32_t ErrorCode)
{
    aci_error_mgr_t *iderr = aciGetErrorMgr();

    if (iderr->LastError == ErrorCode)
    {
        return ACI_SUCCESS;
    }
    return ACI_FAILURE;
}

ACP_EXPORT
acp_sint32_t aciFindErrorAction(acp_uint32_t Action)
{
    aci_error_mgr_t *iderr = aciGetErrorMgr();

    if ((iderr->LastError & ACI_E_ACTION_MASK) == Action)
    {
        return ACI_SUCCESS;
    }
    return ACI_FAILURE;
}

ACP_EXPORT
acp_sint32_t   aciIsAbort()
{
    return aciFindErrorAction(ACI_E_ACTION_ABORT);
}

ACP_EXPORT
acp_sint32_t   aciIsFatal()
{
    return aciFindErrorAction(ACI_E_ACTION_FATAL);
}

ACP_EXPORT
acp_sint32_t   aciIsIgnore()
{
    return aciFindErrorAction(ACI_E_ACTION_IGNORE);
}

ACP_EXPORT
acp_sint32_t   aciIsRetry()
{
    return aciFindErrorAction(ACI_E_ACTION_RETRY);
}

ACP_EXPORT
acp_sint32_t   aciIsRebuild()
{
    return aciFindErrorAction(ACI_E_ACTION_REBUILD);
}


/* ----------------------------------------------------------------------
 *
 *  에러 코드값 설정 + 메시지 구성
 *
 * ----------------------------------------------------------------------*/

ACP_EXPORT
void aciClearError()
{
    aci_error_mgr_t *iderr = aciGetErrorMgr();

    ACE_ASSERT(iderr != NULL);

    iderr->LastError       = aciERR_IGNORE_NoError;
    iderr->LastSystemErrno = 0;
    acpMemSet(iderr->LastErrorMsg, 0, ACI_MAX_ERROR_MSG_LEN);

/* #ifdef DEBUG *//* BUG-26930 */
    iderr->ErrorTested = 0;
    iderr->ErrorIndex  = 0;
/* #endif */
}

/* #ifdef DEBUG *//* BUG-26930 */
ACP_EXPORT
acp_sint32_t aciSetDebugInfo(acp_char_t *File,     /* 에러 화일 */
                     acp_uint32_t   Line,     /* 에러 라인 */
                     acp_char_t *testline) /* 에러정보 */
{
    aci_error_mgr_t* error = aciGetErrorMgr();
    if( error->ErrorTested)
    {
        error->ErrorIndex++;
        error->ErrorTested = 0;
    }
    if( error->ErrorIndex < ACI_MAX_ACI_ERROR_STACK )
    {
        acpSnprintf( error->TestLine [error->ErrorIndex], ACI_MAX_ERROR_TEST_LEN, "ideSetErrorCode( %s )", testline );
        acpSnprintf( error->ErrorFile[error->ErrorIndex], ACI_MAX_ERROR_FILE_LEN, "%s", File );
        error->ErrorLine[error->ErrorIndex] = Line;
        error->ErrorIndex++;
    }
    return ACI_SUCCESS;
}
/* #endif */

/* #if defined(SERVER_COMPILE) */

/* ----------------------------------------------------------------------
 *
 *  서버를 위한 ID 에러 메시지 로딩 및 얻기
 *
 * ----------------------------------------------------------------------*/

static aci_error_factory_t aciErrorStorage[ACI_E_MODULE_COUNT];

/* formatted stirng을 읽어서 가변인자 정보를 구성 */
static acp_sint32_t aciGetArgumentInfo(acp_char_t *orgFmt, aci_arg_info_t *orgInfo, va_list args)
{
    acp_uint32_t  maxInfoCnt = 0;
    acp_uint32_t  outputOrder = 0;
    acp_uint32_t  inputOrder;
    acp_char_t c;
    acp_sint32_t  i;
    aci_arg_info_t *info = NULL;
    acp_char_t      *fmt = NULL;
    acp_rc_t sRC;

    fmt = orgFmt;
    orgInfo[0].type_info = NULL; /* 초기화 */

    while(( c = *fmt++) )
    {
        if (c == '<') /* [<] 출현 */
        {
            acp_char_t numBuf[8]; /* 인자번호 입력 */
            acp_sint32_t sign;

            /* ------------------
             * [1] 인자번호 얻기
             * -----------------*/
            if (acpCharIsDigit(*fmt) == ACP_FALSE) /* 숫자가 아님 */
            {
                continue;
            }

            for (i = 0; i < 8; i++)
            {
                if (*fmt  == '%')
                {
                    numBuf[i] = 0;
                    break;
                }
                numBuf[i] = *fmt++;
            }
            /* 몇번째 입력 인자인가? 포인터 대입 */
            sRC = acpCStrToInt32(numBuf, 8, &sign, &inputOrder, 10, NULL);

            if (ACP_RC_NOT_SUCCESS(sRC)
                || inputOrder >= ACI_MAX_ARGUMENT
                || sign < 0) /* added by gurugio */
            {
                 return ACI_FAILURE;
            }
            
            info              = &orgInfo[inputOrder];

            orgInfo[outputOrder++].outputOrder = info;

            if (inputOrder > maxInfoCnt) maxInfoCnt = inputOrder;

            /* ------------------
             * [2] 인자타입 얻기
             * -----------------*/
            for (i = 0; ; i++)
            {
                if (gAciTypeInfo[i].type == ACI_ERR_NONE)
                {
                    return ACI_FAILURE;
                }
                if (acpCStrCmp(fmt,
                               gAciTypeInfo[i].tmpSpecStr,
                               gAciTypeInfo[i].len) == 0)
                {
                    info->type_info = &gAciTypeInfo[i];
                    fmt += gAciTypeInfo[i].len;
                    break;
                }
            }
            if ( *fmt != '>')
            {
                return ACI_FAILURE;
            }
            fmt++;
        }
    }
    if (maxInfoCnt >= ACI_MAX_ARGUMENT - 1)
    {
        return ACI_FAILURE;
    }
    orgInfo[maxInfoCnt + 1].type_info = NULL; /* NULL을 지정 ; 마지막 flag */

    /* ------------------
     * [3] 인자 포인터 대입
     *     argument *를 다시 저장.
     * -----------------*/

    for (i = 0; ; i++) /* BUGBUG : sizeof(data types)로 해서 미리 계산할 수 있음. */
    {
        aci_err_type_info_t *typeinfo = orgInfo[i].type_info;
        if (typeinfo == NULL) break;
        switch(typeinfo->type)
        {
        case ACI_ERR_NONE:
            goto out;
        case ACI_ERR_SCHAR:
            orgInfo[i].data.schar  = (acp_char_t)va_arg(args, acp_sint32_t); /* BUGBUG gcc 2.96 */
            break;
        case ACI_ERR_STRING:
            orgInfo[i].data.string = va_arg(args, acp_char_t *);
            orgInfo[i].stringLen = acpCStrLen(orgInfo[i].data.string, ACP_SINT32_MAX);
            break;
        case ACI_ERR_STRING_N:
            orgInfo[i].stringLen  = va_arg(args, acp_sint32_t);
            orgInfo[i].data.string = va_arg(args, acp_char_t *);
            break;
        case ACI_ERR_SINT:
            orgInfo[i].data.sint  = va_arg(args, acp_sint32_t);
            break;
        case ACI_ERR_UINT:
            orgInfo[i].data.uint  = va_arg(args, acp_uint32_t);
            break;
        case ACI_ERR_SLONG:
            orgInfo[i].data.slong  = va_arg(args, acp_sint64_t);
            break;
        case ACI_ERR_ULONG:
            orgInfo[i].data.ulong  = va_arg(args, acp_uint64_t);
            break;
        case ACI_ERR_VSLONG:
            orgInfo[i].data.vslong = va_arg(args, acp_slong_t);
            break;
        case ACI_ERR_VULONG:
            orgInfo[i].data.vulong = va_arg(args, acp_ulong_t);
            break;
        case ACI_ERR_HEX32:
            orgInfo[i].data.hex32  = va_arg(args, acp_uint32_t);
            break;
        case ACI_ERR_HEX64:
            orgInfo[i].data.hex64  = va_arg(args, acp_uint64_t);
            break;
        case ACI_ERR_HEX_V:
            orgInfo[i].data.hexv  = va_arg(args, acp_ulong_t);
            break;
        default:
            return ACI_FAILURE;
        }
    }
out:;

    return ACI_SUCCESS;
}

static acp_sint32_t aciConcatErrorArg(acp_char_t *orgBuf, acp_char_t *fmt, aci_arg_info_t *info)
{
    acp_char_t c;
    acp_char_t *curBuf = orgBuf;
    acp_uint32_t  orgBufLen;

    /* ---------------------------
     * [1] formatted string 구성
     * --------------------------*/
    while( (c = *fmt++) != 0)
    {
        if (c == '<') /* [<] 출현 */
        {
            while(*fmt++ != '>') ; /* [>]가 나올때 까지 skip 및 메시지 구현 */

            /* BUG-21296 : HP장비에서 긴 error message로 인해 메리가 긁힘 */
            /* orgBuf에 append시키고 orgBufLen을 재계산해야 하고 */
            /* MAX_ERROR_MSG_LENG보다 긴 error message를 truncate함 */
            orgBufLen = acpCStrLen(orgBuf, ACP_SINT32_MAX);

            switch(info->outputOrder->type_info->type)
            {
            case ACI_ERR_SCHAR:
                if (orgBufLen + ACI_MAX_FORMAT_ITEM_LEN < ACI_MAX_ERROR_MSG_LEN)
                {
                    aciVaAppendFormat(orgBuf, ACI_MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.schar);
                }
                break;
            case ACI_ERR_STRING:
                if (orgBufLen + info->outputOrder->stringLen
                    < ACI_MAX_ERROR_MSG_LEN)
                {
                    aciVaAppendFormat(orgBuf, ACI_MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.string);
                }
                break;
            case ACI_ERR_STRING_N:
                if (orgBufLen + info->outputOrder->stringLen
                    < ACI_MAX_ERROR_MSG_LEN)
                {
                    aciVaAppendFormat(orgBuf, ACI_MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->stringLen,
                                        info->outputOrder->data.string);
                }
                break;
            case ACI_ERR_SINT:
                if (orgBufLen + ACI_MAX_FORMAT_ITEM_LEN < ACI_MAX_ERROR_MSG_LEN)
                {
                    aciVaAppendFormat(orgBuf, ACI_MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.sint);
                }
                break;
            case ACI_ERR_UINT:
                if (orgBufLen + ACI_MAX_FORMAT_ITEM_LEN < ACI_MAX_ERROR_MSG_LEN)
                {
                    aciVaAppendFormat(orgBuf, ACI_MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.uint);
                }
                break;
            case ACI_ERR_SLONG:
                if (orgBufLen + ACI_MAX_FORMAT_ITEM_LEN < ACI_MAX_ERROR_MSG_LEN)
                {
                    aciVaAppendFormat(orgBuf, ACI_MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.slong);
                }
                break;
            case ACI_ERR_ULONG:
                if (orgBufLen + ACI_MAX_FORMAT_ITEM_LEN < ACI_MAX_ERROR_MSG_LEN)
                {
                    aciVaAppendFormat(orgBuf, ACI_MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.ulong);
                }
                break;
            case ACI_ERR_VSLONG:
                if (orgBufLen + ACI_MAX_FORMAT_ITEM_LEN < ACI_MAX_ERROR_MSG_LEN)
                {
                    aciVaAppendFormat(orgBuf, ACI_MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.vslong);
                }
                break;
            case ACI_ERR_VULONG:
                if (orgBufLen + ACI_MAX_FORMAT_ITEM_LEN < ACI_MAX_ERROR_MSG_LEN)
                {
                    aciVaAppendFormat(orgBuf, ACI_MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.vulong);
                }
                break;
            case ACI_ERR_HEX32:
                if (orgBufLen + ACI_MAX_FORMAT_ITEM_LEN < ACI_MAX_ERROR_MSG_LEN)
                {
                    aciVaAppendFormat(orgBuf, ACI_MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.hex32);
                }
                break;
            case ACI_ERR_HEX64:
                if (orgBufLen + ACI_MAX_FORMAT_ITEM_LEN < ACI_MAX_ERROR_MSG_LEN)
                {
                    aciVaAppendFormat(orgBuf, ACI_MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.hex64);
                }
                break;
            case ACI_ERR_HEX_V:
                if (orgBufLen + ACI_MAX_FORMAT_ITEM_LEN < ACI_MAX_ERROR_MSG_LEN)
                {
                    aciVaAppendFormat(orgBuf, ACI_MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.hexv);
                }
            break;
            default:
                break;
            }
            curBuf = orgBuf + acpCStrLen(orgBuf, ACP_SINT32_MAX);
            info++;
        }
        else
        {
            *curBuf++ = c;
        }
    }

    return ACI_SUCCESS;
}

ACP_EXPORT
acp_char_t *aciGetErrorMsg(acp_uint32_t ErrorCode )
{
    static acp_char_t *ideFIND_FAILURE = (acp_char_t *)"Can't find Error Message";
    aci_error_mgr_t *iderr = aciGetErrorMgr();

    if (iderr->LastError == ErrorCode)
    {
        return iderr->LastErrorMsg;
    }
    return ideFIND_FAILURE;
}

/* BUG-15166 */
ACP_EXPORT
acp_uint32_t aciGetErrorArgCount(acp_uint32_t ErrorCode)
{
    acp_uint32_t      Section;
    acp_uint32_t      Index;
    acp_uint32_t      Count;
    acp_char_t   * Fmt = NULL;
    acp_char_t     c;
        
    Section = (ErrorCode & ACI_E_MODULE_MASK) >> 28;
    Index   =  ErrorCode & ACI_E_INDEX_MASK;
    Fmt     = aciErrorStorage[Section].MsgBuf[Index];
    Count   = 0;
    
    while( (c = *Fmt) )
    {
        if (c == '<') /* [<] 출현 */
        {
            Count++;
            Fmt++; 
            while( (c = *Fmt) )
            {
                if (c == '>')
                {
                    break;
                }
                Fmt++; 
            }
        }
        Fmt++; 
    }

    return Count;
}

/* Error Code를 할당할 수 없음. */
ACP_EXPORT
acp_sint32_t aciRegistErrorMsb(acp_char_t *fn)
{
    int             i;
    acp_std_file_t  sFD;
    acp_char_t           **MsgBuf = NULL;
    acp_uint32_t            AltiVer;
    acp_uint64_t           errCount, Section;
    aci_error_msb_type_t  TempMsbHeader;
    aci_error_factory_t *Storage = NULL;

    acp_size_t sRecNum = 0;
    acp_bool_t sEof = ACP_FALSE;
    /* --------------------------
     * [0] 인자 검사
     * -------------------------*/

    if (fn == NULL)
    {
        return ACI_FAILURE;
    }

    /* --------------------------
     * [1] MSB 화일을 읽는다.
     * -------------------------*/
    if (acpStdOpen(&sFD, fn, "rt") != ACP_RC_SUCCESS)
    {
/*
 * BUGBUG: CLIENT does not use log
 * These codes are activated when do poting Altibase server.
#ifndef GEN_ERR_MSG
        aciLog(ACI_SERVER_0, "<%s> MSB File Not Found \n", fn);
#endif
*/
        return ACI_FAILURE;
    }

    /* BUGBUG: lack of read-size checking */
    if (acpStdRead(&sFD, &TempMsbHeader, sizeof(aci_error_msb_type_t), 1, &sRecNum) != ACP_RC_SUCCESS)
    {
        ACI_RAISE(failure_process);
    }

    /* ------------------------------------------
     * [2] 읽은 임시 헤더 데이타 저장
     * -----------------------------------------*/
    AltiVer  = acpByteOrderTOH4(TempMsbHeader.value_.header.AltiVersionId);
    errCount = acpByteOrderTOH8(TempMsbHeader.value_.header.ErrorCount);
    Section  = acpByteOrderTOH8(TempMsbHeader.value_.header.Section);
    TempMsbHeader.value_.header.AltiVersionId = AltiVer;
    TempMsbHeader.value_.header.ErrorCount    = errCount;
    TempMsbHeader.value_.header.Section       = Section;

    /* ------------------------------------------
     * [2.5] 헤더의 버전 정보 검사
     * -----------------------------------------*/
    if (AltiVer != aciVersionID)
    {
        /* BUGBUG: PROJ-1000
         * Do not poring here becuase this is not used by Client.
         */

/* BUGBUG: PROJ-1000
 * Do not poring here becuase this is not used by Client.
 */
/*
#ifndef GEN_ERR_MSG
        aciLog(ACI_SERVER_0, ACI_TRC_MSB_VERSION_INVALID, fn);
#endif
*/
        ACI_RAISE(failure_process);
    }

    if (Section >= ACI_E_MODULE_COUNT)
    {
        ACI_RAISE(failure_process);
    }

    /* ------------------------------------------
     * [3] Error Storage의 Section 결정 및 헤더 저장
     * -----------------------------------------*/
    Storage = &aciErrorStorage[Section];
    (void)acpMemCpy((void **)&Storage->MsbHeader, &TempMsbHeader, sizeof(aci_error_msb_type_t));

    /* ------------------------------------------
     * [4] 실제 에러 메시지를 읽어 들임
     * -----------------------------------------*/
    if (errCount >= ACP_SIZE_MAX)
    {
        ACI_RAISE(failure_process);
    }
    else
    {
        /* do nothing */
    }
    (void)acpMemCalloc((void **)&MsgBuf, errCount, sizeof(acp_char_t *));
    if (MsgBuf == NULL)
    {
        ACI_RAISE(failure_process);
    }

    for (i = 0; sEof != ACP_TRUE && i < (acp_sint32_t)errCount; i++)
    {
        /* 에러 메시지를 읽어서 보관 */
        acp_char_t buffer[1024] = {0};

        if (ACP_RC_IS_SUCCESS(acpStdGetCString(&sFD, buffer, 1024)))
        {
            int len = acpCStrLen(buffer, 1024);
            (void)acpMemCalloc((void **)&MsgBuf[i], 1, len + 1);

            if (MsgBuf[i] == NULL)
            {
                ACI_RAISE(failure_process);
            }

            ACI_TEST_RAISE((len == 0) || (len > (int)sizeof(buffer)),
                           failure_process);
            if (acpCharIsSpace(buffer[len - 1]) == ACP_TRUE) buffer[len - 1] = 0;
            (void)acpCStrCpy(MsgBuf[i], 1024,
                       buffer, len);
        }
        else
        {
            break;
        }

        acpStdIsEOF(&sFD, &sEof);
    }

    (void)acpStdClose(&sFD);

    /* ------------------------------------------
     * [5] Error Storage 완성
     * -----------------------------------------*/
    Storage->MsgBuf = MsgBuf;

    return ACI_SUCCESS;

    ACI_EXCEPTION(failure_process);

    (void)acpStdClose(&sFD);

    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACP_EXPORT
acp_sint32_t aciRegistErrorFromBuffer( acp_uint64_t aSection, 
                                       acp_uint32_t aVersionID,
                                       acp_uint64_t aErrorCount,
                                       acp_char_t **aErrorBuffer )
{
    acp_uint64_t         sSection = 0;
    aci_error_factory_t *sStorage = NULL;

    ACI_TEST( *aErrorBuffer == NULL );

    sSection = (aSection & ACI_E_MODULE_MASK) >> 28;

    ACI_TEST( sSection >= ACI_E_MODULE_COUNT );

    sStorage = &aciErrorStorage[sSection];

    /* Set the CM error storage of the server error factory. */
    sStorage->MsbHeader.value_.header.AltiVersionId = aVersionID;
    sStorage->MsbHeader.value_.header.Section       = sSection;
    sStorage->MsbHeader.value_.header.ErrorCount    = aErrorCount;

    sStorage->MsgBuf = aErrorBuffer;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static void aciSetServerErrorCode(aci_error_mgr_t *aErrorMgr,
                                  acp_uint32_t         aErrorCode,
                                  acp_sint32_t         aSystemErrno,
                                  va_list      aArgs)
{
    static acp_char_t *NULL_MESSAGE           = (acp_char_t *)"No Error Message Loaded";
    static acp_char_t *OUT_BOUND_MESSAGE      = (acp_char_t *)"Out of ErrorCode Bound";
    static acp_char_t *ERR_PARSE_ARG_MESSAGE  = (acp_char_t *)"Invalid Error-Formatted String";
    static acp_char_t *ERR_CONCAT_ARG_MESSAGE = (acp_char_t *)"Invalid Error-Concatenation";
    acp_uint32_t         Section;
    acp_uint32_t         Index;
    aci_arg_info_t   argInfo[ACI_MAX_ARGUMENT];
    aci_error_factory_t *Storage = NULL;

    Section = (aErrorCode & ACI_E_MODULE_MASK) >> 28;
    Index   =  aErrorCode & ACI_E_INDEX_MASK;
    
    Storage = &aciErrorStorage[Section];

    aErrorMgr->LastError       = aErrorCode;
    aErrorMgr->LastSystemErrno = aSystemErrno;

    if (Storage->MsbHeader.value_.header.ErrorCount <= 0 )
    {
        acpCStrCpy(aErrorMgr->LastErrorMsg, ACI_MAX_ERROR_MSG_LEN+256,     
                   NULL_MESSAGE, ACI_MAX_ERROR_MSG_LEN);
    }
    else if (Index < Storage->MsbHeader.value_.header.ErrorCount) /* 에러 메시지를 구성하자. */
    {
        acp_char_t *fmt = Storage->MsgBuf[Index];

        if (fmt == NULL)
        {
            acpCStrCpy(aErrorMgr->LastErrorMsg, ACI_MAX_ERROR_MSG_LEN+256,     
                       NULL_MESSAGE, ACI_MAX_ERROR_MSG_LEN);
        }
        else
        {
            if (aciGetArgumentInfo(fmt, &argInfo[0], aArgs) == ACI_SUCCESS)
            {
                acpMemSet(aErrorMgr->LastErrorMsg, 0, sizeof(aErrorMgr->LastErrorMsg));
                aErrorMgr->LastErrorMsg[0] = 0;
                if(aciConcatErrorArg(aErrorMgr->LastErrorMsg,
                                     fmt,
                                     &argInfo[0]) != ACI_SUCCESS)
                {
                    acpCStrCpy(aErrorMgr->LastErrorMsg, ACI_MAX_ERROR_MSG_LEN+256,
                               ERR_CONCAT_ARG_MESSAGE, ACI_MAX_ERROR_MSG_LEN);
                }
            }
            else
            {
                acpCStrCpy(aErrorMgr->LastErrorMsg, ACI_MAX_ERROR_MSG_LEN+256,
                           ERR_PARSE_ARG_MESSAGE,  ACI_MAX_ERROR_MSG_LEN);
            }
        }
    }
    else
    {
        acpCStrCpy(aErrorMgr->LastErrorMsg,  ACI_MAX_ERROR_MSG_LEN+256,
                   OUT_BOUND_MESSAGE,  ACI_MAX_ERROR_MSG_LEN);
    }

/* BUGBUG: PROJ-1000
 * Do not poring here becuase this is not used by Client.
 */
#if 0
#ifndef GEN_ERR_MSG
    {
        acp_uint32_t sFlag;
        sFlag = aErrorCode & (ACI_E_MODULE_MASK | ACI_E_ACTION_MASK);

        if (sFlag == (ACI_E_ACTION_FATAL | ACI_E_MODULE_SM) ||
            sFlag == (ACI_E_ACTION_FATAL | ACI_E_MODULE_QP) ||
            sFlag == (ACI_E_ACTION_FATAL | ACI_E_MODULE_MT) ||
            sFlag == (ACI_E_ACTION_FATAL | ACI_E_MODULE_RP)
            )
        {
            /*
              aciLogCallStack(ACI_SERVER_0);
              ACI_CALLBACK_FATAL("FATAL Error Code Occurred.");
            */
        }
    }
#endif
#endif
}

ACP_EXPORT
aci_error_mgr_t* aciSetErrorCode(acp_uint32_t ErrorCode, ...)
{
    acp_sint32_t systemErrno = ACP_RC_GET_OS_ERROR();  /*errno; *//* 미리 설정 */
    va_list      args;
    aci_error_mgr_t* sErrMgr = aciGetErrorMgr();

    va_start(args, ErrorCode);

    aciSetServerErrorCode(sErrMgr,
                          ErrorCode,
                          systemErrno,
                          args);
    va_end(args);

    return sErrMgr;
}

/* PROJ-1335 errorCode, errorMsg를 직접 세팅 */
ACP_EXPORT
aci_error_mgr_t* aciSetErrorCodeAndMsg( acp_uint32_t ErrorCode, acp_char_t* ErrorMsg )
{
    acp_sint32_t         systemErrno = ACP_RC_GET_OS_ERROR(); /*errno; *//* 미리 설정 */
    aci_error_mgr_t* sErrMgr = aciGetErrorMgr();

    sErrMgr->LastError       = ErrorCode;
    sErrMgr->LastSystemErrno = systemErrno;

    acpSnprintf( sErrMgr->LastErrorMsg, ACI_MAX_ERROR_MSG_LEN,
                     "%s", ErrorMsg );
    
    return sErrMgr;
}

ACP_EXPORT
void aciDump()
{
/* #ifdef DEBUG *//* BUG-26930 */
/*    acp_sint32_t i;
      aci_error_mgr_t* err = aciGetErrorMgr();*/
/* #endif */
    acp_char_t *errbuf = NULL;
    acp_sint32_t   errbuflen = 1024 * 1024;

    (void)acpMemCalloc((void **)&errbuf, 1, errbuflen);
    /* BUG-25586
     * [CodeSonar::NullPointerDereference] ideDump() 에서 발생 */
    ACI_TEST_RAISE( errbuf == NULL, skip_err_dump );

    {
        aci_error_mgr_t *iderr = aciGetErrorMgr();
/* BUGBUG: PROJ-1000
 * Do not poring here becuase this is not used by Client.
 */
#if 0
#ifndef GEN_ERR_MSG
        acp_uint32_t sSourceInfo = 0;
        /* ------------------------------------------------
         *  genErrMsg를 만들경우 property를
         *  참조하지 않도록!
         * ----------------------------------------------*/
        /* NOTICE: PROJ-1000
         * original code : (void)idp::read("SOURCE_INFO", &sSourceInfo);
         * idp class is not been porting in PROJ-1000 because it is
         * not client code.
         * Therefor aciPropertyGetSourceInfo() is used instead.
         */
        sSourceInfo = aciPropertyGetSourceInfo();

        if ( (sSourceInfo & ACI_SOURCE_INFO_SERVER) == 0)
        {
            aciVaAppendFormat(errbuf, errbuflen, " ERR-%05X(errno=%d): %s\n",
                                ACI_E_ERROR_CODE(aciGetErrorCode()),
                                aciGetSystemErrno(),
                                aciGetErrorMsg(aciGetErrorCode()));
        }
        else
#endif
#endif
        {
            acpSnprintf(errbuf, errbuflen, "ERR-%05X (%s:%d)[errno=%d] : %s\n",
                            ACI_E_ERROR_CODE(aciGetErrorCode()),
                            iderr->LastErrorFile,
                            iderr->LastErrorLine,
                            aciGetSystemErrno(),
                            aciGetErrorMsg(aciGetErrorCode()));
        }
    }
/* #ifdef DEBUG *//* BUG-26930 */
    /*
    for( i = 0; i < (acp_sint32_t)err->ErrorIndex; i++)
    {
        aciVaAppendFormat(errbuf, errbuflen, " %d: %s:%d\t",
                            i,
                            err->ErrorFile[i],
                            err->ErrorLine[i]);
        aciVaAppendFormat(errbuf, errbuflen, "\t%s\n",
                            err->TestLine[i]);
    }*/
/* #endif */
    acpFprintf(ACP_STD_ERR, "%s\n", errbuf);
    acpMemFree(errbuf);
    aciClearError();

    ACI_EXCEPTION_CONT( skip_err_dump );
}


/* ------------------------------------------------
 *  Client Part Error Managing
 * ----------------------------------------------*/

ACP_EXPORT
void aciSetClientErrorCode(aci_client_error_mgr_t     *aErrorMgr,
                           aci_client_error_factory_t *aFactory,
                           acp_uint32_t                   aErrorCode,
                           va_list                aArgs)
{
    static acp_char_t *NULL_MESSAGE           = (acp_char_t *)"No Error Message Loaded";
    static acp_char_t *OUT_BOUND_MESSAGE      = (acp_char_t *)"Out of ErrorCode Bound";
    static acp_char_t *ERR_PARSE_ARG_MESSAGE  = (acp_char_t *)"Invalid Error-Formatted String";
    static acp_char_t *ERR_CONCAT_ARG_MESSAGE = (acp_char_t *)"Invalid Error-Concatenation";
    acp_uint32_t         Index;
    aci_arg_info_t   argInfo[ACI_MAX_ARGUMENT];
    
    Index   =  aErrorCode & ACI_E_INDEX_MASK;
    
    aErrorMgr->mErrorCode       = aErrorCode;

    if (1)/* Index < Storage->MsbHeader.value_.header.ErrorCount) 에러 메시지를 구성하자. */
    {
        acp_char_t *fmt = (acp_char_t *)aFactory[Index].mErrorMsg;

        if (fmt == NULL)
        {
            acpCStrCpy(aErrorMgr->mErrorMessage, ACI_MAX_ERROR_MSG_LEN+256,
                       NULL_MESSAGE, ACI_MAX_ERROR_MSG_LEN);
            
        }
        else
        {
            if (aciGetArgumentInfo(fmt, &argInfo[0], aArgs) == ACI_SUCCESS)
            {
                /* ------------------------------------------------
                 * Normal Error Code Setting
                 * ----------------------------------------------*/
                
                acpMemSet(aErrorMgr->mErrorMessage, 0, sizeof(aErrorMgr->mErrorMessage));
                aErrorMgr->mErrorMessage[0] = 0;
                if(aciConcatErrorArg(aErrorMgr->mErrorMessage,
                                     fmt,
                                     &argInfo[0]) != ACI_SUCCESS)
                {
                    acpCStrCpy(aErrorMgr->mErrorMessage, ACI_MAX_ERROR_MSG_LEN+256,
                               ERR_CONCAT_ARG_MESSAGE, ACI_MAX_ERROR_MSG_LEN);
                }
                acpCStrCpy(aErrorMgr->mErrorState,
                           ACI_SIZEOF(aErrorMgr->mErrorState),
                           aFactory[Index].mErrorState,
                           ACI_SIZEOF(aErrorMgr->mErrorState));
            }
            else
            {
                acpCStrCpy(aErrorMgr->mErrorMessage, ACI_MAX_ERROR_MSG_LEN+256,
                           ERR_PARSE_ARG_MESSAGE, ACI_MAX_ERROR_MSG_LEN);
            }
        }
    }
    else
    {
        acpCStrCpy(aErrorMgr->mErrorMessage,  ACI_MAX_ERROR_MSG_LEN+256,
                   OUT_BOUND_MESSAGE, ACI_MAX_ERROR_MSG_LEN);
    }
}

ACP_EXPORT
struct aci_error_mgr_t *aciGetErrorMgr()
{
    ACP_TLS(aci_error_mgr_t, sErrMgr);

    return &sErrMgr;
}


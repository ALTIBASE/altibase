/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: ideErrorMgr.cpp 90135 2021-03-05 06:50:13Z kclee $
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

#include <idl.h>
#include <idp.h>

#if defined(VC_WIN32)
#    include <stdio.h>
#endif

#include <ideErrorMgr.h>
#include <iduVersion.h>
#ifndef GEN_ERR_MSG
#include <idErrorCode.h>
#include <ideLog.h>
#endif
#define idERR_IGNORE_NoError 0x42000000
#include <idtContainer.h>
#include <iduMemMgr.h>

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

#ifdef SPARC_SOLARIS
# ifdef COMPILE_64BIT
#  ifndef __GNUC__
int sys_nerr;
#  endif
# endif
#endif

#if defined(ITRON) || defined(WIN32_ODBC_CLI2)
struct ideErrorMgr *toppers_error = NULL;
#endif

/* TASK-6739 Altibase 710 성능 개선 */
IDTHREAD ideErrorMgr gIdeErrorMgr;

ideErrTypeInfo typeInfo[] = // 에러 메시지 화일에 사용되는 데이타 타입 종류
{
    { IDE_ERR_SCHAR   , "%c"  , "%c"  , 2 },
    { IDE_ERR_STRING  , "%s"  , "%s"  , 2 },
    { IDE_ERR_STRING_N, "%.*s", "%.*s", 4 }, /* BUG-45567 */
    { IDE_ERR_SINT    , "%d"  , "%d"  , 2 },
    { IDE_ERR_UINT    , "%u"  , "%u"  , 2 },
    { IDE_ERR_SLONG   , "%ld" , "%lld", 3 },
    { IDE_ERR_ULONG   , "%lu" , "%llu", 3 },
    { IDE_ERR_VSLONG  , "%vd" , "%ld" , 3 },
    { IDE_ERR_VULONG  , "%vu" , "%lu" , 3 },
    { IDE_ERR_HEX32   , "%x"  , "%x"  , 2 },
    { IDE_ERR_HEX64   , "%lx" , "%llx", 3 },
    { IDE_ERR_HEX_V   , "%vx" , "%lx" , 3 },
    { IDE_ERR_NONE    , ""    , ""    , 0 },
};

/* ----------------------------------------------------------------------
 *
 *  Error Code Conversion Matrix
 *
 * ---------------------------------------------------------------------- */

const idBool ideErrorConversionMatrix[IDE_MAX_ERROR_ACTION][IDE_MAX_ERROR_ACTION] = // [OLD][NEW]
{
    //             OLD : FATAL
    //  NEW : FATAL     ABORT     IGNORE    RETRY    REBUILD
    {        ID_FALSE, ID_FALSE, ID_FALSE, ID_FALSE, ID_FALSE },
    //             OLD : ABORT
    //  NEW : FATAL     ABORT     IGNORE    RETRY    REBUILD
    {        ID_TRUE,  ID_FALSE, ID_FALSE, ID_FALSE, ID_FALSE },
    //             OLD : IGNORE
    //  NEW : FATAL     ABORT     IGNORE    RETRY    REBUILD
    {        ID_TRUE,  ID_TRUE,  ID_FALSE, ID_TRUE,  ID_TRUE  },
    //             OLD : RETRY
    //  NEW : FATAL     ABORT     IGNORE    RETRY    REBUILD
    {        ID_TRUE,  ID_TRUE,  ID_FALSE, ID_FALSE, ID_TRUE  },
    //             OLD : REBUILD
    //  NEW : FATAL     ABORT     IGNORE    RETRY    REBUILD
    {        ID_TRUE,  ID_TRUE,  ID_FALSE, ID_FALSE, ID_FALSE },
};

/* ----------------------------------------------------------------------
 *
 *  MT 클라이언트를 위한 컴파일 - shore storage manager 참조
 *
 * ---------------------------------------------------------------------- */

// fix BUG-17287

void  ideClearTSDError(ideErrorMgr* aIdErr)
{
    assert(aIdErr != NULL);
    aIdErr->Stack.LastError        = idERR_IGNORE_NoError;
    aIdErr->Stack.LastSystemErrno  = 0;
    aIdErr->Stack.HasErrorPosition = ID_FALSE;
    idlOS::memset(aIdErr->Stack.LastErrorMsg, 0, MAX_ERROR_MSG_LEN);
}

/* PROJ-2617 */
struct ideFaultMgr* ideGetFaultMgr()
{
    return &ideGetErrorMgr()->mFaultMgr;
}

void ideLogError(const SChar* aErrInfo,
                 const idBool aAcceptFaultTolerance, /* PROJ-2617 */
                 const SChar* aFile,
                 const SInt   aLine,
                 const SChar* aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    ideLog::writeErrorTrace(aErrInfo,
                            aAcceptFaultTolerance,
                            aFile, aLine,
                            aFormat, args);
    va_end(args);
}

/* ----------------------------------------------------------------------
 *
 *  ID 에러 코드값 얻기 : SERVER, CLIENT 공통 함수
 *
 * ----------------------------------------------------------------------*/

UInt ideGetErrorCode()
{
    return gIdeErrorMgr.Stack.LastError;
}

SChar* ideGetErrorMsg()
{
    return gIdeErrorMgr.Stack.LastErrorMsg;
}

SInt ideGetSystemErrno()
{
    return gIdeErrorMgr.Stack.LastSystemErrno;
}

UInt   ideGetErrorLine()
{
    return gIdeErrorMgr.Stack.LastErrorLine;
}

SChar *ideGetErrorFile()
{
    return gIdeErrorMgr.Stack.LastErrorFile;
}

// BUG-38883
idBool ideHasErrorPosition()
{
    return gIdeErrorMgr.Stack.HasErrorPosition;
}

// BUG-38883
void   ideSetHasErrorPosition()
{
    gIdeErrorMgr.Stack.HasErrorPosition = ID_TRUE;
}

// BUG-38883
void   ideInitHasErrorPosition()
{
    gIdeErrorMgr.Stack.HasErrorPosition = ID_FALSE;
}

idBool ideNoErrorYet()
{
    return (idBool)(gIdeErrorMgr.Stack.LastError == idERR_IGNORE_NoError);
}

SInt ideFindErrorCode(UInt ErrorCode)
{
    if (gIdeErrorMgr.Stack.LastError == ErrorCode)
    {
        return IDE_SUCCESS;
    }
    return IDE_FAILURE;
}

SInt ideFindErrorAction(UInt Action)
{
    if ((gIdeErrorMgr.Stack.LastError & E_ACTION_MASK) == Action)
    {
        return IDE_SUCCESS;
    }
    return IDE_FAILURE;
}

IDE_RC ideFindErrorAction( UInt aErrorCode, UInt Action )
{
    if ( ( aErrorCode & E_ACTION_MASK ) == Action )
    {
        return IDE_SUCCESS;
    }
    return IDE_FAILURE;
}

SInt   ideIsAbort()
{
    return ideFindErrorAction(E_ACTION_ABORT);
}

SInt   ideIsFatal()
{
    return ideFindErrorAction(E_ACTION_FATAL);
}

SInt   ideIsIgnore()
{
    return ideFindErrorAction(E_ACTION_IGNORE);
}

SInt   ideIsRetry()
{
    return ideFindErrorAction(E_ACTION_RETRY);
}

SInt   ideIsRebuild()
{
    return ideFindErrorAction(E_ACTION_REBUILD);
}

IDE_RC ideIsRetry( UInt aErrorCode )
{
    return ideFindErrorAction( aErrorCode, E_ACTION_RETRY );
}

IDE_RC ideIsRebuild( UInt aErrorCode )
{
    return ideFindErrorAction( aErrorCode, E_ACTION_REBUILD );
}

/* ----------------------------------------------------------------------
 *
 *  에러 코드값 설정 + 메시지 구성
 *
 * ----------------------------------------------------------------------*/

IDL_EXTERN_C void ideClearError()
{
    if(gIdeErrorMgr.Stack.LastError != idERR_IGNORE_NoError)
    {
        gIdeErrorMgr.Stack.LastError        = idERR_IGNORE_NoError;
        gIdeErrorMgr.Stack.LastSystemErrno  = 0;
        gIdeErrorMgr.Stack.HasErrorPosition = ID_FALSE;
        idlOS::memset(gIdeErrorMgr.Stack.LastErrorMsg, 0, MAX_ERROR_MSG_LEN);
    }
    else
    {
    }

    /* BUG-47635 */
    ideErrorCollectionClear();

#ifdef DEBUG
    gIdeErrorMgr.ErrorTested = 0;
    gIdeErrorMgr.ErrorIndex  = 0;
#endif
}

#ifdef DEBUG
SInt ideSetDebugInfo(SChar *File,     // 에러 화일
                     UInt   Line,     // 에러 라인
                     SChar *testline) // 에러정보
{
    ideErrorMgr* error = ideGetErrorMgr();
    if( error->ErrorTested)
    {    
        error->ErrorIndex++;
        error->ErrorTested = 0; 
    }    
    if( error->ErrorIndex < MAX_ERROR_STACK )
    {    
        idlOS::snprintf( error->ErrorTestLine [error->ErrorIndex], MAX_ERROR_TEST_LEN, "ideSetErrorCode( %s )", testline );
        idlOS::snprintf( error->ErrorFile[error->ErrorIndex], MAX_ERROR_FILE_LEN, "%s", File );
        error->ErrorLine[error->ErrorIndex] = Line;
        error->ErrorIndex++;
    }
    return IDE_SUCCESS;
}
#endif


/* ----------------------------------------------------------------------
 *
 *  서버를 위한 ID 에러 메시지 로딩 및 얻기
 *
 * ----------------------------------------------------------------------*/

static ideErrorFactory ideErrorStorage[E_MODULE_COUNT];

// formatted stirng을 읽어서 가변인자 정보를 구성
static SInt ideGetArgumentInfo(SChar *orgFmt, ideArgInfo *orgInfo, va_list args)
{
    UInt  maxInfoCnt = 0;
    UInt  outputOrder = 0;
    UInt  inputOrder;
    SChar c;
    SInt  i;
    ideArgInfo *info;
    SChar      *fmt;

    fmt = orgFmt;
    orgInfo[0].type_info = NULL; // 초기화

    while(( c = *fmt++) )
    {
        if (c == '<') // [<] 출현
        {
            SChar numBuf[8]; // 인자번호 입력

            /* ------------------
             * [1] 인자번호 얻기
             * -----------------*/
            if (isdigit(*fmt) == 0) // 숫자가 아님
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
            // 몇번째 입력 인자인가? 포인터 대입
            inputOrder        = (UInt)idlOS::strtol(numBuf, NULL, 10);
            if (inputOrder >= MAX_ARGUMENT)
            {
                 return IDE_FAILURE;
            }
            info              = &orgInfo[inputOrder];

            orgInfo[outputOrder++].outputOrder = info;

            if (inputOrder > maxInfoCnt) maxInfoCnt = inputOrder;

            /* ------------------
             * [2] 인자타입 얻기
             * -----------------*/
            for (i = 0; ; i++)
            {
                if (typeInfo[i].type == IDE_ERR_NONE)
                {
                    return IDE_FAILURE;
                }
                if (idlOS::strncmp(fmt,
                                   typeInfo[i].tmpSpecStr,
                                   typeInfo[i].len) == 0)
                {
                    info->type_info = &typeInfo[i];
                    fmt += typeInfo[i].len;
                    break;
                }
            }
            if ( *fmt != '>')
            {
                return IDE_FAILURE;
            }
            fmt++;
        }
    }
    if (maxInfoCnt >= MAX_ARGUMENT - 1)
    {
        return IDE_FAILURE;
    }
    orgInfo[maxInfoCnt + 1].type_info = NULL; // NULL을 지정 ; 마지막 flag

    /* ------------------
     * [3] 인자 포인터 대입
     //     argument *를 다시 저장.
     * -----------------*/

    for (i = 0; ; i++) // BUGBUG : sizeof(data types)로 해서 미리 계산할 수 있음.
    {
        ideErrTypeInfo *typeinfo = orgInfo[i].type_info;
        if (typeinfo == NULL) break;
        switch(typeinfo->type)
        {
        case IDE_ERR_NONE:
            goto out;
        case IDE_ERR_SCHAR:
            orgInfo[i].data.schar  = (SChar)va_arg(args, SInt); /* BUGBUG gcc 2.96 */
            break;
        case IDE_ERR_STRING:
            orgInfo[i].data.string = va_arg(args, SChar *);
            orgInfo[i].stringLen = strlen(orgInfo[i].data.string);
            break;
        case IDE_ERR_STRING_N:
            orgInfo[i].stringLen  = va_arg(args, SInt);
            orgInfo[i].data.string = va_arg(args, SChar *);
            break;
        case IDE_ERR_SINT:
            orgInfo[i].data.sint  = va_arg(args, SInt);
            break;
        case IDE_ERR_UINT:
            orgInfo[i].data.uint  = va_arg(args, UInt);
            break;
        case IDE_ERR_SLONG:
            orgInfo[i].data.slong  = va_arg(args, SLong);
            break;
        case IDE_ERR_ULONG:
            orgInfo[i].data.ulong  = va_arg(args, ULong);
            break;
        case IDE_ERR_VSLONG:
            orgInfo[i].data.vslong = va_arg(args, vSLong);
            break;
        case IDE_ERR_VULONG:
            orgInfo[i].data.vulong = va_arg(args, vULong);
            break;
        case IDE_ERR_HEX32:
            orgInfo[i].data.hex32  = va_arg(args, UInt);
            break;
        case IDE_ERR_HEX64:
            orgInfo[i].data.hex64  = va_arg(args, ULong);
            break;
        case IDE_ERR_HEX_V:
            orgInfo[i].data.hexv  = va_arg(args, vULong);
            break;
        default:
            return IDE_FAILURE;
        }
    }
out:;

    return IDE_SUCCESS;
}

static SInt ideConcatErrorArg(SChar *orgBuf, SChar *fmt, ideArgInfo *info)
{
    SChar c;
    SChar *curBuf = orgBuf;
    UInt  orgBufLen;

    /* ---------------------------
     * [1] formatted string 구성
     * --------------------------*/
    while( (c = *fmt++) != 0)
    {
        if (c == '<') // [<] 출현
        {
            while(*fmt++ != '>') ; //[>]가 나올때 까지 skip 및 메시지 구현

            // BUG-21296 : HP장비에서 긴 error message로 인해 메리가 긁힘
            // orgBuf에 append시키고 orgBufLen을 재계산해야 하고
            // MAX_ERROR_MSG_LENG보다 긴 error message를 truncate함
            orgBufLen = idlOS::strlen(orgBuf);

            switch(info->outputOrder->type_info->type)
            {
            case IDE_ERR_SCHAR:
                if (orgBufLen + MAX_FORMAT_ITEM_LEN < MAX_ERROR_MSG_LEN)
                {
                    idlVA::appendFormat(orgBuf, MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.schar);
                }
                break;
            case IDE_ERR_STRING:
                if (orgBufLen + info->outputOrder->stringLen
                    < MAX_ERROR_MSG_LEN)
                {
                    idlVA::appendFormat(orgBuf, MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.string);
                }
                break;
            case IDE_ERR_STRING_N:
                if (orgBufLen + info->outputOrder->stringLen
                    < MAX_ERROR_MSG_LEN)
                {
                    idlVA::appendFormat(orgBuf, MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->stringLen,
                                        info->outputOrder->data.string);
                }
                break;
            case IDE_ERR_SINT:
                if (orgBufLen + MAX_FORMAT_ITEM_LEN < MAX_ERROR_MSG_LEN)
                {
                    idlVA::appendFormat(orgBuf, MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.sint);
                }
                break;
            case IDE_ERR_UINT:
                if (orgBufLen + MAX_FORMAT_ITEM_LEN < MAX_ERROR_MSG_LEN)
                {
                    idlVA::appendFormat(orgBuf, MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.uint);
                }
                break;
            case IDE_ERR_SLONG:
                if (orgBufLen + MAX_FORMAT_ITEM_LEN < MAX_ERROR_MSG_LEN)
                {
                    idlVA::appendFormat(orgBuf, MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.slong);
                }
                break;
            case IDE_ERR_ULONG:
                if (orgBufLen + MAX_FORMAT_ITEM_LEN < MAX_ERROR_MSG_LEN)
                {
                    idlVA::appendFormat(orgBuf, MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.ulong);
                }
                break;
            case IDE_ERR_VSLONG:
                if (orgBufLen + MAX_FORMAT_ITEM_LEN < MAX_ERROR_MSG_LEN)
                {
                    idlVA::appendFormat(orgBuf, MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.vslong);
                }
                break;
            case IDE_ERR_VULONG:
                if (orgBufLen + MAX_FORMAT_ITEM_LEN < MAX_ERROR_MSG_LEN)
                {
                    idlVA::appendFormat(orgBuf, MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.vulong);
                }
                break;
            case IDE_ERR_HEX32:
                if (orgBufLen + MAX_FORMAT_ITEM_LEN < MAX_ERROR_MSG_LEN)
                {
                    idlVA::appendFormat(orgBuf, MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.hex32);
                }
                break;
            case IDE_ERR_HEX64:
                if (orgBufLen + MAX_FORMAT_ITEM_LEN < MAX_ERROR_MSG_LEN)
                {
                    idlVA::appendFormat(orgBuf, MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.hex64);
                }
                break;
            case IDE_ERR_HEX_V:
                if (orgBufLen + MAX_FORMAT_ITEM_LEN < MAX_ERROR_MSG_LEN)
                {
                    idlVA::appendFormat(orgBuf, MAX_ERROR_MSG_LEN,
                                        info->outputOrder->type_info->realSpecStr,
                                        info->outputOrder->data.hexv);
                }
            break;
            default:
                break;
            }
            curBuf = orgBuf + idlOS::strlen(orgBuf);
            info++;
        }
        else
        {
            *curBuf++ = c;
        }
    }

    return IDE_SUCCESS;
}

SChar *ideGetErrorMsg(UInt ErrorCode )
{
    static SChar *ideFIND_FAILURE = (SChar *)"Can't find Error Message";

    if (gIdeErrorMgr.Stack.LastError == ErrorCode)
    {
        return gIdeErrorMgr.Stack.LastErrorMsg;
    }
    return ideFIND_FAILURE;
}

// BUG-15166
UInt ideGetErrorArgCount(UInt ErrorCode)
{
    UInt      Section;
    UInt      Index;
    UInt      Count;
    SChar   * Fmt;
        
    Section = (ErrorCode & E_MODULE_MASK) >> 28;
    Index   =  ErrorCode & E_INDEX_MASK;
    Fmt     = ideErrorStorage[Section].MsgBuf[Index];
    Count   = 0;
    
    while( *Fmt != '\0')
    {
        if (*Fmt == '<') // [<] 출현
        {
            Count++;
            Fmt++;
            while( *Fmt != '\0')
            {
                if (*Fmt == '>')
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

// Error Code를 할당할 수 없음.
SInt ideRegistErrorMsb(SChar *fn)
{
    int             i;
    PDL_HANDLE      sFD;
    SChar           **MsgBuf;
    UInt            AltiVer;
    ULong           errCount, Section;
    idErrorMsbType  TempMsbHeader;
    ideErrorFactory *Storage;

    /* --------------------------
     * [0] 인자 검사
     * -------------------------*/

    if (fn == NULL)
    {
        return IDE_FAILURE;
    }

    /* --------------------------
     * [1] MSB 화일을 읽는다.
     * -------------------------*/
#ifndef GEN_ERR_MSG
    if ( (sFD = idf::open(fn, O_RDONLY)) == PDL_INVALID_HANDLE)
#else
    if ( (sFD = idlOS::open(fn, O_RDONLY)) == PDL_INVALID_HANDLE)
#endif
    {
#ifndef GEN_ERR_MSG
        ideLog::log(IDE_SERVER_0, ID_TRC_MSB_NOT_FOUND, fn);
#endif
        return IDE_FAILURE;
    }
#ifndef GEN_ERR_MSG
    if (idf::read(sFD, &TempMsbHeader, sizeof(idErrorMsbType)) <= 0)
#else
    if (idlOS::read(sFD, &TempMsbHeader, sizeof(idErrorMsbType)) <= 0)
#endif
    {
        IDE_RAISE(failure_process);
    }

    /* ------------------------------------------
     * [2] 읽은 임시 헤더 데이타 저장
     * -----------------------------------------*/
    AltiVer  = idlOS::ntoh(TempMsbHeader.value_.header.AltiVersionId);
    errCount = idlOS::ntoh(TempMsbHeader.value_.header.ErrorCount);
    Section  = idlOS::ntoh(TempMsbHeader.value_.header.Section);
    TempMsbHeader.value_.header.AltiVersionId = AltiVer;
    TempMsbHeader.value_.header.ErrorCount    = errCount;
    TempMsbHeader.value_.header.Section       = Section;

    /* ------------------------------------------
     * [2.5] 헤더의 버전 정보 검사
     * -----------------------------------------*/

    if (AltiVer != iduVersionID)
    {
#ifndef GEN_ERR_MSG
        ideLog::log(IDE_SERVER_0, ID_TRC_MSB_VERSION_INVALID, fn);
#endif
        IDE_RAISE(failure_process);
    }

    if (Section >= E_MODULE_COUNT)
    {
        IDE_RAISE(failure_process);
    }

    /* ------------------------------------------
     * [3] Error Storage의 Section 결정 및 헤더 저장
     * -----------------------------------------*/
    Storage = &ideErrorStorage[Section];
    idlOS::memcpy(&Storage->MsbHeader, &TempMsbHeader, sizeof(idErrorMsbType));

    /* ------------------------------------------
     * [4] 실제 에러 메시지를 읽어 들임
     * -----------------------------------------*/
    MsgBuf = (SChar **)idlOS::malloc(errCount * sizeof(SChar *));
    if (MsgBuf == NULL)
    {
        IDE_RAISE(failure_process);
    }
    idlOS::memset(MsgBuf, 0, errCount * sizeof(SChar *)); 

#ifndef GEN_ERR_MSG
    for (i = 0; !idf::fdeof(sFD) && i < (SInt)errCount; i++)
#else
    for (i = 0; !idlOS::fdeof(sFD) && i < (SInt)errCount; i++)
#endif
    {
        // 에러 메시지를 읽어서 보관
        SChar buffer[1024];

#ifndef GEN_ERR_MSG
        if (idf::fdgets(buffer, 1024, sFD))
#else
        if (idlOS::fdgets(buffer, 1024, sFD))
#endif
        {
            UInt len = idlOS::strlen(buffer);
            MsgBuf[i] = (SChar *)idlOS::calloc(1, len + 1);

            if (MsgBuf[i] == NULL)
            {
                IDE_RAISE(failure_process);
            }

            IDE_TEST_RAISE((len == 0) || (len > sizeof(buffer)), failure_process);
            if (isspace(buffer[len - 1])) buffer[len - 1] = 0;
            idlOS::strncpy(MsgBuf[i], buffer, len); //BUG-48552
        }
        else
        {
            break;
        }
    }

#ifndef GEN_ERR_MSG
    (void)idf::close(sFD);
#else
    (void)idlOS::close(sFD);
#endif
    /* ------------------------------------------
     * [5] Error Storage 완성
     * -----------------------------------------*/
    Storage->MsgBuf = MsgBuf;

    return IDE_SUCCESS;

    IDE_EXCEPTION(failure_process);
#ifndef GEN_ERR_MSG
    (void)idf::close(sFD);
#else
    (void)idlOS::close(sFD);
#endif

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

SInt ideFreeErrorMsb(SChar * /* fn */)
{
    SInt  i;
    ULong j;
    ideErrorFactory *Storage;

    for(i = 0; i < E_MODULE_COUNT; i++)
    {
        Storage = &ideErrorStorage[i];
        if(Storage->MsbHeader.value_.header.ErrorCount != 0)
        {
            for(j = 0; j < Storage->MsbHeader.value_.header.ErrorCount; j++)
            {
                (void)idlOS::free(Storage->MsgBuf[j]);
            }

            (void)idlOS::free(Storage->MsgBuf);
            Storage->MsbHeader.value_.header.ErrorCount = 0;
        }
        else
        {
            /* continue */
        }
    }
    return IDE_SUCCESS;
}

static void ideSetServerErrorCode(ideErrorMgr *aErrorMgr,
                                  UInt         aErrorCode,
                                  SInt         aSystemErrno,
                                  va_list      aArgs)
{
    static SChar *NULL_MESSAGE           = (SChar *)"No Error Message Loaded";
    static SChar *OUT_BOUND_MESSAGE      = (SChar *)"Out of ErrorCode Bound";
    static SChar *ERR_PARSE_ARG_MESSAGE  = (SChar *)"Invalid Error-Formatted String";
    static SChar *ERR_CONCAT_ARG_MESSAGE = (SChar *)"Invalid Error-Concatenation";

    /* 에러 메시지 구성 준비*/
    UInt         Section;
    UInt         Index;
    ideArgInfo   argInfo[MAX_ARGUMENT];
    Section = (aErrorCode & E_MODULE_MASK) >> 28;
    Index   =  aErrorCode & E_INDEX_MASK;
    ideErrorFactory *Storage = &ideErrorStorage[Section];

    aErrorMgr->Stack.LastError       = aErrorCode;
    aErrorMgr->Stack.LastSystemErrno = aSystemErrno;

    if (Storage->MsbHeader.value_.header.ErrorCount <= 0 )
    {
        aErrorMgr->Stack.HasErrorPosition = ID_FALSE;
        idlOS::strcpy(aErrorMgr->Stack.LastErrorMsg, NULL_MESSAGE );
    }
    else if (Index < Storage->MsbHeader.value_.header.ErrorCount) // 에러 메시지를 구성하자.
    {
        SChar *fmt = Storage->MsgBuf[Index];

        if (fmt == NULL)
        {
            aErrorMgr->Stack.HasErrorPosition = ID_FALSE;
            idlOS::strcpy(aErrorMgr->Stack.LastErrorMsg, NULL_MESSAGE);
        }
        else
        {
            if (ideGetArgumentInfo(fmt, &argInfo[0], aArgs) == IDE_SUCCESS)
            {
                idlOS::memset(aErrorMgr->Stack.LastErrorMsg, 0,
                              sizeof(aErrorMgr->Stack.LastErrorMsg));
                aErrorMgr->Stack.LastErrorMsg[0] = 0;

                if(ideConcatErrorArg(aErrorMgr->Stack.LastErrorMsg,
                                     fmt,
                                     &argInfo[0]) != IDE_SUCCESS)
                {
                    aErrorMgr->Stack.HasErrorPosition = ID_FALSE;
                    idlOS::strcpy(aErrorMgr->Stack.LastErrorMsg,
                                  ERR_CONCAT_ARG_MESSAGE);
                }
            }
            else
            {
                aErrorMgr->Stack.HasErrorPosition = ID_FALSE;
                idlOS::strcpy(aErrorMgr->Stack.LastErrorMsg,
                              ERR_PARSE_ARG_MESSAGE);
            }
        }
    }
    else
    {
        aErrorMgr->Stack.HasErrorPosition = ID_FALSE;
        idlOS::strcpy(aErrorMgr->Stack.LastErrorMsg, OUT_BOUND_MESSAGE);
    }

#ifndef GEN_ERR_MSG
    {
        UInt sFlag;
        sFlag = aErrorCode & (E_MODULE_MASK | E_ACTION_MASK);

        if (sFlag == (E_ACTION_FATAL | E_MODULE_SM) ||
            sFlag == (E_ACTION_FATAL | E_MODULE_QP) ||
            sFlag == (E_ACTION_FATAL | E_MODULE_MT) ||
            sFlag == (E_ACTION_FATAL | E_MODULE_RP)
            )
        {
            ideLog::logCallStack( IDE_ERR_0 );
            IDE_CALLBACK_FATAL("FATAL Error Code Occurred.");
        }
    }
#endif
}

ideErrorMgr* ideSetErrorCode(UInt ErrorCode, ...)
{
    SInt         systemErrno = errno; // 미리 설정
    va_list      args;

    va_start(args, ErrorCode);

    ideSetServerErrorCode(&gIdeErrorMgr,
                          ErrorCode,
                          systemErrno,
                          args);
    va_end(args);

    return &gIdeErrorMgr;
}

#ifdef ALTIBASE_FIT_CHECK

ideErrorMgr* ideFitSetErrorCode( UInt aErrorCode, va_list aArgs )
{
    SInt         sSystemErrno = errno; // 미리 설정

    ideSetServerErrorCode( &gIdeErrorMgr,
                           aErrorCode,
                           sSystemErrno,
                           aArgs );

    return &gIdeErrorMgr;
}
#endif

// PROJ-1335 errorCode, errorMsg를 직접 세팅
ideErrorMgr* ideSetErrorCodeAndMsg( UInt ErrorCode, SChar* ErrorMsg )
{
    SInt         systemErrno = errno; // 미리 설정

    gIdeErrorMgr.Stack.LastError       = ErrorCode;
    gIdeErrorMgr.Stack.LastSystemErrno = systemErrno;

    idlOS::snprintf( gIdeErrorMgr.Stack.LastErrorMsg, MAX_ERROR_MSG_LEN,
                     "%s", ErrorMsg );
    
    return &gIdeErrorMgr;
}

/* PROJ-2617 */
void ideCopyErrorInfo(ideErrorMgr       *aDestErrorMgr,
                      const ideErrorMgr *aSrcErrorMgr)
{
    idlOS::memcpy(aDestErrorMgr,
                  aSrcErrorMgr,
                  ID_SIZEOF(ideErrorMgr) - ID_SIZEOF(ideFaultMgr));
}

void ideDump()
{
    SChar *errbuf;
    SInt   errbuflen = 1024 * 1024;

    errbuf = (SChar *)idlOS::calloc(1, errbuflen);
    /* BUG-25586
     * [CodeSonar::NullPointerDereference] ideDump() 에서 발생 */
    IDE_TEST_RAISE( errbuf == NULL, skip_err_dump );

    {
#ifndef GEN_ERR_MSG
        UInt sSourceInfo = 0;
        /* ------------------------------------------------
         *  genErrMsg를 만들경우 property를
         *  참조하지 않도록!
         * ----------------------------------------------*/
        (void)idp::read("SOURCE_INFO", &sSourceInfo);

        if ( (sSourceInfo & IDE_SOURCE_INFO_SERVER) == 0)
        {
            idlVA::appendFormat(errbuf, errbuflen, " ERR-%05X(errno=%d): %s\n",
                                E_ERROR_CODE(ideGetErrorCode()),
                                ideGetSystemErrno(),
                                ideGetErrorMsg(ideGetErrorCode()));
        }
        else
#endif
        {
            idlOS::snprintf(errbuf, errbuflen, "ERR-%05X (%s:%d)[errno=%d] : %s\n",
                            E_ERROR_CODE(ideGetErrorCode()),
                            gIdeErrorMgr.Stack.LastErrorFile,
                            gIdeErrorMgr.Stack.LastErrorLine,
                            ideGetSystemErrno(),
                            ideGetErrorMsg(ideGetErrorCode()));
        }
    }
    idlOS::fprintf(stderr, "%s\n", errbuf);
    idlOS::free(errbuf);
    ideClearError();

    IDE_EXCEPTION_CONT( skip_err_dump );
}

void ideSetClientErrorCode(ideClientErrorMgr     *aErrorMgr,
                           ideClientErrorFactory *aFactory,
                           UInt                   aErrorCode,
                           va_list                aArgs)
{
    static SChar *NULL_MESSAGE           = (SChar *)"No Error Message Loaded";
    static SChar *OUT_BOUND_MESSAGE      = (SChar *)"Out of ErrorCode Bound";
    static SChar *ERR_PARSE_ARG_MESSAGE  = (SChar *)"Invalid Error-Formatted String";
    static SChar *ERR_CONCAT_ARG_MESSAGE = (SChar *)"Invalid Error-Concatenation";

    /* 에러 메시지 구성 준비*/
    UInt         Index;
    ideArgInfo   argInfo[MAX_ARGUMENT];
    Index   =  aErrorCode & E_INDEX_MASK;
    
    aErrorMgr->mErrorCode       = aErrorCode;

    if (1)//Index < Storage->MsbHeader.value_.header.ErrorCount) // 에러 메시지를 구성하자.
    {
        SChar *fmt = (SChar *)aFactory[Index].mErrorMsg;

        if (fmt == NULL)
        {
            idlOS::strcpy(aErrorMgr->mErrorMessage, NULL_MESSAGE);
        }
        else
        {
            if (ideGetArgumentInfo(fmt, &argInfo[0], aArgs) == IDE_SUCCESS)
            {
                /* ------------------------------------------------
                 * Normal Error Code Setting
                 * ----------------------------------------------*/
                
                idlOS::memset(aErrorMgr->mErrorMessage, 0, sizeof(aErrorMgr->mErrorMessage));
                aErrorMgr->mErrorMessage[0] = 0;
                if(ideConcatErrorArg(aErrorMgr->mErrorMessage,
                                     fmt,
                                     &argInfo[0]) != IDE_SUCCESS)
                {
                    idlOS::strcpy(aErrorMgr->mErrorMessage, ERR_CONCAT_ARG_MESSAGE);
                }
                idlOS::strncpy(aErrorMgr->mErrorState,
                               aFactory[Index].mErrorState,
                               ID_SIZEOF(aErrorMgr->mErrorState));
            }
            else
            {
                idlOS::strcpy(aErrorMgr->mErrorMessage, ERR_PARSE_ARG_MESSAGE);
            }
        }
    }
    else
    {
        idlOS::strcpy(aErrorMgr->mErrorMessage, OUT_BOUND_MESSAGE);
    }
}

/* TASK-7218 Handling Multi-Error for SD */
IDE_RC ideErrorCollectionGetNode( iduList **aListNode )
{
    iduListNode             *sListNode = NULL;
    ideErrorCollectionStack *sError    = NULL;
    ideErrorCollection      *sErrorMgr = &(gIdeErrorMgr.mErrors);

    if ( sErrorMgr->mFreeListCnt > 0 )
    {
        sListNode = IDU_LIST_GET_FIRST( &(sErrorMgr->mFreeList) );
        IDU_LIST_REMOVE(sListNode);
        sErrorMgr->mFreeListCnt--;
    }
    else
    {
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_ID_MULTI_ERROR_MANAGER,
                                     sizeof(iduListNode),
                                     (void **)&sListNode)
                  != IDE_SUCCESS );

        IDE_TEST( iduMemMgr::malloc( IDU_MEM_ID_MULTI_ERROR_MANAGER,
                                     sizeof(ideErrorCollectionStack),
                                     (void **)&sError)
                  != IDE_SUCCESS );

        IDU_LIST_INIT_OBJ( sListNode, sError );
    }
    *aListNode = sListNode;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
 * TASK-7218 Multi-Error Handling 2nd
 *   1. mIsAllTheSame 설정
 *   2. Multi-Error용 에러 메시지 누적
 */
static IDE_RC ideErrorCollectionSetMultiError( UInt    aErrorCode,
                                          SChar  *aErrorMessage )
{
    ideErrorCollection  *sErrorMgr = &(gIdeErrorMgr.mErrors);
    SInt                 sBufferSize;

    // 1.
    if ( sErrorMgr->mErrorListCnt == 1 ) // first input
    {
        sErrorMgr->mMultiErrorMsgLen = 0;
        sErrorMgr->mIsAllTheSame = ID_TRUE;
    }
    else
    {
        /* 누적된 에러 코드가 모두 동일한 경우에만 검사하면 된다.
         * 맨 앞 에러 코드를 가져와서 비교한다. */
        if ( ( sErrorMgr->mIsAllTheSame == ID_TRUE ) &&
             ( aErrorCode != ideErrorCollectionFirstErrorCode() ) )
        {
            sErrorMgr->mIsAllTheSame = ID_FALSE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    // 2.
    sBufferSize = MAX_LAST_ERROR_MSG_LEN - sErrorMgr->mMultiErrorMsgLen;
    if ( sBufferSize > 1 )
    {
        sErrorMgr->mMultiErrorMsgLen += idlOS::snprintf(
               sErrorMgr->mMultiErrorMessage + sErrorMgr->mMultiErrorMsgLen,
               sBufferSize,
               (sErrorMgr->mErrorListCnt == 1)? "%s" : "\n%s",
               aErrorMessage );
    }
    else
    {
        /* Nothing to do */
    }
    return IDE_SUCCESS;
}

IDE_RC ideErrorCollectionAdd( UInt    aErrorCode,
                              SChar  *aErrorMessage,
                              UInt    aNodeId )
{
    iduListNode             *sListNode = NULL;
    ideErrorCollectionStack *sError    = NULL;
    ideErrorCollection      *sErrorMgr = &(gIdeErrorMgr.mErrors);

    // Skip to add an error after IDE_POP
    IDE_TEST( sErrorMgr->mFreezeCnt > 0 );

    /* defense code
     *   iduList 변수인 sErrorMgr->mErrorList는 사용전에 초기화 필요.
     *   ideErrorCollectionInit() 함수를 사용해서 초기화가 수행되지만,
     *   혹시라도 새는 데가 있을까봐 방어 코드 삽입.
     */
    if ( sErrorMgr->mErrorListCnt == 0 )
    {
        IDU_LIST_INIT( &(sErrorMgr->mErrorList) );
    }
    if ( sErrorMgr->mFreeListCnt == 0 )
    {
        IDU_LIST_INIT( &(sErrorMgr->mFreeList) );
    }

    IDE_TEST_RAISE( ideErrorCollectionGetNode( &sListNode )
                    != IDE_SUCCESS, InsufficientMemory );

    sError = (ideErrorCollectionStack*) (sListNode->mObj);

    sError->mErrorCode = aErrorCode;
    idlOS::strcpy( sError->mErrorMessage, aErrorMessage );
    sError->mNodeId = aNodeId;

    IDU_LIST_ADD_LAST( &(sErrorMgr->mErrorList), sListNode );

    sErrorMgr->mErrorListCnt++;

    /* TASK-7218 Multi-Error Handling 2nd */
    ideErrorCollectionSetMultiError( aErrorCode, aErrorMessage );

    return IDE_SUCCESS;

    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt ideErrorCollectionFirstErrorCode()
{
    iduList *sListNode  = NULL;
    iduList *sErrorList = &(gIdeErrorMgr.mErrors.mErrorList);

    IDE_TEST( IDU_LIST_IS_EMPTY( sErrorList ) == ID_TRUE );

    sListNode = IDU_LIST_GET_FIRST( sErrorList );
    return ((ideErrorCollectionStack*)(sListNode->mObj))->mErrorCode;

    IDE_EXCEPTION_END;
    return 0;
}

UInt ideErrorCollectionLastErrorCode()
{
    iduList *sListNode  = NULL;
    iduList *sErrorList = &(gIdeErrorMgr.mErrors.mErrorList);

    IDE_TEST( IDU_LIST_IS_EMPTY( sErrorList ) == ID_TRUE );

    sListNode = IDU_LIST_GET_LAST( sErrorList );
    return ((ideErrorCollectionStack*)(sListNode->mObj))->mErrorCode;

    IDE_EXCEPTION_END;
    return 0;
}

SChar* ideErrorCollectionFirstErrorMsg()
{
    iduList *sListNode  = NULL;
    iduList *sErrorList = &(gIdeErrorMgr.mErrors.mErrorList);

    IDE_TEST( IDU_LIST_IS_EMPTY( sErrorList ) == ID_TRUE );

    sListNode = IDU_LIST_GET_FIRST( sErrorList );
    return ((ideErrorCollectionStack*)(sListNode->mObj))->mErrorMessage;

    IDE_EXCEPTION_END;
    return NULL;
}

SChar* ideErrorCollectionLastErrorMsg()
{
    iduList *sListNode  = NULL;
    iduList *sErrorList = &(gIdeErrorMgr.mErrors.mErrorList);

    IDE_TEST( IDU_LIST_IS_EMPTY( sErrorList ) == ID_TRUE );

    sListNode = IDU_LIST_GET_LAST( sErrorList );
    return ((ideErrorCollectionStack*)(sListNode->mObj))->mErrorMessage;

    IDE_EXCEPTION_END;
    return NULL;
}

UInt ideErrorCollectionFirstErrorNodeId()
{
    iduList *sListNode  = NULL;
    iduList *sErrorList = &(gIdeErrorMgr.mErrors.mErrorList);

    IDE_TEST( IDU_LIST_IS_EMPTY( sErrorList ) == ID_TRUE );

    sListNode = IDU_LIST_GET_FIRST( sErrorList );
    return ((ideErrorCollectionStack*)(sListNode->mObj))->mNodeId;

    IDE_EXCEPTION_END;
    return 0;
}

void ideErrorCollectionRemoveFirst()
{
    iduList *sListNode  = NULL;
    iduList *sErrorList = &(gIdeErrorMgr.mErrors.mErrorList);
    iduList *sFreeList  = &(gIdeErrorMgr.mErrors.mFreeList);

    if ( IDU_LIST_IS_EMPTY( sErrorList ) == ID_FALSE )
    {
        sListNode = IDU_LIST_GET_FIRST( sErrorList );

        IDU_LIST_REMOVE( sListNode );
        IDU_LIST_ADD_LAST( sFreeList, sListNode );

        gIdeErrorMgr.mErrors.mErrorListCnt--;
        gIdeErrorMgr.mErrors.mFreeListCnt++;
    }

    return;
}

void ideErrorCollectionRemoveLast()
{
    iduList *sListNode  = NULL;
    iduList *sErrorList = &(gIdeErrorMgr.mErrors.mErrorList);
    iduList *sFreeList  = &(gIdeErrorMgr.mErrors.mFreeList);

    if ( IDU_LIST_IS_EMPTY( sErrorList ) == ID_FALSE )
    {
        sListNode = IDU_LIST_GET_LAST( sErrorList );

        IDU_LIST_REMOVE( sListNode );
        IDU_LIST_ADD_LAST( sFreeList, sListNode );

        gIdeErrorMgr.mErrors.mErrorListCnt--;
        gIdeErrorMgr.mErrors.mFreeListCnt++;
    }
}

SInt ideErrorCollectionSize()
{
    return gIdeErrorMgr.mErrors.mErrorListCnt;
}

void ideErrorCollectionInit()
{
    ideErrorCollection *sErrorMgr = &(gIdeErrorMgr.mErrors);

    sErrorMgr->mErrorListCnt = 0;
    sErrorMgr->mFreeListCnt = 0;
    sErrorMgr->mFreezeCnt = 0;
    sErrorMgr->mIsAllTheSame = ID_TRUE;
    sErrorMgr->mMultiErrorMsgLen = 0;

    IDU_LIST_INIT( &(sErrorMgr->mErrorList) );
    IDU_LIST_INIT( &(sErrorMgr->mFreeList) );
}

void ideErrorCollectionClear()
{
    iduListNode *sIterator = NULL;
    iduListNode *sNextNode = NULL;

    ideErrorCollection *sErrorMgr = &(gIdeErrorMgr.mErrors);
    iduList            *sFreeList = &(sErrorMgr->mFreeList);

    // Defense Code
    if ( sErrorMgr->mErrorListCnt == 0 )
    {
        IDU_LIST_INIT( &(sErrorMgr->mErrorList) );
    }
    if ( sErrorMgr->mFreeListCnt == 0 )
    {
        IDU_LIST_INIT( sFreeList );
    }

    if ( sErrorMgr->mErrorListCnt > 0 )
    {
        IDU_LIST_ITERATE_SAFE( &(sErrorMgr->mErrorList),
                               sIterator,
                               sNextNode )
        {
            IDU_LIST_REMOVE( sIterator );
            IDU_LIST_ADD_LAST( sFreeList, sIterator );
            sErrorMgr->mErrorListCnt--;
            sErrorMgr->mFreeListCnt++;
        }
    }

    sErrorMgr->mIsAllTheSame = ID_TRUE;
    sErrorMgr->mMultiErrorMsgLen = 0;
}

void ideErrorCollectionDestroy()
{
    iduListNode *sIterator = NULL;
    iduListNode *sNextNode = NULL;

    ideErrorCollectionStack *sError;
    ideErrorCollection      *sErrorMgr = &(gIdeErrorMgr.mErrors);

    if ( sErrorMgr->mErrorListCnt > 0 )
    {
        IDU_LIST_ITERATE_SAFE( &(sErrorMgr->mErrorList),
                               sIterator,
                               sNextNode )
        {
            sError = (ideErrorCollectionStack *)sIterator->mObj;
            (void)iduMemMgr::free( (void*)sError );
            IDU_LIST_REMOVE( sIterator );
            (void)iduMemMgr::free( (void*)sIterator );
        }
    }

    if ( sErrorMgr->mFreeListCnt > 0 )
    {
        IDU_LIST_ITERATE_SAFE( &(sErrorMgr->mFreeList),
                               sIterator,
                               sNextNode )
        {
            sError = (ideErrorCollectionStack *)sIterator->mObj;
            (void)iduMemMgr::free( (void*)sError );
            IDU_LIST_REMOVE( sIterator );
            (void)iduMemMgr::free( (void*)sIterator );
        }
    }

    ideErrorCollectionInit();
}

idBool ideErrorCollectionIsAllTheSame()
{
    return gIdeErrorMgr.mErrors.mIsAllTheSame;
}

SChar* ideErrorCollectionMultiErrorMsg()
{
    return gIdeErrorMgr.mErrors.mMultiErrorMessage;
}


/*****************************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 ****************************************************************************/

/*****************************************************************************
 * $Id: genErrMsgManual.cpp 68602 2015-01-23 00:13:11Z sbjang $
 * $Log$
 * Revision 1.7  2005/12/23 08:30:04  hssohn
 * PROJ-1477:Client Error Message Intergration
 *
 * Revision 1.6.6.1  2005/11/29 05:57:26  sjkim
 * added to branch for PROJ-1477
 *
 * Revision 1.6  2005/10/17 02:15:01  sjkim
 * fix for BUG-13101
 *
 * Revision 1.5  2005/09/07 01:16:21  shsuh
 * BUG-12922
 *
 * Revision 1.4  2005/04/18 06:32:44  newdaily
 * PRJ-1347
 *
 * Revision 1.3.28.1  2005/03/17 00:37:15  pu7ha
 * windows porting
 *
 * Revision 1.3  2004/03/30 07:15:04  santamonic
 * modify value
 *
 * Revision 1.2  2003/06/25 10:39:20  sjkim
 * BUG-4718
 *
 * Revision 1.1  2002/12/17 05:43:25  jdlee
 * create
 *
 * Revision 1.2  2002/10/29 10:24:20  jdlee
 * remove sm2 flags
 *
 * Revision 1.1.1.1  2002/10/18 13:55:36  jdlee
 * create
 *
 * Revision 1.15  2002/01/03 05:28:27  jdlee
 * fix
 *
 * Revision 1.14  2002/01/03 04:08:42  jdlee
 * fix
 *
 * Revision 1.13  2001/11/21 04:18:24  sjkim
 * fix
 *
 * Revision 1.12  2001/11/21 04:03:39  sjkim
 * manual util fix for altibase 2.0
 *
 * Revision 1.11  2001/11/01 08:51:02  jdlee
 * fix nomencalture
 *
 * Revision 1.10  2001/08/16 07:05:11  sjkim
 * rebuild error action added
 *
 * Revision 1.9  2001/04/18 07:39:12  sjkim
 * sm2 merge and id layer fix
 *
 * Revision 1.8  2001/03/15 12:22:24  jdlee
 * fix callo argument order
 *
 * Revision 1.7  2001/03/06 10:38:44  jdlee
 * fix
 *
 * Revision 1.6  2001/03/02 07:56:12  kmkim
 * 1st version of WIN32 porting
 *
 * Revision 1.5.6.1  2001/02/21 09:00:06  kmkim
 * win32 porting - 컴파일만 성공
 *
 * Revision 1.5  2001/01/02 06:58:50  sjkim
 * fix
 *
 * Revision 1.4  2000/12/29 09:21:00  sjkim
 * fix
 *
 * Revision 1.3  2000/12/29 04:45:52  sjkim
 * fix
 *
 * Revision 1.2  2000/12/04 06:32:18  sjkim
 * fix
 *
 * Revision 1.1  2000/12/01 04:59:59  sjkim
 * Manual fix
 *
 * Revision 1.7  2000/10/11 02:10:07  sjkim
 * idlOS::추가
 *
 * Revision 1.6  2000/10/04 02:13:39  sjkim
 * genErrMgr fix
 *
 * Revision 1.5  2000/09/30 06:57:30  sjkim
 * error code fix
 *
 * Revision 1.4  2000/06/16 02:50:58  jdlee
 * fix naming
 *
 * Revision 1.3  2000/06/15 03:57:31  assam
 * ideErrorMgr changed.
 *
 * Revision 1.2  2000/06/14 00:53:46  jdlee
 * fix compile warning
 *
 * Revision 1.1  2000/06/07 13:19:26  jdlee
 * fix
 *
 * Revision 1.1.1.1  2000/06/07 11:44:12  jdlee
 * init
 *
 * Revision 1.5  2000/05/31 11:14:42  jdlee
 * change naming to id
 *
 * Revision 1.4  2000/05/23 01:14:22  sjkim
 * section 수정
 *
 * Revision 1.3  2000/05/22 10:12:24  sjkim
 * 에러 처리 루틴 수정 : ACTION 추가
 *
 * Revision 1.2  2000/03/14 05:30:25  sjkim
 * 에러 코드 화일 include 금지
 *
 * Revision 1.1  2000/03/14 05:23:18  sjkim
 * 에러 메시지 처리 루틴 추가
 *
 *
 * DESC : 에러코드 원시 스크립트 화일을 입력 받아 에러코드 semi-h 헤더화일
 *        및 에러메시지 데이타화일 생성
 *
 ****************************************************************************/

// 에러 메시지를 생성할 때 에러코드 화일을 include 하지 않는다.
#define ID_ERROR_CODE_H

#include <idl.h>
#include <idn.h>
#include <ideErrorMgr.h>

#define E_ACTION_COMMENT 9999


// MSB 화일에 에러코드를 적는다.
//#define MSB_ECODE_ON



UInt gClientPart   = 0;

/* ----------------------------------------------------------------------
 *
 * 변수 정의
 *
 * ----------------------------------------------------------------------*/

extern ideErrTypeInfo typeInfo[];

idErrorMsbType msbHeader;

FILE *ifp;  // 입력 화일

SChar *inFilename;

SChar *Usage =
(SChar *)"Usage :  "PRODUCT_PREFIX"genErrMsgManual [-n] -i INFILENAME[.msg]\n\n"
"         -n : generatate Error Number\n"
"         -i : specify ErrorCode and Message input file [.msg]\n"
;

SInt Section     = -1;    // 영역 번호
SInt errIdxNum      =  0;
SInt PrintNumber =  1; // 에러값을 기입한다.
UInt HexErrorCode;

SChar *Header =
(SChar *)"/*   This File was created automatically by "PRODUCT_PREFIX"genErrMsg Utility */\n\n";

SChar *StringError =
(SChar *)" 에러 코드는 다음과 같이 구성되어야 합니다.\n"
"\n"
"      SubCode, [MODULE]ERR_[ACTION]_[NamingSpace]_[Description]\n"
"          1           2          3            4\n"
"\n"
"  [MODULE] :  sm | qp | cm | ...\n"
"  [ACTION] :  FATAL | ABORT | IGNORE\n"
"  [NamingSpace] : qex | qpf | smd | smc ...\n"
"  [Description] : FileWriteError | ....\n\n"
"   Ex) idERR_FATAL_FATAL_smd_InvalidOidAddress \n\n";
/* ----------------------------------------------------------------------
 *
 * 스트링 처리 함수
 *
 * ----------------------------------------------------------------------*/

#define MAX_ERR_STRING  512
/* ----------------------------------------------------------------------
 * BUG-25815 genErrMsgManual 이 core 로 죽습니다. 
 * MAX_ERR_NUM이 512이면 에러 메시지 개수보다 배열이 작아져서 동작중에
 * 죽는 문제가 있습니다.
 * MAX_ERR_NUM을 4096으로 설정하면 메모리를 1G를 차지하여
 * 메모리가 적은 서버에서 부하가 큽니다.
 * ----------------------------------------------------------------------*/
// #define MAX_ERR_NUM     4096
#define MAX_ERR_NUM     1024
#define MAX_COMMENT     100

//SChar ErrorArray[3][MAX_ERR_NUM][MAX_ERR_STRING];
struct ErrorInfo
{
    SInt  CommentCount;
    SChar Message[MAX_ERR_STRING];
    SChar Comment[MAX_COMMENT][MAX_ERR_STRING];
};

ErrorInfo *fatalErrorInfo;
ErrorInfo *abortErrorInfo;
ErrorInfo *ignoreErrorInfo;
ErrorInfo *retryErrorInfo;
ErrorInfo *rebuildErrorInfo;

SInt  abortCount  = 0;
SInt  ignoreCount = 0;
SInt  fatalCount  = 0;
SInt  retryCount  = 0;
SInt  rebuildCount  = 0;

/* 스트링 앞뒤에 존재하는 WHITE-SPACE를 제거 */
static void eraseWhiteSpace(SChar *buffer)
{
    SInt i;
    SInt len = idlOS::strlen(buffer);
    SInt ValueRegion = 0; // 현재 수행하는 범위는 ? 이름영역=값영역
    SInt firstAscii  = 0; // 값영역에서 첫번째 ASCII를 찾은 후..

    // 1. 앞에서 부터 검사 시작..
    for (i = 0; i < len && buffer[i]; i++)
    {
        if (buffer[i] == '#') // 주석 처리
        {
            buffer[i]= 0;
            break;
        }
        if (ValueRegion == 0) // 이름 영역 검사
        {
            if (buffer[i] == '=')
            {
                ValueRegion = 1;
                continue;
            }

            if (idlOS::idlOS_isspace(buffer[i])) // 스페이스 임
            {
                SInt j;

                for (j = i;  buffer[j]; j++)
                {
                    buffer[j] = buffer[j + 1];
                }
                i--;
            }
        }
        else // 값영역 검사
        {
            if (firstAscii == 0)
            {
                if (idlOS::idlOS_isspace(buffer[i])) // 스페이스 임
                {
                    SInt j;

                    for (j = i;  buffer[j]; j++)
                    {
                        buffer[j] = buffer[j + 1];
                    }
                    i--;
                }
                else
                {
                    break;
                }
            }

        }
    } // for

    // 2. 끝에서 부터 검사 시작.. : 스페이스 없애기
    len = idlOS::strlen(buffer);
    for (i = len - 1; buffer[i] && len > 0; i--)
    {
        if (idlOS::idlOS_isspace(buffer[i])) // 스페이스 없애기
        {
            buffer[i]= 0;
            continue;
        }
        break;
    }
}

// 이름과 값을 얻어옴
SInt parseBuffer(SChar *buffer,
                 SInt  *SubCode,
                 SChar **State,
                 SChar **Name,
                 SChar **Value)
{
    SInt i;
    SChar SubCodeBuffer[128];
    SChar *buf;

    *SubCode = -1;

    if (buffer[0] == '#')
    {
        *Name = (SChar *)"COMMENT";
        *Value = idlOS::strdup(buffer + 1);

        return IDE_SUCCESS;
    }

    /* ------------------------
     * [1] White Space 제거
     * ----------------------*/
    eraseWhiteSpace(buffer);

    /* ---------------------------------
     * [2] 내용이 없거나 주석이면 무시
     * -------------------------------*/
    SInt len = idlOS::strlen(buffer);

    if (len == 0)
    {
        return IDE_SUCCESS;
    }


    /* -------------------------
     * [3] SubCode 값 얻기
     * ------------------------*/
    buf = SubCodeBuffer;
    idlOS::memset(SubCodeBuffer, 0, 128);
    for (i = 0; ; i++)
    {
        SChar c;
        c = buf[i] = buffer[i];
        if (c == ',') // , 출현
        {
            *SubCode = (SInt)idlOS::strtol(SubCodeBuffer, NULL, 10);
            buffer += i;
            buffer ++; // [,] 건너뛰기

            break;
        }
        else
        if (c == 0)
        {
            break;
        }
    }

    /* -------------------------
     * [4] Client의 경우 State  얻기
     * ------------------------*/
    if (gClientPart == 1)
    {
        eraseWhiteSpace(buffer);
        buf = buffer;
        idlOS::memset(SubCodeBuffer, 0, 128);
        for (i = 0; ; i++)
        {
            SChar c;
            c = buf[i] = buffer[i];
            if (c == ',') // , 출현
            {
                *State = buf;
                buffer += i;
                *buffer = 0; //[,] 제거 
                buffer ++; // [,] 건너뛰기
                break;
            }
            else
            {
                if (c == 0)
                {
                    break;
                }
            }
        }
    }
    
    /* --------------------------
     * [4] Name = Value 값 얻기
     * -------------------------*/
    eraseWhiteSpace(buffer);
    *Name = buffer; // 이름을 결정
    for (i = 0; i < len; i++)
    {
        if (buffer[i] == '=')
        {
            // 구분자가 존재하면,
            buffer[i] = 0;

            if (buffer[i + 1])
            {
                *Value = &buffer[i + 1];

                if (idlOS::strlen(&buffer[i + 1]) > 512)
                {
                    // 쓰레기값이 있음..
                    return IDE_FAILURE;
                }
            }
            return IDE_SUCCESS;
        }
    }

    return IDE_SUCCESS;
}

void ErrorOut(SChar *msg, SInt line, SChar *buffer)
{
    idlOS::printf("\n 파싱에러 :: %s(%s:%d)\n\n",
                  msg, buffer, line);

    idlOS::exit(0);
}

void eraseOutputFile()
{
}

UInt getAction(SInt line, SChar *Name)
{
    SChar *p = idlOS::strstr(Name, "ERR_");
    if (p)
    {
        p += 4; // skip ERR_

        if (idlOS::strncasecmp(p, "FATAL", 5) == 0)
        {
            return E_ACTION_FATAL;
        }

        if (idlOS::strncasecmp(p, "ABORT", 5) == 0)
        {
            return E_ACTION_ABORT;
        }

        if (idlOS::strncasecmp(p, "IGNORE", 5) == 0)
        {
            return E_ACTION_IGNORE;
        }

        if (idlOS::strncasecmp(p, "RETRY", 5) == 0)
        {
            return E_ACTION_RETRY;
        }

        if (idlOS::strncasecmp(p, "REBUILD", 7) == 0)
        {
            return E_ACTION_RETRY;
        }

    }
    else
    {
        return E_ACTION_COMMENT;
    }
    idlOS::printf("[%d:%s] 에러코드의 ACTION 영역에서 에러가 발생하였습니다.\n%s", line, Name, StringError);
    eraseOutputFile();
    idlOS::exit(0);
}

/* ----------------------------------------------------------------------
 *
 *  main()
 *
 * ----------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
    int opr;
    SInt count, count2;
    UInt oldAction = 99;
    SChar buffer[1024];
    SChar dupBuf[E_INDEX_MASK];
    SInt  line = 1;


    // 옵션을 받는다.
    while ( (opr = idlOS::getopt(argc, argv, "ni:o:c")) != EOF)
    {
        switch(opr)
        {
        case 'i':
            //idlOS::printf(" i occurred arg [%s]\n", optarg);
            inFilename = optarg;
            break;
        case 'n':
            PrintNumber = 1;
            break;
        case 'c':
            // Client Part Error Message Generation
            gClientPart = 1;
            break;
        }
    }

    // 메시지 출력
    if (inFilename == NULL)
    {
        idlOS::printf(Usage);
        idlOS::exit(0);
    }


    idlOS::umask(0);
    // 이름이 정확한지 검사한다.

    SInt len = idlOS::strlen(inFilename);
    if ( idlOS::strstr(inFilename, ".msg") == NULL) // 확장자가 없이 입력
    {
        SChar *tmp;
        // .msg가 붙은 확장자 화일로 생성한다.
        tmp = (SChar *)idlOS::calloc(1, len + 5);
        idlOS::snprintf(tmp, len + 5, "%s.msg", inFilename);
        inFilename = tmp; // inFilename에는 *.msg가 붙음
    }
    else
    {
    }

    fatalErrorInfo   = (ErrorInfo *)idlOS::calloc(MAX_ERR_NUM, sizeof(ErrorInfo)  );
    IDE_TEST_RAISE(fatalErrorInfo == NULL, memory_error);

    abortErrorInfo   = (ErrorInfo *)idlOS::calloc(MAX_ERR_NUM, sizeof(ErrorInfo)   );
    IDE_TEST_RAISE(abortErrorInfo == NULL, memory_error);

    ignoreErrorInfo  = (ErrorInfo *)idlOS::calloc(MAX_ERR_NUM, sizeof(ErrorInfo)  );
    IDE_TEST_RAISE(ignoreErrorInfo == NULL, memory_error);

    retryErrorInfo   = (ErrorInfo *)idlOS::calloc(MAX_ERR_NUM, sizeof(ErrorInfo)  );
    IDE_TEST_RAISE(retryErrorInfo == NULL, memory_error);

    rebuildErrorInfo = (ErrorInfo *)idlOS::calloc(MAX_ERR_NUM, sizeof(ErrorInfo)  );
    IDE_TEST_RAISE(rebuildErrorInfo == NULL, memory_error);

    ifp  = idlOS::fopen(inFilename, "r");
    if (ifp == NULL)
    {
        idlOS::printf("Can't open file %s\n", inFilename);
        idlOS::exit(0);
    }

    idlOS::memset(dupBuf, 0, sizeof(dupBuf));

    while(!feof(ifp))
    {
        SInt  SubCode  = 0;
        SChar *State   = NULL;
        SChar *Name    = NULL;
        SChar *Value   = NULL;

        idlOS::memset(buffer, 0, 1024);
        if (idlOS::fgets(buffer, 1024, ifp) == NULL)
        {
            // 화일의 끝까지 읽음
            break;
        }

        // buffer에 한줄의 정보가 있음
        if (parseBuffer(buffer, &SubCode, &State, &Name, &Value) == -1)
        {
            idlOS::fclose(ifp);
            idlOS::printf("에러... 라인 [%d]:%s\n", line, buffer);
            idlOS::exit(0);
        }


        if (Section == -1) // Section 설정되지 않았을 경우
        {
            if (Name)
            {
                if (idlOS::strcasecmp(Name, "SECTION") == 0)
                {
                    Section = (SInt)idlOS::strtol(Value, NULL, 10);
                    if (Section < 0 || Section > 15)
                        ErrorOut((SChar *)"Section의 범위가 틀렸습니다.",
                                 line, buffer);
                }
            }
        }
        else // Section이 설정되었음 : 여기부터는 정확한 N=V 요구
        {
            if ( (Name && Value == NULL) ||
                 (Name == NULL && Value))
            {
                if ((Name != NULL))
                {
                    if (idlOS::strcmp(Name, "INTERNAL_TRACE_MESSAGE_BEGIN") == 0)
                    {
                        // Internal Trace Message Generation - Skip to end.
                        break;
                    }
                }

                ErrorOut((SChar *)"에러코드와 에러메시지가 동시에 존재하지 않습니다.",
                         line, buffer);
            }
            else
            {

                if (Name && Value) // 값이 설정됨
                {
                    SInt isComment = 0;
                    UInt Action    = getAction(line, Name);

                    if (Action == E_ACTION_COMMENT)
                    {
                        Action = oldAction;
                    }
                    else
                    {
                        oldAction = Action;
                    }

                    if (idlOS::strcmp(Name, "COMMENT") == 0)
                    {
                        isComment = 1;
                    }

                    HexErrorCode =
                        ((UInt)Section << 28) | (UInt)Action | ((UInt)SubCode << 12) | (UInt)errIdxNum;

                    switch(Action)
                    {
                    case E_ACTION_RETRY: // fatal
                        if (isComment == 1)
                        {
                            ErrorInfo *cur = &retryErrorInfo[retryCount - 1];
                            idlOS::snprintf(cur->Comment[cur->CommentCount], MAX_ERR_STRING,
                                            "%s", Value);
                            cur->CommentCount++;
                        }
                        else
                        {
                            idlOS::snprintf(retryErrorInfo[retryCount].Message, MAX_ERR_STRING,
                                           "0x%05X (%7"ID_UINT32_FMT") %s %s \n",
                                           E_ERROR_CODE(HexErrorCode),E_ERROR_CODE(HexErrorCode), Name, Value);
                            retryCount++;
                        }
                        break;
                    case E_ACTION_REBUILD: // REBUILD
                        if (isComment == 1)
                        {
                            ErrorInfo *cur = &rebuildErrorInfo[rebuildCount - 1];
                            idlOS::snprintf(cur->Comment[cur->CommentCount], MAX_ERR_STRING,
                                            "%s", Value);
                            cur->CommentCount++;
                        }
                        else
                        {
                            idlOS::snprintf(rebuildErrorInfo[rebuildCount].Message, MAX_ERR_STRING,
                                           "0x%05X (%7"ID_UINT32_FMT") %s %s \n",
                                           E_ERROR_CODE(HexErrorCode),E_ERROR_CODE(HexErrorCode), Name, Value);
                            rebuildCount++;
                        }
                        break;
                    case E_ACTION_FATAL: // fatal
                        if (isComment == 1)
                        {
                            ErrorInfo *cur = &fatalErrorInfo[fatalCount - 1];
                            idlOS::snprintf(cur->Comment[cur->CommentCount], MAX_ERR_STRING,
                                            "%s", Value);
                            cur->CommentCount++;
                        }
                        else
                        {
                            idlOS::snprintf(fatalErrorInfo[fatalCount].Message, MAX_ERR_STRING,
                                           "0x%05X (%7"ID_UINT32_FMT") %s %s \n",
                                           E_ERROR_CODE(HexErrorCode),E_ERROR_CODE(HexErrorCode), Name, Value);
                            fatalCount++;
                        }
                        break;
                    case E_ACTION_ABORT: // abort
                        if (isComment == 1)
                        {
                            ErrorInfo *cur = &abortErrorInfo[abortCount - 1];
                            idlOS::snprintf(cur->Comment[cur->CommentCount], MAX_ERR_STRING,
                                            "%s", Value);
                            cur->CommentCount++;
                        }
                        else
                        {
                            idlOS::snprintf(abortErrorInfo[abortCount].Message, MAX_ERR_STRING,
                                           "0x%05X (%7"ID_UINT32_FMT") %s %s \n",
                                           E_ERROR_CODE(HexErrorCode),E_ERROR_CODE(HexErrorCode), Name, Value);
                            abortCount++;
                        }
                        break;
                    case E_ACTION_IGNORE: // ignore
                        if (isComment == 1)
                        {
                            ErrorInfo *cur = &ignoreErrorInfo[ignoreCount - 1];
                            idlOS::snprintf(cur->Comment[cur->CommentCount], MAX_ERR_STRING,
                                            "%s", Value);
                            cur->CommentCount++;
                        }
                        else
                        {
                            idlOS::snprintf(ignoreErrorInfo[ignoreCount].Message, MAX_ERR_STRING,
                                           "0x%05X (%7"ID_UINT32_FMT") %s %s \n",
                                           E_ERROR_CODE(HexErrorCode),E_ERROR_CODE(HexErrorCode), Name, Value);
                            ignoreCount++;
                        }
                        break;
                    default:
                        if (oldAction != 99)
                        {
                            idlOS::fprintf(stderr, " Action Error");
                            idlOS::exit(-1); // 죽어랏!
                        }
                        break;
                    }
                    errIdxNum++;
                }
            }
        }

        line++;
    }
//SChar ErrorArray[3][MAX_ERR_NUM][MAX_ERR_STRING];
/*
struct ErrorInfo
{
    SInt  CommentCount;
    SChar Message[MAX_ERR_STRING];
    SChar Comment[MAX_COMMENT][MAX_ERR_STRING];
};

ErrorInfo fatalErrorInfo[MAX_ERR_NUM];
ErrorInfo abortErrorInfo[MAX_ERR_NUM];
ErrorInfo ignoreErrorInfo[MAX_ERR_NUM];

SInt  abortCount  = 0;
SInt  ignoreCount = 0;
SInt  fatalCount  = 0;
*/

    idlOS::printf("\n--FATAL--\n");
    for (count = 0; count < fatalCount; count++)
    {
        ErrorInfo *cur = &fatalErrorInfo[count];

        if (cur->Message[0] == 0) break;
        idlOS::printf("%s", cur->Message);

        for (count2 = 0; count2 < cur->CommentCount; count2++)
        {
            if (idlOS::strncmp(cur->Comment[count2], "##", 2) == 0)
            {
                continue;
            }
            idlOS::printf("#%s", cur->Comment[count2]);
        }
        idlOS::printf("\n");
    }

    idlOS::printf("\n\n--ABORT--\n");
    for (count = 0; count < abortCount; count++)
    {
        ErrorInfo *cur = &abortErrorInfo[count];

        if (cur->Message[0] == 0) break;
        idlOS::printf("%s", cur->Message);

        for (count2 = 0; count2 < cur->CommentCount; count2++)
        {
            if (idlOS::strncmp(cur->Comment[count2], "##", 2) == 0)
            {
                continue;
            }
            idlOS::printf("#%s", cur->Comment[count2]);
        }
        idlOS::printf("\n");
    }

    idlOS::printf("\n\n--IGNORE--\n");
    for (count = 0; count < ignoreCount; count++)
    {
        ErrorInfo *cur = &ignoreErrorInfo[count];

        if (cur->Message[0] == 0) break;
        idlOS::printf("%s", cur->Message);

        for (count2 = 0; count2 < cur->CommentCount; count2++)
        {
            if (idlOS::strncmp(cur->Comment[count2], "##", 2) == 0)
            {
                continue;
            }
            idlOS::printf("#%s", cur->Comment[count2]);
        }
        idlOS::printf("\n");
    }

    idlOS::printf("\n\n--RETRY--\n");
    for (count = 0; count < retryCount; count++)
    {
        ErrorInfo *cur = &retryErrorInfo[count];

        if (cur->Message[0] == 0) break;
        idlOS::printf("%s", cur->Message);

        for (count2 = 0; count2 < cur->CommentCount; count2++)
        {
            if (idlOS::strncmp(cur->Comment[count2], "##", 2) == 0)
            {
                continue;
            }
            idlOS::printf("#%s", cur->Comment[count2]);
        }
        idlOS::printf("\n");
    }

    idlOS::printf("\n\n--REBUILD--\n");
    for (count = 0; count < rebuildCount; count++)
    {
        ErrorInfo *cur = &rebuildErrorInfo[count];

        if (cur->Message[0] == 0) break;
        idlOS::printf("%s", cur->Message);

        for (count2 = 0; count2 < cur->CommentCount; count2++)
        {
            if (idlOS::strncmp(cur->Comment[count2], "##", 2) == 0)
            {
                continue;
            }
            idlOS::printf("#%s", cur->Comment[count2]);
        }
        idlOS::printf("\n");
    }

    msbHeader.value_.header.ErrorCount = idlOS::ntoh(errIdxNum);
    msbHeader.value_.header.Section    = idlOS::ntoh((UInt)Section);

    idlOS::fclose(ifp);
    //idlOS::close(obfp);
    return 0;
    IDE_EXCEPTION(memory_error);
    fprintf(stderr, "Error in Memory Allocation. \n");
    IDE_EXCEPTION_END;
    return -1;
}


/*****************************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 ****************************************************************************/

/*****************************************************************************
 * $Id: genErrMsg.cpp 71017 2015-05-28 07:54:48Z sunyoung $
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
#include <iduVersion.h>

// MSB 화일에 에러코드를 적는다.
//#define MSB_ECODE_ON
/*
 *                      (ALTIBASE 4.0)
 *
 *         0   1   2   3   4   5   6    7    8    9
 *        id  sm  mt  qp  mm  ul  rp  none none  ut
 */


/* ----------------------------------------------------------------------
 *
 * 변수 정의
 *
 * ----------------------------------------------------------------------*/

extern ideErrTypeInfo typeInfo[];
void   doTraceGeneration();

idErrorMsbType msbHeader;

FILE *inMsgFP;  // 입력 화일

FILE *outErrHeaderFP;
FILE *outTrcHeaderFP;
FILE *outTrcCppFP;
FILE *outMsbFP;

SChar *inMsgFile = NULL;

SChar *outErrHeaderFile = NULL;
SChar  outMsbFile[256];
SChar  outTrcCppFile[256];
SChar  outTrcHeaderFile[256];



// Server의 경우 MSB, Clinet의 경우 C 소스코드로 생성.
const SChar *gExtName[] =
{
    "msb", "c"
};



SChar *Usage =
(SChar *)"Usage :  "PRODUCT_PREFIX"genErrMsg [-n] -i INFILENAME -o HeaderFile\n\n"
"         -n : generatate Error Number\n"
"         -i : specify ErrorCode and Message input file [.msg]\n"
"         -o : specify to store Error Code Header File \n"
"         Caution!!) INFILENAME have to follow \n"
"                    the NLS Naming convention. (refer idnLocal.h) \n"
"                    ex) (o) E_SM_US7ASCII.msg, (o) E_QP_US7ASCII.msg, \n"
"                        (o) E_CM_US7ASCII.msg  (x) E_SM_ASCII.msg \n\n"
;

SInt Section       = -1;    // 영역 번호
SInt StartNum      = -1;
SInt errIdxNum     =  0;
SInt PrintNumber   =  1; // 에러값을 기입한다.
UInt HexErrorCode;
UInt gClientPart   = 0;
UInt gDebug        = 0;
UInt gMsgOnly      = 0; /* BUG-41330 */

SChar *Header =
(SChar *)"/*   This File was created automatically by "PRODUCT_PREFIX"genErrMsg Utility */\n\n";

SChar *StringError =
(SChar *)" 에러 코드는 다음과 같이 구성되어야 합니다.\n"
"\n"
"      SubCode, [MODULE]ERR_[ACTION]_[NamingSpace]_[Description]\n"
"          1           2          3            4\n"
"\n"
"  [MODULE] :  sm | qp | cm | ...\n"
"  [ACTION] :  FATAL | ABORT | IGNORE | RETRY | REBUILD\n"
"  [NamingSpace] : qex | qpf | smd | smc ...\n"
"  [Description] : FileWriteError | ....\n\n"
"   Ex) idERR_FATAL_FATAL_smd_InvalidOidAddress \n\n";
/* ----------------------------------------------------------------------
 *
 * 스트링 처리 함수
 *
 * ----------------------------------------------------------------------*/

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
                 SChar **Value,
                 UInt  aParseSubCode = 1)
{
    SInt i;
    SChar SubCodeBuffer[128];
    SChar *buf;

    *SubCode = -1;

    /* ------------------------
     * [1] White Space 제거
     * ----------------------*/
    eraseWhiteSpace(buffer);

    /* ---------------------------------
     * [2] 내용이 없거나 주석이면 무시
     * -------------------------------*/
    SInt len = idlOS::strlen(buffer);
    if (len == 0 || buffer[0] == '#')
    {
        return IDE_SUCCESS;
    }


    /* -------------------------
     * [3] SubCode 값 얻기
     * ------------------------*/
    if (aParseSubCode == 1)
    {
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
    }

    /* -------------------------
     * [4] Client의 경우 State  얻기
     * ------------------------*/
    if ( (gClientPart == 1) &&
         (gMsgOnly == 0) )
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
     * [5] Name = Value 값 얻기
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
    idlOS::fclose(outErrHeaderFP);
    idlOS::fclose(outMsbFP);
    idlOS::unlink(outErrHeaderFile);
    idlOS::unlink(outMsbFile);
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

        if (idlOS::strncasecmp(p, "IGNORE", 6) == 0)
        {
            return E_ACTION_IGNORE;
        }

        if (idlOS::strncasecmp(p, "RETRY", 5) == 0)
        {
            return E_ACTION_RETRY;
        }

        if (idlOS::strncasecmp(p, "REBUILD", 7) == 0)
        {
            return E_ACTION_REBUILD;
        }

    }
    idlOS::printf("[%d:%s] 에러코드의 ACTION 영역에서 에러가 발생하였습니다.\n%s", line, Name, StringError);
    eraseOutputFile();
    idlOS::exit(0);
    return 0;
}

void CheckValidation(SInt line, SChar *Name, SChar *Value)
{
    SInt  i, under_score = 0;
    SChar *fmt;

    /* ----------------------------------------------
     * [1] Name Check :
     *     [sm|id|qpERR_[Abort|Fatal|Ignore]_[....]
     * --------------------------------------------*/
    for (i = 0; Name[i]; i++)
    {
        if (Name[i] == '_')
            under_score++;
    }

    if (under_score < 2)
    {
        idlOS::printf("[%d:%s] 에러코드를 분석하는데 에러가 발생하였습니다.\n%s",
                      line,
                      Name,
                      StringError);
        eraseOutputFile();
        idlOS::exit(0);
    }

    /* ----------------------------------------------
     * [2] Value Check
     *     String [<][0-9][%][udldudsc][>] 가 존재
     * --------------------------------------------*/
    fmt = Value;
    SChar c;
    SChar ArgumentFlag[MAX_ARGUMENT]; // 최대 가변인자 갯수
    SInt  argNum;
    SInt  argCount = 0;

    idlOS::memset(ArgumentFlag, 0, sizeof(SChar) * MAX_ARGUMENT);

    while(( c = *fmt++) )
    {
        SChar numBuf[8]; // 인자번호 입력

        if (c == '<') // [<] 출현
        {
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
            argNum = (UInt)idlOS::strtol(numBuf, NULL, 10);
            if (argNum >= MAX_ARGUMENT)
            {
                idlOS::printf("[%d:%s] 에러코드메시지의 가변인자 리스트 값이 너무 큽니다. 최대 %d.\n",
                              line,
                              Name,
                              MAX_ARGUMENT);
                eraseOutputFile();
                idlOS::exit(0);
            }
            ArgumentFlag[argNum] = 1;

            /* ------------------
             * [2] 인자타입 검사
             * -----------------*/
            for (i = 0; ; i++)
            {
                if (typeInfo[i].type == IDE_ERR_NONE)
                {
                    idlOS::printf("[%d:%s] 가변인자 데이타 타입이 적절치 않습니다. %s.\n",
                                  line,
                                  Name,
                                  fmt);
                    eraseOutputFile();
                    idlOS::exit(0);
                }
                if (idlOS::strncmp(fmt,
                                   typeInfo[i].tmpSpecStr,
                                   typeInfo[i].len) == 0)
                {
                    fmt += typeInfo[i].len;
                    break;
                }
            }
            if ( *fmt != '>')
            {
                idlOS::printf("[%d:%s] > 표시를 찾을 수 없습니다. [%s].\n",
                              line,
                              Name,
                              fmt);
                eraseOutputFile();
                idlOS::exit(0);
            }
            fmt++;
            argCount++;
        }
    }

    if (argCount >= MAX_ARGUMENT)
    {
        idlOS::printf("[%d:%s] 에러코드메시지의 가변인자의 갯수가 %d를 초과했습니다.\n",
                      line,
                      Name,
                      MAX_ARGUMENT);
        eraseOutputFile();
        idlOS::exit(0);
    }
    for (i = 0; i < argCount; i++)
    {
        if (ArgumentFlag[i] == 0)
        {
            idlOS::printf("[%d:%s] 가변인자 리스트의 일련번호중 빠진 것이 있습니다. 순서대로 기입되었는지 확인하십시요. [%d]의 번호가 없음.\n",
                          line,
                          Name,
                          i);
            eraseOutputFile();
            idlOS::exit(0);
        }
    }


}

/* ----------------------------------------------------------------------
 *
 *  main()
 *
 * ----------------------------------------------------------------------*/


int main(int argc, char *argv[])
{
    int opr;

//     fprintf(stderr, "abd\ \qabd\n");
//     exit(0);

    // 옵션을 받는다.
    while ( (opr = idlOS::getopt(argc, argv, "djafvni:o:ctkm")) != EOF)
    {
        switch(opr)
        {
        case 'd':
            gDebug= 1;
            break;
        case 'i':
            //idlOS::printf(" i occurred arg [%s]\n", optarg);
            inMsgFile = optarg;
            break;
        case 'o':
            //idlOS::printf(" o occurred arg [%s]\n", optarg);
            outErrHeaderFile = optarg;
            break;
        case 'n':
            PrintNumber = 1;
            break;
        case 'v':
            idlOS::fprintf(stdout, "version %s %s %s \n",
                iduVersionString,
                iduGetSystemInfoString(),
                iduGetProductionTimeString());
            idlOS::fflush(stdout);
            idlOS::exit(0); /* just exit after print */
        /* BUG-34010 4자리 숫자로 된 버전 정보 필요 */
        case 'a':
            idlOS::fprintf(stdout, "%d.%d.%d.%d", 
                    IDU_ALTIBASE_MAJOR_VERSION,
                    IDU_ALTIBASE_MINOR_VERSION,
                    IDU_ALTIBASE_DEV_VERSION,
                    IDU_ALTIBASE_PATCHSET_LEVEL);
            idlOS::fflush(stdout);
            idlOS::exit(0); /* just exit after print */
        case 'f':
            idlOS::fprintf(stdout, "%d.%d.%d.%d.%d", 
                    IDU_ALTIBASE_MAJOR_VERSION,
                    IDU_ALTIBASE_MINOR_VERSION,
                    IDU_ALTIBASE_DEV_VERSION,
                    IDU_ALTIBASE_PATCHSET_LEVEL,
                    IDU_ALTIBASE_PATCH_LEVEL);
            idlOS::fflush(stdout);
            idlOS::exit(0); /* just exit after print */
        case 'j':
            idlOS::fprintf(stdout, IDU_ALTIBASE_VERSION_STRING);
            idlOS::fflush(stdout);
            idlOS::exit(0); /* just exit after print */
        case 't':
            idlOS::fprintf(stdout,iduGetProductionTimeString());
            idlOS::fflush(stdout);
            idlOS::exit(0); /* just exit after print */
        case 'k': /* package string */
            idlOS::fprintf(stdout, iduGetPackageInfoString());
            idlOS::fflush(stdout);
            idlOS::exit(0); /* just exit after print */
        case 'c':
            // Client Part Error Message Generation
            gClientPart = 1;
            break;
        case 'm': 
            gMsgOnly = 1;
            break;
        }
    }

    // 메시지 출력
    if (inMsgFile == NULL || outErrHeaderFile == NULL)
    {
        idlOS::fprintf(stdout, Usage);
        idlOS::fflush(stdout);
        idlOS::exit(0);
    }


    idlOS::umask(0);

    /* ------------------------------------------------
     *  생성될 화일명을 지정한다.
     * ----------------------------------------------*/

    SInt len = idlOS::strlen(inMsgFile);
    if ( idlOS::strstr(inMsgFile, ".msg") == NULL) // 확장자가 없이 입력
    {
        idlOS::fprintf(stderr, "you should specify the input msg file \n");
        idlOS::exit(-1);
    }
    else
    {
        /* ------------------------------------------------
         *  [MSB/CPP] 화일명 생성
         * ----------------------------------------------*/

        idlOS::strcpy(outMsbFile, inMsgFile);
        outMsbFile[len - 3] = 0;
        idlOS::strcat(outMsbFile, gExtName[gClientPart]);

        /* ------------------------------------------------
         *  Trace Log용 C 소스코드 생성
         *
         *  inpurt E_ID_XXXXX.msg => TRC_ID_STRING.ic
         *                        => TRC_ID_STRING.ih
         * ----------------------------------------------*/

        idlOS::snprintf(outTrcCppFile,
                        ID_SIZEOF(outTrcCppFile),
                        "%c%c_TRC_CODE.ic",
                        inMsgFile[2],
                        inMsgFile[3]);
        if (gDebug != 0)
        {
            fprintf(stderr, "TRC=>[%s]\n", outTrcCppFile);
        }


        idlOS::snprintf(outTrcHeaderFile,
                        ID_SIZEOF(outTrcCppFile),
                        "%c%c_TRC_CODE.ih",
                        inMsgFile[2],
                        inMsgFile[3]);

        if (gDebug != 0)
        {
            fprintf(stderr, "TRC=>[%s]\n", outTrcHeaderFile);
        }
    }


    inMsgFP  = idlOS::fopen(inMsgFile, "r");
    if (inMsgFP == NULL)
    {
        idlOS::printf("Can't open file %s\n", inMsgFile);
        idlOS::exit(0);
    }

    /* ------------------------------------------------
     *  화일 Open
     * ----------------------------------------------*/

    outErrHeaderFP = idlOS::fopen(outErrHeaderFile, "wb");
    if (outErrHeaderFP == NULL)
    {
        idlOS::printf("Can't create file %s\n", outErrHeaderFile);
        idlOS::exit(0);
    }

    outTrcHeaderFP = idlOS::fopen(outTrcHeaderFile, "wb");
    if (outTrcHeaderFP == NULL)
    {
        idlOS::printf("Can't create file %s\n", outTrcHeaderFile);
        idlOS::exit(0);
    }

    outTrcCppFP = idlOS::fopen(outTrcCppFile, "wb");
    if (outTrcCppFP == NULL)
    {
        idlOS::printf("Can't create file %s\n", outTrcCppFile);
        idlOS::exit(0);
    }

    outMsbFP = idlOS::fopen(outMsbFile, "wb");
    if (outMsbFP == NULL)
    {
        idlOS::printf("Can't create file %s\n", outMsbFile);
        idlOS::exit(0);
    }

    if (gClientPart == 0)
    {
        // Server의 경우 : 메시지 화일 헤더 구조체 만큼 스킵
        idlOS::fseek(outMsbFP, sizeof(idErrorMsbType), SEEK_SET);
    }

    SChar buffer[1024];
    SChar dupBuf[E_INDEX_MASK];
    SInt  line = 1;

    idlOS::memset(dupBuf, 0, sizeof(dupBuf));

    while(!feof(inMsgFP))
    {
        SInt   SubCode = 0;
        SChar *State   = NULL;
        SChar *Name    = NULL;
        SChar *Value   = NULL;

        idlOS::memset(buffer, 0, 1024);
        if (idlOS::fgets(buffer, 1024, inMsgFP) == NULL)
        {
            // 화일의 끝까지 읽음
            break;
        }

        // buffer에 한줄의 정보가 있음
        if (parseBuffer(buffer, &SubCode, &State, &Name, &Value) == -1)
        {
            idlOS::fclose(inMsgFP);
            idlOS::printf("에러... 라인 [%d]:%s\n", line, buffer);
            idlOS::exit(0);
        }


        if (Section == -1) // Section 설정되지 않았을 경우
        {
            if (Name)
            {
                if (idlOS::strcasecmp(Name, "SECTION") == 0)
                {
                    assert( Value != NULL );
                    Section = (SInt)idlOS::strtol(Value, NULL, 10);
                    if (Section < 0 || Section > 15)
                        ErrorOut((SChar *)"Section의 범위가 틀렸습니다.",
                                 line, buffer);
                    // 화일에 헤더 출력
                    idlOS::fprintf(outErrHeaderFP, "%s\n", Header);
                }
            }
        }
        else // Section이 설정되었음 : 여기부터는 정확한 N=V 요구
        {
            if ( (Name && Value == NULL) || (Name == NULL && Value) )
            {
                if ((Name != NULL))
                {
                    if (idlOS::strcmp(Name, "INTERNAL_TRACE_MESSAGE_BEGIN") == 0)
                    {
                        // Internal Trace Message Generation Do.
                        doTraceGeneration();

                        break;
                    }
                }

                ErrorOut((SChar *)"에러코드와 에러메시지가 동시에 존재하지 않습니다.",
                         line, buffer);
            }
            else
            {

                if ( (Name != NULL) && ( Value != NULL) ) // 값이 설정됨
                {
                    if (SubCode < 0)
                    {
                        idlOS::fclose(inMsgFP);
                        idlOS::printf("에러:SubCode의 값이 정의되지 않은 것 같습니다. \n line%d:%s\n", line, buffer);
                        idlOS::exit(0);
                    }

                    if (SubCode >= E_INDEX_MASK)
                    {
                        idlOS::fclose(inMsgFP);
                        idlOS::printf("에러:SubCode의 값이 허용된 값(%d) 이상입니다. \n line%d:%s\n", E_INDEX_MASK, line, buffer);
                        idlOS::exit(0);
                    }

                    if (dupBuf[SubCode] != 0)
                    {
                        idlOS::fclose(inMsgFP);
                        idlOS::printf("에러:SubCode의 값(%d)이 중복되었습니다. \n line%d:%s\n", SubCode, line, buffer);
                        idlOS::exit(0);
                    }

                    dupBuf[SubCode] = 1;

                    CheckValidation(line, Name, Value);

                    UInt Action       = getAction(line, Name);

                    HexErrorCode =
                        ((UInt)Section << 28) | (UInt)Action | ((UInt)SubCode << 12) | (UInt)errIdxNum;
                    // 1. 헤더화일 데이타 저장 (*.ih)
                    idlOS::fprintf(outErrHeaderFP, "    %s = 0x%08x, /* (0x%x) (%d) */\n",
                                   Name, HexErrorCode, E_ERROR_CODE(HexErrorCode), E_ERROR_CODE(HexErrorCode));

                    if (gClientPart == 0)
                    {
                        // Server Part :  메시지 화일 데이타 저장 (*.msb)
                        len = idlOS::strlen(Value) + 1;
#ifdef MSB_ECODE_ON
                        idlOS::fprintf(outMsbFP, "0x%08x %s\n", errIdxNum, Value);
#else
                        idlOS::fprintf(outMsbFP, "%s\n", Value);
#endif
                    }
                    else
                    {
                        /* client part */

                        if ( gMsgOnly == 0 )
                        {
                            // Client Part : 소스코드 생성
                            idlOS::fprintf(outMsbFP, "    { \"%s\", \"%s\"}, \n", State, Value);
                        }
                        else
                        {
                            /* No state code will be printed in the file. */
                            idlOS::fprintf(outMsbFP, "     \"%s\", \n", Value);
                        }
                    }
                    errIdxNum++;
                }
                else
                {
                    // comment. SKIP
                }

            }
        }
        line++;
    }

    msbHeader.value_.header.AltiVersionId = idlOS::hton((UInt)iduVersionID);
    msbHeader.value_.header.ErrorCount    = idlOS::hton((ULong)errIdxNum);
    msbHeader.value_.header.Section       = idlOS::hton((UInt)Section);

    if (gClientPart == 0)
    {
        idlOS::fseek(outMsbFP, 0, SEEK_SET);
        idlOS::fwrite(&msbHeader, sizeof(msbHeader), 1, outMsbFP);
    }

    idlOS::fclose(inMsgFP);
    idlOS::fclose(outErrHeaderFP);
    idlOS::fclose(outTrcHeaderFP);
    idlOS::fclose(outTrcCppFP);
    idlOS::fclose(outMsbFP);
	return 0;
}


extern ideErrTypeInfo typeInfo[];

/* ------------------------------------------------
 *  Formatted String을 변환한다.
    { IDE_ERR_SCHAR,  "%c" ,  "%c", 2},
    { IDE_ERR_STRING, "%s" ,  "%s", 2},
    { IDE_ERR_SINT,   "%d" ,  "%d", 2},
    { IDE_ERR_UINT,   "%u" ,  "%d", 2},
    { IDE_ERR_SLONG,  "%ld" , "%"ID_INT64_FMT,   3},
    { IDE_ERR_ULONG,  "%lu" , "%"ID_UINT64_FMT,  3},
    { IDE_ERR_VSLONG, "%vd" , "%"ID_vSLONG_FMT,  3},
    { IDE_ERR_VULONG, "%vu" , "%"ID_vULONG_FMT,  3},
    { IDE_ERR_HEX32,  "%x" ,  "%"ID_xINT32_FMT,  2},
    { IDE_ERR_HEX64,  "%lx" , "%"ID_xINT64_FMT,  3},
    { IDE_ERR_HEX_V,  "%vx" , "%"ID_vxULONG_FMT, 3},
    { IDE_ERR_NONE,   "" ,    "", 0},
 * ----------------------------------------------*/
UInt convertFormatString(SChar *sWorkPtr, SChar *sFmt)
{
    UInt            i;

    for ( i = 0; typeInfo[i].len != 0; i++)
    {
        if (idlOS::strcmp(sFmt, typeInfo[i].tmpSpecStr) == 0)
        {
            idlOS::strcpy(sWorkPtr, typeInfo[i].realSpecStr);

            return idlOS::strlen(typeInfo[i].realSpecStr);
        }
    }
    idlOS::fprintf(stderr, "formatted string error : [%s]:%s\n", sWorkPtr, sFmt);
    idlOS::exit(-1);
    return 0;
}

/* ------------------------------------------------
 *  print Error Position
 * ----------------------------------------------*/

void printErrorPosition(SChar *aString, UInt aPos)
{
    UInt j;

    idlOS::fprintf(stderr, "!!!!!!!!!!!!!!! ERROR !!!!!!!!!!!!!!!!!\n");

    for (j = 0; j < idlOS::strlen(aString); j++)
    {
        SChar sChar;

        switch((sChar = aString[j]))
        {
            case '\n':
            case '\r':
            case ' ':
            case '\t':
                sChar = ' ';
                break;
            default:
                break;
        }
        if (j == aPos)
        {
            idlOS::fprintf(stderr, "[ERROR] => %c", sChar);
        }
        idlOS::fprintf(stderr, "%c", sChar);

    }
    idlOS::fprintf(stderr, "\n");
    idlOS::exit(-1);
}



/* ------------------------------------------------
 *  Trace Message를 생성한다.
 * ----------------------------------------------*/
void doTraceGeneration()
{
    UInt   sSeqNum = 0; // Trace Code의 번호

    SChar  sLineBuffer[1024];
    SChar *sFormatString;
    SChar *sWorkString;

    // 64k를 넘는 에러 메시지는 없겠지?
    sFormatString = (SChar *)idlOS::malloc(64 * 1024);
    sWorkString   = (SChar *)idlOS::malloc(64 * 1024);

    assert( (sFormatString != NULL) && (sWorkString != NULL));

    while(!feof(inMsgFP))
    {
        SInt   SubCode = 0;
        SChar *State   = NULL;
        SChar *Name    = NULL;
        SChar *Value   = NULL;

        idlOS::memset(sLineBuffer, 0, 1024);
        if (idlOS::fgets(sLineBuffer, 1024, inMsgFP) == NULL)
        {
            // 화일의 끝까지 읽음
            break;
        }

        // sLineBuffer에 한줄의 정보가 있음
        if (parseBuffer(sLineBuffer, &SubCode, &State, &Name, &Value, 0) == -1)
        {
            idlOS::fclose(inMsgFP);
            idlOS::printf("에러... 라인 [%d]:%s\n", 0, sLineBuffer);
            idlOS::exit(0);
        }

        /* ------------------------------------------------
         *  이 블럭내에서는 스트링 값을 읽고 처리함.
         * ----------------------------------------------*/
        if ( (Name != NULL) && (Value != NULL ))
        {
            UInt i;

            idlOS::memset(sFormatString, 0, 64 * 1024);

            /* ------------------------------------------------
             *  1.  ;(세미콜론)이 발생할 때 까지 읽음.
             * ----------------------------------------------*/
            i = idlOS::strlen(Value);

            idlOS::strcpy(sFormatString, Value);

            if (sFormatString[i - 1] == ';')
            {
                // remove ;
                sFormatString[i - 1] = 0;
            }
            else
            {
                while(!feof(inMsgFP))
                {
                    SChar sCh;

                    if (idlOS::fread(&sCh, 1, 1, inMsgFP) == 1)
                    {
                        if (sCh == ';')
                        {
                            break;
                        }
                        else
                        {
                            sFormatString[i++] = sCh;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }


            /* ------------------------------------------------
             *  2. sFormatString에서 " "외부에 있는 공백
             *     및 " 자체를 을 제거한다.
             * ----------------------------------------------*/
            {
                UInt sQuoteState = 0;
                SChar *sWorkPtr = sWorkString;

                idlOS::memset(sWorkString, 0, 64 * 1024);

                for (i = 0; sFormatString[i] != 0; i++)
                {
                    switch(sQuoteState)
                    {
                        case 0: /* non quote state : remove space*/

                            if (sFormatString[i] == '"')
                            {
                                sQuoteState = 1;
                            }
                            else
                            {
                                if (idlOS::idlOS_isspace(sFormatString[i]))
                                {
                                    // OK..skip this
                                }
                                else
                                {
                                    // character occurred in no quote state.
                                    // ex)  "abcd"\n   baaa  "abcd"  <== ERR
                                    printErrorPosition(sFormatString, i);
                                }
                            }

                            break;

                        case 1: /* in quote state : just skip*/
                            if (sFormatString[i] == '"')
                            {
                                if (*(sWorkPtr - 1) == '\\')
                                {
                                    *sWorkPtr++ = sFormatString[i];

                                    // "이 스트링 내부에 있을 경우 " .... \"... "
                                    //*(sWorkPtr - 1) = '"'; remove case
                                }
                                else
                                {
                                    sQuoteState = 0;
                                }

                            }
                            else
                            {
                                *sWorkPtr++ = sFormatString[i];
                            }
                            break;
                    }
                }
            }

            // recover
            idlOS::memcpy(sFormatString, sWorkString, 64 * 1024);

            if (gDebug != 0)
            {
                idlOS::fprintf(stderr, "%s = [%s]\n", Name, sFormatString);
            }


            /* ------------------------------------------------
             *  3. sFormatString에서 스트링 변환 수행.
             *     => \ escape 문자 대치
             *     => 가변인자 <0%d>를 대치
             *
             * ----------------------------------------------*/
            {
                SChar *sWorkPtr = sWorkString;

                idlOS::memset(sWorkString, 0, 64 * 1024);

                for (i = 0; sFormatString[i] != 0; i++)
                {
                    switch(sFormatString[i])
                    {
#ifdef NOTDEF // text -> text 에서는 필요없음. text -> bin은 필요함.
                        case '\\': /* escape sequence char*/
                            switch(sFormatString[i + 1])
                            {
                                case 'n': // /n
                                    *sWorkPtr++ = '\n';
                                    break;
                                case 'r': // /n
                                    *sWorkPtr++ = '\r';
                                    break;
                                case 't': // /n
                                    *sWorkPtr++ = '\t';
                                    break;
                                case '\\': // /n
                                    *sWorkPtr++ = '\\';
                                    break;
                                case '\"': // /n
                                    *sWorkPtr++ = '\\';
                                    break;
                                default:
                                    *sWorkPtr++ = sFormatString[i + 1];
                            }
                            i++; // skip next char

                            break;
#endif
                        case '<': // argument control
                        {
                            UInt  k;
                            UInt  sLen;


                            SChar sFmt[128]; // <...>의 최대크기를 128로 가정함.

                            idlOS::memset(sFmt, 0, ID_SIZEOF(sFmt));

                            for (k = 1; k < ID_SIZEOF(sFmt); k++)
                            {
                                if (sFormatString[i + k]  == '>')
                                {
                                    break;
                                }
                                sFmt[k - 1] = sFormatString[i + k];
                            }

                            // convert formatted string

                            sLen = convertFormatString(sWorkPtr, sFmt);

                            sWorkPtr += sLen;

                            i += k; // skip to next char

                        }
                        break;
                        case '%':
                            if (sFormatString[ i + 1 ] != '%')
                            {
                                printErrorPosition(sFormatString, i);
                            }
                            else
                            {
                                *sWorkPtr++ = sFormatString[i];
                                *sWorkPtr++ = sFormatString[i + 1];
                                i++; // skip double %% as %
                            }

                            break;

                        default:
                            *sWorkPtr++ = sFormatString[i];
                    }
                }
            }

            // recover
            idlOS::memcpy(sFormatString, sWorkString, 64 * 1024);

            /* ------------------------------------------------
             *  !! 완료
             *  Name과 sFormatString을 출력한다.
             * ----------------------------------------------*/
            if (gDebug != 0)
            {
                idlOS::fprintf(stderr, "after [%s]\n", sFormatString);
            }
            idlOS::fprintf(outTrcHeaderFP, "#define  %s  g%c%cTraceCode[%d]\n",
                           Name,
                           inMsgFile[2],
                           inMsgFile[3],
                           sSeqNum);

            idlOS::fprintf(outTrcCppFP, "   \"%s\",\n", sFormatString);

            sSeqNum++;

        }
        else
        {
            if ( (Name != NULL) )
            {
                if (idlOS::strcmp(Name, "INTERNAL_TRACE_MESSAGE_END") == 0)
                {
                    return;
                }

                if (idlOS::strlen(Name) > 0)
                {
                    idlOS::fprintf(stderr, "!!!!!!!!!!!!!!! ERROR !!!!!!!!!!!!!!!!!\n");

                    idlOS::fprintf(stderr, "No Value in [Name] %s \n", Name);

                    idlOS::fprintf(stderr, "Check the format : NAME = VALUE \n");

                    idlOS::exit(-1);
                }
            }
        }
    }
}


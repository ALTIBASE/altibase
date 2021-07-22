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
 
/*****************************************************************************
 * $Id: checkEnv.cpp 80575 2017-07-21 07:06:35Z yoonhee.kim $
 *
 * DESC : configure된 내용이 실제 RUNTIME 결과에 문제를 미치지 않는지 검사.
 *        by gamestar 2000/8/30
 *
 ****************************************************************************/
#include <idl.h>

#include <ida.h>
#include <iduVersion.h>
#include <iddTypes.h>
#include <sqlcli.h>
#include "testInline.h"

#include <actTest.h>

#define MAKE_ARG2( a1, a2 )            ( (((UInt)(a1))<<8) | ((UInt)(a2)) )

typedef struct
{
    SChar name[128];
    SInt compile_size;
    SInt runtime_size;
} Datasize;

static Datasize TypeFactory[] =
{
    { "SChar",  sizeof(SChar),  1 },
    { "UChar",  sizeof(UChar),  1 },
    { "SShort", sizeof(SShort), 2 },
    { "UShort", sizeof(UShort), 2 },
    { "SInt",   sizeof(SInt),   4 },
    { "UInt",   sizeof(UInt),   4 },
    { "SLong",  sizeof(SLong),  8 },
    { "ULong",  sizeof(ULong),  8 },
#ifdef COMPILE_64BIT
    { "vULong", sizeof(vULong), 8 },
    { "vSLong", sizeof(vSLong), 8 },
    { "SChar*", sizeof(SChar*), 8 },
#else
    { "vULong", sizeof(vULong), 4 },
    { "vSLong", sizeof(vSLong), 4 },
    { "SChar*", sizeof(SChar*), 4 },
#endif

    { "SQLINTEGER", sizeof(SQLINTEGER), 4 },
#if defined(COMPILE_64BIT) && defined(_MSC_VER)
    { "SQLLEN",     sizeof(SQLLEN),     8 },
#else
    { "SQLLEN",     sizeof(SQLLEN),     4 },
#endif

    { "end",    0, 0} /* end of check */
};

void checkInline()
{
    chkLSN lsn1 = {ID_UINT_MAX, ID_UINT_MAX};
    chkLSN lsn2 = {0, 0};

    ACT_CHECK( testInline::isLT(&lsn1, &lsn2) == ID_FALSE );
}

void checkVersion()
{
    UInt myMajor, myMinor;

    idlOS::printf("Version ID = %08X\n", iduVersionID);
    idlOS::printf("Version String = %s\n", iduVersionString);

    myMajor = __MajorVersion__ >> 24;
    myMinor = __MinorVersion__ >> 16;

    ACT_CHECK_DESC( ( myMajor > 0 && myMajor < 100 ), ( "Major Version range error.. 0 < range < 100 [%X]", __MajorVersion__ ) );

    ACT_CHECK_DESC( myMinor < 100, ( "Minor Version range error.. 0 <= range < 100 [%X]", __MinorVersion__ ) );

    ACT_CHECK_DESC( __PatchLevel__ < 10000, ( "Patch Level range error.. 0 <= range < 10000 [%X]", __PatchLevel__ ) );

    idlOS::printf("Production Time = %s \n",
                  iduGetProductionTimeString());

    idlOS::printf("Production Version  = %s \n",
                  iduGetVersionString());

    idlOS::printf("Production CopyRight = %s \n",
                  iduGetCopyRightString());

    idlOS::printf("Production System Info = %s \n",
                  iduGetSystemInfoString());
}

void checkDataType()
{
    SInt i;

    idlOS::printf("Check of DataType and the Size\n");

    for(i = 0; ; i++)
    {
        if(TypeFactory[i].compile_size == 0)
        {
            break;
        }

        idlOS::printf("[%s] compile_size[%d] runtime_size[%d]\n",
                      TypeFactory[i].name,
                      TypeFactory[i].compile_size,
                      TypeFactory[i].runtime_size);

        ACT_CHECK_DESC( TypeFactory[i].compile_size == TypeFactory[i].runtime_size, ( "size of %s is incorrect (%s : %d, expected : %d)", TypeFactory[i].name, TypeFactory[i].name, TypeFactory[i].compile_size, TypeFactory[i].runtime_size ) );
    }
}

void checkLongLiteral()
{
    static ULong data = ID_ULONG(0x1234567812345678);

    ACT_CHECK_DESC( data == ID_ULONG(0x1234567812345678), ( "Compare data=0x%"ID_xINT64_FMT" literal=%s\n", data, "0x1234567812345678" ) );
}

#define SSHORT_VALUE  0x80F0
#define USHORT_VALUE  0x80F0
#define SINT_VALUE    0x80F0A0B0
#define UINT_VALUE    0x80F0A0B0
#define SLONG_VALUE   ID_LONG(0x80F0A0B0C0D0E0F0)
#define ULONG_VALUE   ID_ULONG(0x80F0A0B0C0D0E0F0)

void checkEndianConversion()
{
    static SShort sshort_data = SSHORT_VALUE;
    static UShort ushort_data = USHORT_VALUE;
    static SInt   sint_data   = SINT_VALUE;
    static UInt   uint_data   = UINT_VALUE;
    static SLong  slong_data  = SLONG_VALUE;
    static ULong  ulong_data  = ULONG_VALUE;

    ////// SSHORT
    SShort ss1 = idlOS::hton(sshort_data);
    SShort ss2 = idlOS::ntoh(ss1);

    idlOS::printf("SSHORT=(%d)(0x%x) -> HTON=(%d)(0x%x) -> NTOH=(%d)(0x%x) \n\n",
                  sshort_data, sshort_data,
                  ss1, ss1,
                  ss2, ss2 );

    ACT_CHECK( sshort_data  == ss2 );

    ///// USHORT
    UShort us1 = idlOS::hton(ushort_data);
    UShort us2 = idlOS::ntoh(us1);

    idlOS::printf("USHORT=(%d)(0x%x) -> HTON=(%d)(0x%x) -> NTOH=(%d)(0x%x) \n\n",
                  ushort_data, ushort_data,
                  us1, us1,
                  us2, us2 );

    ACT_CHECK( ushort_data == us2 );

    ///// SINT
    SInt si1 = idlOS::hton(sint_data);
    SInt si2 = idlOS::ntoh(si1);

    idlOS::printf("SINT=(%d)(0x%x) -> HTON=(%d)(0x%x) -> NTOH=(%d)(0x%x) \n\n",
                  sint_data, sint_data,
                  si1, si1,
                  si2, si2 );

    ACT_CHECK( sint_data == si2 );

    ///// UINT
    UInt ui1 = idlOS::hton(uint_data);
    UInt ui2 = idlOS::ntoh(ui1);

    idlOS::printf("UINT=(%d)(0x%x) -> HTON=(%d)(0x%x) -> NTOH=(%d)(0x%x) \n\n",
                  uint_data, uint_data,
                  ui1, ui1,
                  ui2, ui2 );

    ACT_CHECK( uint_data == ui2 );

    ///// SLONG
    SLong sl1 = idlOS::hton(slong_data);
    SLong sl2 = idlOS::ntoh(sl1);

    idlOS::printf("SLONG=(%"ID_INT64_FMT")(0x%"ID_xINT64_FMT") -> HTON=(%"ID_INT64_FMT")(0x%"ID_xINT64_FMT") -> NTOH=(%"ID_INT64_FMT")(0x%"ID_xINT64_FMT") \n\n",
                  slong_data, slong_data,
                  sl1, sl1,
                  sl2, sl2 );

    ACT_CHECK( slong_data == sl2 );

    ///// ULONG
    ULong ul1 = idlOS::hton(ulong_data);
    ULong ul2 = idlOS::ntoh(ul1);

    idlOS::printf("ULONG=(%"ID_UINT64_FMT")(0x%"ID_xINT64_FMT") -> HTON=(%"ID_UINT64_FMT")(0x%"ID_xINT64_FMT") -> NTOH=(%"ID_UINT64_FMT")(0x%"ID_xINT64_FMT") \n\n",
                  ulong_data, ulong_data,
                  ul1, ul1,
                  ul2, ul2 );

    ACT_CHECK( ulong_data == ul2 );
}

void checkToUpper()
{
    SChar *s1 = (SChar*)"가나다라마바사";
    SChar  c1;
    SChar  c2;
    SInt   i;

    for( i = 0; s1[i]; i++)
    {
        c1 = s1[i];
        c2 = idlOS::idlOS_toupper(s1[i]);

        ACT_CHECK_DESC( c1 == c2, ("[%d] %c != %c", i, c1, c2) );
    }
}

void checkStructInit()
{
    typedef struct
    {
        SChar tmpSpecStr[10];
    } mystruct_t;

    mystruct_t mystruct[] = { { "1234567" }, { "" }, };

    ACT_CHECK(idlOS::strcmp(mystruct[0].tmpSpecStr,"1234567") == 0);
    ACT_CHECK(idlOS::strlen(mystruct[0].tmpSpecStr) == 7);
    ACT_CHECK(idlOS::strcmp(mystruct[1].tmpSpecStr,"") == 0);
    ACT_CHECK(idlOS::strlen(mystruct[1].tmpSpecStr) == 0);
}

ULong hton ( ULong Value )
{
    return (((Value&ID_ULONG(0xFF00000000000000))>>56)|
            ((Value&ID_ULONG(0x00FF000000000000))>>40)|
            ((Value&ID_ULONG(0x0000FF0000000000))>>24)|
            ((Value&ID_ULONG(0x000000FF00000000))>>8)|
            ((Value&ID_ULONG(0x00000000FF000000))<<8)|
            ((Value&ID_ULONG(0x0000000000FF0000))<<24)|
            ((Value&ID_ULONG(0x000000000000FF00))<<40)|
            ((Value&ID_ULONG(0x00000000000000FF))<<56));
}

void checkFdSet()
{
    SInt i;
    SInt CurMaxHandle, MaxHandle;
    fd_set  myset;

    CurMaxHandle = idlVA::max_handles();
    (void)idlVA::set_handle_limit(); // 최대로 올림
    MaxHandle = idlVA::max_handles();

    ACT_CHECK_DESC( MaxHandle <= FD_SETSIZE, ( "MaxHandle(%"ID_INT32_FMT") > FD_SETSIZE(%"ID_INT32_FMT").", MaxHandle, FD_SETSIZE ) );

    ACT_CHECK_DESC( ID_SIZEOF(fd_set) >= (SInt) (FD_SETSIZE / 8), ( "size of fd_set(%"ID_INT32_FMT") < (FD_SETSIZE/ 8)(%"ID_INT32_FMT").", ID_SIZEOF(fd_set), (SInt) (FD_SETSIZE / 8) ) );

    idlOS::printf("MaxHandle %d \n", MaxHandle);

    FD_ZERO(&myset);

    for(i = 0; i < MaxHandle; i++)
    {
        FD_SET(i, &myset);
    }
    for(i = 0; i < MaxHandle; i++)
    {
        if(FD_ISSET(i, &myset))
        {
            idlOS::printf("SET ok  %d \n", i);

            FD_CLR(i, &myset);

            if(FD_ISSET(i, &myset))
            {
                ACT_ERROR( ("CLR error!  %d \n", i) );
                break;
            }
            else
            {
                idlOS::printf("CLR ok  %d \n", i);
            }
        }
        else
        {
            ACT_ERROR( ("SET error!  %d \n", i) );
            break;
        }
    }
}

SInt checkMacroArg_ALL(
    iddDataType * arg1,
    iddDataType * arg2 )
{
    switch( MAKE_ARG2( *arg1, *arg2 ) )
    {
        case MAKE_ARG2( IDDT_CHA, IDDT_CHA ):
        case MAKE_ARG2( IDDT_CHA, IDDT_VAR ):
        case MAKE_ARG2( IDDT_CHA, IDDT_NUM ):
        case MAKE_ARG2( IDDT_CHA, IDDT_TNU ):
        case MAKE_ARG2( IDDT_CHA, IDDT_INT ):
        case MAKE_ARG2( IDDT_CHA, IDDT_DAT ):
        case MAKE_ARG2( IDDT_CHA, IDDT_NUL ):
        case MAKE_ARG2( IDDT_VAR, IDDT_CHA ):
        case MAKE_ARG2( IDDT_VAR, IDDT_VAR ):
        case MAKE_ARG2( IDDT_VAR, IDDT_NUM ):
        case MAKE_ARG2( IDDT_VAR, IDDT_TNU ):
        case MAKE_ARG2( IDDT_VAR, IDDT_INT ):
        case MAKE_ARG2( IDDT_VAR, IDDT_DAT ):
        case MAKE_ARG2( IDDT_VAR, IDDT_NUL ):
        case MAKE_ARG2( IDDT_NUM, IDDT_CHA ):
        case MAKE_ARG2( IDDT_NUM, IDDT_VAR ):
        case MAKE_ARG2( IDDT_NUM, IDDT_NUM ):
        case MAKE_ARG2( IDDT_NUM, IDDT_TNU ):
        case MAKE_ARG2( IDDT_NUM, IDDT_INT ):
        case MAKE_ARG2( IDDT_NUM, IDDT_DAT ):
        case MAKE_ARG2( IDDT_NUM, IDDT_NUL ):
        case MAKE_ARG2( IDDT_TNU, IDDT_CHA ):
        case MAKE_ARG2( IDDT_TNU, IDDT_VAR ):
        case MAKE_ARG2( IDDT_TNU, IDDT_NUM ):
        case MAKE_ARG2( IDDT_TNU, IDDT_TNU ):
        case MAKE_ARG2( IDDT_TNU, IDDT_INT ):
        case MAKE_ARG2( IDDT_TNU, IDDT_DAT ):
        case MAKE_ARG2( IDDT_TNU, IDDT_NUL ):
        case MAKE_ARG2( IDDT_INT, IDDT_CHA ):
        case MAKE_ARG2( IDDT_INT, IDDT_VAR ):
        case MAKE_ARG2( IDDT_INT, IDDT_NUM ):
        case MAKE_ARG2( IDDT_INT, IDDT_TNU ):
        case MAKE_ARG2( IDDT_INT, IDDT_INT ):
        case MAKE_ARG2( IDDT_INT, IDDT_DAT ):
        case MAKE_ARG2( IDDT_INT, IDDT_NUL ):
        case MAKE_ARG2( IDDT_DAT, IDDT_CHA ):
        case MAKE_ARG2( IDDT_DAT, IDDT_VAR ):
        case MAKE_ARG2( IDDT_DAT, IDDT_NUM ):
        case MAKE_ARG2( IDDT_DAT, IDDT_TNU ):
        case MAKE_ARG2( IDDT_DAT, IDDT_INT ):
        case MAKE_ARG2( IDDT_DAT, IDDT_DAT ):
        case MAKE_ARG2( IDDT_DAT, IDDT_NUL ):
        case MAKE_ARG2( IDDT_NUL, IDDT_CHA ):
        case MAKE_ARG2( IDDT_NUL, IDDT_VAR ):
        case MAKE_ARG2( IDDT_NUL, IDDT_NUM ):
        case MAKE_ARG2( IDDT_NUL, IDDT_TNU ):
        case MAKE_ARG2( IDDT_NUL, IDDT_INT ):
        case MAKE_ARG2( IDDT_NUL, IDDT_DAT ):
        case MAKE_ARG2( IDDT_NUL, IDDT_NUL ):
            break;
        default:
            goto ERR_NOT_AVAILABLE;
            break;
    }

    return IDE_SUCCESS;

ERR_NOT_AVAILABLE:
    idlOS::printf("checkMacroArg_ALL error\n");
    return IDE_FAILURE;
}

void checkMacroArg()
{
    SInt sIndex;
    SInt sIndex2;

    iddDataType arg1[6] = {IDDT_CHA, IDDT_VAR, IDDT_NUM, IDDT_TNU, IDDT_DAT, IDDT_NUL};

    for(sIndex = 0; sIndex < 6; sIndex++)
    {
        for(sIndex2 = 0; sIndex2 < 6; sIndex2++)
        {
            ACT_CHECK_DESC( checkMacroArg_ALL( &(arg1[sIndex]), &(arg1[sIndex2])) == IDE_SUCCESS , ("checkMacroArg Error : [arg1 : %d][arg2 : %d], sIndex, sIndex2") );
        }
    }
}

void checkGetSCN()
{
    ULong sSCN1;
    ULong sSCN2;

    sSCN1 = 0x10005867;
    sSCN2 = ID_ULONG(0x8000000000000000);

    CHK_SET_SCN(&sSCN1, &sSCN2);

    ACT_CHECK_DESC( sSCN1 == sSCN2, ( "GetSCN error [sSCN1:%"ID_UINT64_FMT"][sSCN2:%"ID_UINT64_FMT"]", sSCN1, sSCN2 ) );
}

// struct 멤버만 있는 것과 struct 멤버와 inline 함수가 같이 있을때
// struct가 Disk에 저장될 경우 함수가 저장되지 않는 것을 보장하기 위한 테스트
void checkSizeOfStructWithFunction()
{
    struct A
    {
        vULong a1;
    private:
        vULong a2;
    };

    struct B
    {
        vULong b1;
    private:
        vULong b2;
    public:
        inline vULong f() { vULong f1=0; return (f1+b1+b2); };
    };

    ACT_CHECK_DESC( sizeof(A) == sizeof(B), ( "Size Different Between Struct With Member Only And Struct With Function" ) );
}

void checkLongVal()
{
    SLong longVal;
    SLong longValResult;

    // BUG of optimizer in VC++ using "Maximize Speed" mode
    longVal = hton(3);

    longValResult = \
    (((((longVal&ID_ULONG(0xFF00000000000000))>>56)|
       ((longVal&ID_ULONG(0x00FF000000000000))>>40)|
       ((longVal&ID_ULONG(0x0000FF0000000000))>>24)|
       ((longVal&ID_ULONG(0x000000FF00000000))>>8)|
       ((longVal&ID_ULONG(0x00000000FF000000))<<8)|
       ((longVal&ID_ULONG(0x0000000000FF0000))<<24)|
       ((longVal&ID_ULONG(0x000000000000FF00))<<40)|
       ((longVal&ID_ULONG(0x00000000000000FF))<<56))+7) >> 3 << 3 );

    ACT_CHECK( longValResult == 8 );
}


int main(/*BUGBUG_NT  int argc, char *argv[]  BUGBUG_NT*/)
{
    ACT_TEST_BEGIN();

    checkVersion();

    checkDataType();

    checkLongLiteral();

    checkEndianConversion();

    checkToUpper();

    checkInline();

    checkSizeOfStructWithFunction();

#if !defined (_MSC_VER)
    checkFdSet();
#endif

#if !defined (AIX)
    checkStructInit(); /* for aix porint */
#endif

    checkLongVal();

    checkMacroArg();

    checkGetSCN();

    ACT_TEST_END();

    idlOS::exit(0);
    /*BUGBUG_NT*/return 0;/*BUGBUG_NT*/
}


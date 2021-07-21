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

#ifndef _ULP_MACRO_H_
#define _ULP_MACRO_H_ 1

/* ulpMain */

// 프로그램 도중 에러가 발생했을경우 중간 file과 결과 file을 제거해 줘야한다.
// 수행코드 부분에 따라 file제거 여부를 결정한다.
#define ERR_DEL_FILE_NONE     (0)
#define ERR_DEL_TMP_FILE      (1)
#define ERR_DEL_ALL_FILE      (2)

/* ulpProgOption */

/* BUG-33025 Predefined types should be able to be set by user in APRE 
 * name of header file for predefined types
 * used in ulpProgOption and ulpGenCode */
#define PREDEFINE_HEADER      "aprePredefinedTypes.h"

#define MAX_INPUT_FILE_NUM      (50)
#define MAX_FILE_NAME_LEN      (256)
#define MAX_FILE_PATH_LEN     (1024)
#define MAX_HEADER_FILE_NUM    (100)
#define MAX_INCLUDE_PATH_LEN (10240)
#define MAX_FILE_EXT_LEN        (10)
#define MAX_DEFINE_NUM          (50)
#define MAX_DEFINE_NAME_LEN    (256)
#define MAX_VERSION_LEN       (1024)

#define NLS_NCHAR_NOT_USED     (0) // encoding: altibase_nls_use
#define NLS_NCHAR_UTF16        (1) // encoding: utf16

#define MAX_BANNER_SIZE       (1024)
#define MAX_KEYWORD_LEN        (100)

typedef enum
{
    HEADER_BEGIN = 0,
    HEADER_END
} ulpHEADERSTATE;

typedef enum
{
    PARSE_NONE = 0,
    PARSE_PARTIAL,
    PARSE_FULL
} ulpPARSEOPT;

/* ulpCodeGen */

#define GEN_WRITE_BUF_SIZE     (10240)
#define GEN_EMSQL_INFO_SIZE      (256)
#define MAX_HOSTVAR_NAME_SIZE   (1024)
#define GEN_INIT_QUERYBUF_SIZE (32768)
#define GEN_WRITE_BUF              (0)
#define GEN_QUERY_BUF              (1)
#define MAX_WHENEVER_ACTION_LEN (1024)
#define INIT_NUMOF_HOSTVAR       (100)
#define GEN_FIELD_OPT_LEN          (3)

#define GET_HVINFO_ZEROSET       (0x00000000)
#define GEN_HVINFO_IS_SYMNODE    (0x00000001)
#define GEN_HVINFO_IS_STRUCT     (0x00000002)
#define GEN_HVINFO_IS_VARCHAR    (0x00000004)
#define GEN_HVINFO_IS_1POINTER   (0x00000008)
#define GEN_HVINFO_IS_2POINTER   (0x00000010)
#define GEN_HVINFO_IS_ARRAY      (0x00000020)
#define GEN_HVINFO_IS_ARRAYSIZE1 (0x00000040)
#define GEN_HVINFO_IS_ARRAYSIZE2 (0x00000080)
#define GEN_HVINFO_IS_DALLOC     (0x00000100)
#define GEN_HVINFO_IS_SIGNED     (0x00000200)
#define GEN_HVINFO_IS_LOB        (0x00000400)
#define GEN_HVINFO_IS_MOREINFO   (0x00000800)
#define GEN_HVINFO_IS_INDNODE    (0x00001000)
#define GEN_HVINFO_IS_INDSTRUCT  (0x00002000)
#define GEN_HVINFO_IS_INDPOINTER (0x00004000)
#define GEN_HVINFO_IS_STRTYPE    (0x00008000)
// #define GEN_HVINFO_IS_     (0x00010000)
#define GEN_VERIFY_BIT( aSET, aBIT )  ( (aBIT) == ((aSET) & (UInt)(aBIT)) )
#define GEN_SUBSET_BIT( aSET, aSUBSET )    ( (aSET) & (aSUBSET) )

#define PRINT_mHostVar(STR, ...)  sSLength += idlOS::snprintf( sTmpStr + sSLength,\
                                            GEN_WRITE_BUF_SIZE - sSLength,\
                                            "    ulpSqlstmt.hostvalue[%d].mHostVar = " STR ";\n",\
                                            __VA_ARGS__ )
#define PRINT_mHostInd(STR, ...)  sSLength += idlOS::snprintf( sTmpStr + sSLength,\
                                            GEN_WRITE_BUF_SIZE - sSLength,\
                                            "    ulpSqlstmt.hostvalue[%d].mHostInd = " STR ";\n",\
                                            __VA_ARGS__ )
#define PRINT_mVcInd(STR, ...)    sSLength += idlOS::snprintf( sTmpStr + sSLength,\
                                            GEN_WRITE_BUF_SIZE - sSLength,\
                                            "    ulpSqlstmt.hostvalue[%d].mVcInd = " STR ";\n",\
                                            __VA_ARGS__ )
#define PRINT_isarr(STR, ...)     sSLength += idlOS::snprintf( sTmpStr + sSLength,\
                                            GEN_WRITE_BUF_SIZE - sSLength,\
                                            "    ulpSqlstmt.hostvalue[%d].isarr = " STR ";\n",\
                                            __VA_ARGS__ )
#define PRINT_arrsize(STR, ...)   sSLength += idlOS::snprintf( sTmpStr + sSLength,\
                                            GEN_WRITE_BUF_SIZE - sSLength,\
                                            "    ulpSqlstmt.hostvalue[%d].arrsize = " STR ";\n",\
                                            __VA_ARGS__ )
#define PRINT_mType(STR, ...)     sSLength += idlOS::snprintf( sTmpStr + sSLength,\
                                            GEN_WRITE_BUF_SIZE - sSLength,\
                                            "    ulpSqlstmt.hostvalue[%d].mType = " STR ";\n",\
                                            __VA_ARGS__ )
#define PRINT_isstruct(STR, ...)  sSLength += idlOS::snprintf( sTmpStr + sSLength,\
                                            GEN_WRITE_BUF_SIZE - sSLength,\
                                            "    ulpSqlstmt.hostvalue[%d].isstruct = " STR ";\n",\
                                            __VA_ARGS__ )
#define PRINT_structsize(STR, ...)  sSLength += idlOS::snprintf( sTmpStr + sSLength,\
                                            GEN_WRITE_BUF_SIZE - sSLength,\
                                            "    ulpSqlstmt.hostvalue[%d].structsize = " STR ";\n",\
                                            __VA_ARGS__ )
#define PRINT_mSizeof(STR, ...)   sSLength += idlOS::snprintf( sTmpStr + sSLength,\
                                            GEN_WRITE_BUF_SIZE - sSLength,\
                                            "    ulpSqlstmt.hostvalue[%d].mSizeof = " STR ";\n",\
                                            __VA_ARGS__ )
#define PRINT_mMaxlen(STR, ...)   sSLength += idlOS::snprintf( sTmpStr + sSLength,\
                                            GEN_WRITE_BUF_SIZE - sSLength,\
                                            "    ulpSqlstmt.hostvalue[%d].mMaxlen = " STR ";\n",\
                                            __VA_ARGS__ )
#define PRINT_mLen(STR, ...)      sSLength += idlOS::snprintf( sTmpStr + sSLength,\
                                            GEN_WRITE_BUF_SIZE - sSLength,\
                                            "    ulpSqlstmt.hostvalue[%d].mLen = " STR ";\n",\
                                            __VA_ARGS__ )
#define PRINT_mUnsign(STR, ...)   sSLength += idlOS::snprintf( sTmpStr + sSLength,\
                                            GEN_WRITE_BUF_SIZE - sSLength,\
                                            "    ulpSqlstmt.hostvalue[%d].mUnsign = " STR ";\n",\
                                            __VA_ARGS__ )
#define PRINT_mInOut(STR, ...)    sSLength += idlOS::snprintf( sTmpStr + sSLength,\
                                            GEN_WRITE_BUF_SIZE - sSLength,\
                                            "    ulpSqlstmt.hostvalue[%d].mInOut = " STR ";\n",\
                                            __VA_ARGS__ )
#define PRINT_mIsDynAlloc(STR, ...)  sSLength += idlOS::snprintf( sTmpStr + sSLength,\
                                            GEN_WRITE_BUF_SIZE - sSLength,\
                                            "    ulpSqlstmt.hostvalue[%d].mIsDynAlloc = " STR ";\n",\
                                            __VA_ARGS__ )
#define PRINT_mMoreInfo(STR, ...)   sSLength += idlOS::snprintf( sTmpStr + sSLength,\
                                            GEN_WRITE_BUF_SIZE - sSLength,\
                                            "    ulpSqlstmt.hostvalue[%d].mMoreInfo = " STR ";\n",\
                                            __VA_ARGS__ )
#define PRINT_mFileopt(STR, ...)    sSLength += idlOS::snprintf( sTmpStr + sSLength,\
                                            GEN_WRITE_BUF_SIZE - sSLength,\
                                            "    ulpSqlstmt.hostvalue[%d].mFileopt = " STR ";\n",\
                                            __VA_ARGS__ )
#define PRINT_mDiagType(STR, ...)   sSLength += idlOS::snprintf( sTmpStr + sSLength,\
                                            GEN_WRITE_BUF_SIZE - sSLength,\
                                            "    ulpSqlstmt.hostvalue[%d].mDiagType = " STR ";\n",\
                                            __VA_ARGS__ )
/* BUG-42357 [mm-apre] The -lines option is added to apre. (INC-31008) */
#define PRINT_LineMacro() do\
{\
    if (gUlpProgOption.mOptLineMacro == ID_TRUE)\
    {\
        sSLength += ulpGenMakeLineMacroStr( sTmpStr + sSLength, GEN_WRITE_BUF_SIZE - sSLength );\
    }\
} while (0)

#define WRITEtoFile(BUF,LEN) do\
{\
    ulpGenNString( BUF, LEN );\
    LEN = 0;\
} while (0)


typedef enum
{
    GEN_CONNNAME = 0,
    GEN_CURNAME,
    GEN_STMTNAME,
    GEN_STMTTYPE,
    GEN_ITERS,
    GEN_NUMHOSTVAR,
    GEN_SQLINFO,
    GEN_SCROLLCUR,
    GEN_QUERYSTR,
    GEN_QUERYHV,
    GEN_PSMEXEC,
    GEN_EXTRASTRINFO,
    GEN_STATUSPTR,
    GEN_ATOMIC,
    GEN_ERRCODEPTR,
    GEN_MT,
    GEN_HVTYPE,
    GEN_CURSORSCROLLABLE,
    GEN_CURSORSENSITIVITY,
    GEN_CURSORWITHHOLD,
    GEN_CONDITIONNUM
} ulpGENSQLINFO;

typedef enum
{
    GEN_WHEN_NONE = -1,
    GEN_WHEN_NOTFOUND = 0,
    GEN_WHEN_SQLERROR = 1,
    GEN_WHEN_NOTFOUND_SQLERROR = 2
} ulpGENWHENEVERCOND;

typedef enum
{
    GEN_WHEN_CONT = 0,
    GEN_WHEN_DO_FUNC,
    GEN_WHEN_DO_BREAK,
    GEN_WHEN_DO_CONT,
    GEN_WHEN_GOTO,
    GEN_WHEN_STOP
} ulpGENWHENEVERACT;

typedef enum
{
    GEN_GENERAL = 0,
    GEN_ARRAY,
    GEN_STRUCT,
    IN_GEN_ARRAYSTRUCT,
    OUT_GEN_ARRAYSTRUCT,
    INOUT_GEN_ARRAYSTRUCT
} ulpGENhvType;

/* ulpPreproc & ulpPreprocl & ulpPreprocy */

#define PP_ST_NONE          (-1)
#define PP_ST_INIT_SKIP     (-2)
#define PP_ST_MACRO         (-3)
#define PP_ST_MACRO_IF_SKIP (-4)

#define MAX_MACRO_DEFINE_NAME_LEN    (1024)
/* BUG-30233 : #define 내용이 5K이상이면 apre segv발생함. */
#define MAX_MACRO_DEFINE_CONTENT_LEN (1024*32)
#define MAX_MACRO_IF_EXPR_LEN        (1024*4)
#define MAX_SKIP_MACRO_LEN           (1024*4)
#define MAX_MACRO_IF_STACK_SIZE      (1024)
#define MAX_COMMENTSTR_LEN           (1024)

#define WRITESTR2BUFPP(STR) if( !gUlpPPisCInc ) { gUlpCodeGen.ulpGenString(STR); }
#define WRITECH2BUFPP(CH)   if( !gUlpPPisCInc ) { gUlpCodeGen.ulpGenPutChar(CH);  }
#define WRITEUNCH2BUFPP()   if( !gUlpPPisCInc ) { gUlpCodeGen.ulpGenUnputChar(); }

#define IFSTACKINITINDEX (-1)

typedef enum
{
    PP_IF = 0,
    PP_ELIF,
    PP_ELSE,
    PP_IFDEF,
    PP_IFNDEF,
    PP_ENDIF
} ulpPPiftype;

typedef enum
{
    PP_IF_IGNORE = 0,
    PP_IF_TRUE,
    PP_IF_FALSE,
    /* BUG-28162 : SESC_DECLARE 부활  */
    PP_IF_SESC_DEC
} ulpPPifresult;


/* ulpPreprocifl & ulpPreprocify */
#define MAX_ID_EXPANSION     (50)

/* ulpComp & ulpCompl & ulpCompy */

/* BUG-28061 : preprocessing을마치면 marco table을 초기화하고, *
 *             ulpComp 에서 재구축한다.                       */
#define CP_ST_VOID          (-1)
#define CP_ST_NONE          (-2)
#define CP_ST_PARTIAL       (-3)
#define CP_ST_EMSQL         (-4)
#define CP_ST_EN            (-5)
#define CP_ST_EP            (-6)
#define CP_ST_EJ            (-7)
#define CP_ST_C             (-8)
#define CP_ST_MACRO_IF_SKIP (-9)

#define MAX_EXPR_LEN        (1024)

/* BUG-28068 : #define 매크로이름 확장안되는 문제 */
#define WRITESTR2BUFCOMP(STR) if( (!gDontPrint2file) && (gUlpCOMPMacroExpIndex <= 0) ) { gUlpCodeGen.ulpGenString(STR); }
#define WRITECH2BUFCOMP(CH)   if( (!gDontPrint2file) && (gUlpCOMPMacroExpIndex <= 0) ) { gUlpCodeGen.ulpGenPutChar(CH); }
#define WRITEUNCH2BUFCOMP()   if( (!gDontPrint2file) && (gUlpCOMPMacroExpIndex <= 0) ) { gUlpCodeGen.ulpGenUnputChar(); }

#define WRITESTR2QUERYCOMP(STR)  { gUlpCodeGen.ulpGenQueryString(STR); }

#define ERR_E (1)
#define ERR_L (2)
#define ERR_M (3)
#define ERR_H (4)

/* BUG-28118 : system 헤더파일들도 파싱돼야함.                      *
  6th. problem : Nested structure 정의중 scope를 잘못 계산하는 문제 */
#define MAX_NESTED_STRUCT_DEPTH    (100)

typedef enum
{
    HV_UNKNOWN_TYPE = 0,
    HV_IN_TYPE,
    HV_OUT_TYPE,
    HV_OUT_PSM_TYPE,
    HV_INOUT_TYPE
} ulpHVarType;

typedef enum
{
    HV_FILE_NONE = 0,
    HV_FILE_CLOB,
    HV_FILE_BLOB
} ulpHVFileType;

/* ulpLexer */

typedef enum
{
    LEX_PP = 0,
    LEX_COMP
} ulpLexerKind;

/* SymTable */

#define MAX_NUMBER_LEN        (1024)
#define MAX_SYMTABLE_ELEMENTS (53)
#define MAX_SCOPE_DEPTH       (100)

#define BAN_FILE_NAME "APRE.ban"
#define INCLUDE_ULPLIBINTERFACE_STR "#include <ulpLibInterface.h>\n"
#define VERSION_STR "Altibase Precompiler2(APRE) Ver.1"

#endif

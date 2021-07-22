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

#ifndef _ULP_COMP_H_
#define _ULP_COMP_H_ 1


#include <idl.h>
#include <idn.h>
#include <ide.h>
#include <iduMemory.h>
#include <ulpProgOption.h>
#include <ulpGenCode.h>
#include <ulpMacroTable.h>
#include <ulpScopeTable.h>
#include <ulpSymTable.h>
#include <ulpIfStackMgr.h>
#include <ulpErrorMgr.h>
#include <ulpTypes.h>
#include <ulpMacro.h>
#include <ulpLibMacro.h>




//========= For Parser ==========//

// parsing function at ulpCompy.y
int doCOMPparse( SChar *aFilename );

// array binding을 해야할지(isarr을 true로 set할지) 여부를 체크하기 위한 함수.
idBool ulpCOMPCheckArray( ulpSymTElement *aSymNode );

// for validating host values
void ulpValidateHostValue( void         *yyvsp,
                           ulpHVarType   aInOutType,
                           ulpHVFileType aFileType,
                           idBool        aTransformQuery,
                           SInt          aNumofTokens,
                           SInt          aHostValIndex,
                           SInt          aRemoveTokIndexs );

void ulpValidateHostValueWithDiagType(
                           void         *yyvsp,
                           ulpHVarType   aInOutType,
                           ulpHVFileType aFileType,
                           idBool        aTransformQuery,
                           SInt          aNumofTokens,
                           SInt          aHostValIndex,
                           SInt          aRemoveTokIndexs,
                           ulpHostDiagType aDiagType );

IDE_RC ulpValidateFORStructArray(ulpSymTElement *aElement);

IDE_RC ulpValidateFORGetDiagnostics(ulpSymTElement *aElement);

/*
 * parsing중에 처리하기 되는 host 변수들에 대한 정보를
 * 잠시 저장해두기 위한 class.
 */
class ulpParseInfo
{
    public:
        /* attributes */

        // id 를 저장할 거냐 말거냐 정보.
        idBool          mSaveId;
        // function 인자 선언부분인지 정보.
        idBool          mFuncDecl;
        // array depth
        SShort          mArrDepth;
        // array 선언부분인지 정보.
        idBool          mArrExpr;
        // constant_expr
        SChar           mConstantExprStr[MAX_EXPR_LEN];
        // structure node pointer 정보.
        ulpStructTNode *mStructPtr;
        // typedef 관련 정보.
        ulpSymTElement *mHostValInfo4Typedef;
        
        /* BUG-28118 : system 헤더파일들도 파싱돼야함.                       *
         * 6th. problem : Nested structure 정의중 scope를 잘못 계산하는 문제 */
        // host value, struct type 관련 정보 배열.
        // 중첩 구조체일경우를 처리하기 위해 배열로 선언됨.
        // ex>
        // int i;             <= mHostValInfo[0] 에 변수 정보 저장.
        // struct A           <= mHostValInfo[0]에 구조체 정보 저장, mHostValInfo[1] 동적 할당후 초기화.
        // {
        //    int a;          <= mHostValInfo[1] 에 변수 정보 저장.
        //    struct B        <= mHostValInfo[1]에 구조체 정보 저장, mHostValInfo[2] 동적 할당후 초기화.
        //    {
        //       int b;       <= mHostValInfo[2] 에 변수 정보 저장.
        //    } sB;           <= mHostValInfo[2] 해제.
        // } ;                <= mHostValInfo[1] 해제.
        ulpSymTElement *mHostValInfo[MAX_NESTED_STRUCT_DEPTH];
        // structure 선언부분 depth 정보.
        // mHostValInfo의 index로 사용됨.
        // ex> <depth 0>..struct {..<depth 1>..struct {..<depth 2>..}..}
        SShort          mStructDeclDepth;

        // varchar 변수 list
        iduList mVarcharVarList;
        // varchar 변수선언중인지 여부.
        idBool  mVarcharDecl;

        /* BUG-35518 Shared pointer should be supported in APRE */
        idBool  mIsSharedPtr;
        iduList mSharedPtrVarList;

        // typedef 처리를 skip한다.
        /* BUG-27875 : 구조체안의 typedef type인식못함. */
        // 아래 구문을 처리하기위해 mSkipTypedef 변수 추가됨.
        // typedef struct Struct1 Struct1;
        // struct Struct1       <- mSkipTypedef = ID_TRUE  :
        //                          Struct1은 비록 이전에 typedef되어 있지만 렉서에서 C_TYPE_NAME이아닌
        // {                        C_IDENTIFIER로 인식되어야 한다.
        //    ...               <- mSkipTypedef = ID_FALSE :
        //    ...                   필드에 typedef 이름이 오면 C_TYPE_NAME으로 인식돼야한다.
        // };
        idBool  mSkipTypedef;

        /* BUG-31648 : segv on windows */
        ulpErrorMgr mErrorMgr;

        /* functions */

        // constructor
        ulpParseInfo();

        // finalize
        void ulpFinalize(void);

        // host 변수 정보 초기화 함수.
        void ulpInitHostInfo(void);

        // typedef 구문 처리를 위한 ulpSymTElement copy 함수.
        void ulpCopyHostInfo4Typedef( ulpSymTElement *aD, ulpSymTElement *aS );

        /* print host variable infos. for debugging */
        void ulpPrintHostInfo(void);
};


//========= For Lexer ==========//


/*
 * COMPLexer에서 사용되는 internal function들.
 */

IDE_RC ulpCOMPInitialize( SChar *aFileName );

void   ulpCOMPFinalize();

/* 현재 buffer 정보 구조체 저장. (YY_CURRENT_BUFFER) */
void   ulpCOMPSaveBufferState( void );

/* C comment 처리 함수 */
IDE_RC ulpCOMPCommentC( idBool aSkip );

/* C++ comment 처리 함수 */
void   ulpCOMPCommentCplus( void );

/* lexer의 start condition을 원복해주는 함수. */
void   ulpCOMPRestoreCond(void);

/* 파싱 할 필요 없는 macro 구문을 skip해주는 함수. */
IDE_RC ulpCOMPSkipMacro(void);

/* macro 구문안의 '//n'문자를 제거 해주는 함수. */
void   ulpCOMPEraseBN4MacroText( SChar *aTmpDefinedStr, idBool aIsIf );

/* WHENEVER 구문 DO function_name 의 함수이름을 저장하는 함수. */
void   ulpCOMPWheneverFunc( SChar *aTmpStr );

/* EXEC SQL INCLUDE 의 헤더파일 이름을 저장하기위한 함수. */
void   ulpCOMPSetHeaderName( void );

/* BUG-28061 : preprocessing을마치면 marco table을 초기화하고, *
 *             ulpComp 에서 재구축한다.                       */
/* yyinput 대용 */
SChar  ulpCOMPYYinput(void);
/* unput 대용 */
void   ulpCOMPYYunput( SChar aCh );
/* #if안에 comment올경우 skip */
void   ulpCOMPCommentCplus4IF();
IDE_RC ulpCOMPCommentC4IF();

/* BUG-28250 : :다음에 변수이름이오는데 공백이 있어서는 안됩니다. */
// : <호스트변수이름> 에서 :와 이름사이의 공백을 제거해준다.
void   ulpCOMPEraseHostValueSpaces( SChar *aString );

/* BUG-28118 : system 헤더파일들도 파싱돼야함.     *
 * 3th. problem : 매크로 함수가 확장되지 않는 문제. */
// Macro 함수가 올경우 id 뒤의 (...) 토큰을 소모해주기 위한 함수.
void   ulpCOMPSkipMacroFunc( void );

#endif

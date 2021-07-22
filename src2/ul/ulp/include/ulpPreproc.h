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

#ifndef _ULP_PREPROC_H_
#define _ULP_PREPROC_H_ 1

#include <idl.h>
#include <ide.h>
#include <idn.h>
#include <iduMemory.h>
#include <ulpProgOption.h>
#include <ulpGenCode.h>
#include <ulpMacroTable.h>
#include <ulpScopeTable.h>
#include <ulpSymTable.h>
#include <ulpIfStackMgr.h>
#include <ulpErrorMgr.h>
#include <ulpMacro.h>


//========= For Parser ==========//

// parsing function at ulpPreprocy.y
int doPPparse( SChar *aFilename );



//========= For Lexer ==========//


/*
 * Lexer 사용되는 모든 function들을 갖는다.
 */

IDE_RC ulpPPInitialize( SChar *aFileName );

void   ulpPPFinalize();

/* 현재 buffer 정보 구조체 저장. (YY_CURRENT_BUFFER) */
void   ulpPPSaveBufferState( void );

/* C comment 처리 함수 */
IDE_RC ulpPPCommentC( void );

/* #if 안의 C comment 처리 함수 (newline만 file에 write한다.) */
IDE_RC ulpPPCommentC4IF( void );

/* C++ comment 처리 함수 */
void   ulpPPCommentCplus( void );

/* #if 안의 C++ comment 처리 함수 (file로 write 하지 않는다.)*/
void   ulpPPCommentCplus4IF( void );

/* macro 구문안의 '//n'문자를 제거 해주는 함수. */
void   ulpPPEraseBN4MacroText(SChar *aTmpDefinedStr, idBool aIsIf);

/* 처리 할필요 없는 macro 구문을 skip해주는 함수. */
IDE_RC ulpPPSkipUnknownMacro( void );

/* lexer의 start condition을 원복해주는 함수. */
void   ulpPPRestoreCond(void);

/* yyinput 대용 */
SChar  ulpPPYYinput(void);

/* unput 대용 */
void  ulpPPYYunput( SChar aCh );

/*
 * extern functions
 */

/* for delete output files when error occurs. */
extern void ulpFinalizeError();

#endif

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

#ifndef _ULP_MAIN_H_
#define _ULP_MAIN_H_ 1


#include <idl.h>
#include <ide.h>
#include <iduMemory.h>
#include <ulpErrorMgr.h>
#include <ulpProgOption.h>
#include <ulpGenCode.h>
#include <ulpMacroTable.h>
#include <ulpScopeTable.h>
#include <ulpStructTable.h>
#include <ulpMacro.h>



/****************************************
 * Global 변수 선언
 ****************************************/

/* command-line option정보를 갖는 객체. global하게 관리된다. */
ulpProgOption gUlpProgOption;
/* lexer와 parser에서 global하게 사용되는 code generator 객체. */
ulpCodeGen    gUlpCodeGen;
/* lexer token을 저장하기 위한 memory manager*/
iduMemory       *gUlpMem;
/* Symbol tables */
// Macro table
ulpMacroTable  gUlpMacroT;
// Scope table
ulpScopeTable  gUlpScopeT;
// Struct Table
ulpStructTable gUlpStructT;

/* 에러 발생시 수행코드 부분에 따라 file제거 여부를 결정. */
SInt          gUlpErrDelFile = ERR_DEL_FILE_NONE;



/* functions */
void ulpFinalizeError(void);

#endif

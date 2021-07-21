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

#ifndef _ULP_SCOPETABLE_H_
#define _ULP_SCOPETABLE_H_ 1

#include <idl.h>
#include <ide.h>
#include <ulpSymTable.h>
#include <ulpStructTable.h>
#include <ulpErrorMgr.h>
#include <ulpHash.h>
#include <ulpTypes.h>
#include <ulpMacro.h>

/* BUG-45289 symblos은 SCOPE가 없는 0 ~ 100까지 허용되므로 
 * mScopeTable Size는 MAX_SCOPE_DEPTH + 1 사용하도록 한다 */
#define SCOPE_TABLE_SIZE MAX_SCOPE_DEPTH + 1 

/******************************
 * ulpScopeTable
 * host variable의 scope 관리를 위한 class
 ******************************/
class ulpScopeTable
{

public:
    ulpScopeTable();
    ~ulpScopeTable();

    void ulpInit();

    void ulpFinalize();

    // host variable를 m_SymbolTable에 저장한다.
    ulpSymTNode    *ulpSAdd ( ulpSymTElement *aSym, SInt aScope );

    // 특정 이름을 갖는 변수를 symbol table에서 scope를 줄여가면서 검색한다.
    ulpSymTElement *ulpSLookupAll( SChar *aName, SInt aScope );

    // 같은 scope상의 모든 변수정보를 symbol table에서 제거한다.
    void            ulpSDelScope( SInt aScope );

    // print symbol table for debug
    void            ulpPrintAllSymT( void );

    // scope depth 관리 함수들.
    SInt            ulpGetDepth(void);
    void            ulpSetDepth(SInt aDepth);
    void            ulpIncDepth(void);
    void            ulpDecDepth(void);

private:

    // current scope depth value
    SInt mCurDepth;

    // scope table for symbols
    ulpSymTable *mScopeTable[SCOPE_TABLE_SIZE];
};

#endif

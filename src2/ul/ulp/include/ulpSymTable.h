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

#ifndef _ULP_SYMTABLE_H_
#define _ULP_SYMTABLE_H_ 1

#include <idl.h>
#include <ide.h>
#include <ulpStructTable.h>
#include <ulpErrorMgr.h>
#include <ulpHash.h>
#include <ulpTypes.h>
#include <ulpMacro.h>

struct ulpStructTNode;

/* infomation for host variable */
typedef struct ulpSymTElement
{
    SChar            mName[MAX_HOSTVAR_NAME_SIZE];   // var name
    ulpHostType      mType;                          // variable type
    idBool           mIsTypedef;                     // typdef로 정의된 type 이름이냐?
    idBool           mIsarray;
    SChar            mArraySize[MAX_NUMBER_LEN];
    SChar            mArraySize2[MAX_NUMBER_LEN];
    idBool           mIsstruct;
    SChar            mStructName[MAX_HOSTVAR_NAME_SIZE];  // struct tag name
    ulpStructTNode  *mStructLink;             // struct type일경우 link
    idBool           mIssign;                 // unsigned or signed
    SShort           mPointer;
    idBool           mAlloc;                  // application에서 직접 alloc했는지 여부.
    UInt             mMoreInfo;               // Some additional infomation.
    /* BUG-28118 : system 헤더파일들도 파싱돼야함.                                 *
     * 8th. problem : can't resolve extern variable type at declaring section. */
    idBool           mIsExtern;               // is extern variable?

    /* TASK-7218 Handling Multiple Errors */
    ulpHostDiagType  mDiagType;               // diagnotics information type
} ulpSymTElement;

typedef struct ulpSymTNode
{
    ulpSymTElement mElement;
    ulpSymTNode   *mNext;
    ulpSymTNode   *mInOrderNext;
} ulpSymTNode;


/******************************
 * ulpSymTable
 * host variable의 관리를 위한 class
 ******************************/
class ulpSymTable
{
/* METHODS */
public:
    ulpSymTable();
    ~ulpSymTable();

    void ulpInit();

    void ulpFinalize();

    // host variable를 Symbol Table에 저장한다.
    ulpSymTNode *   ulpSymAdd ( ulpSymTElement *aSym );

    // 특정 이름을 갖는 변수를 symbol table에서 검색한다.
    ulpSymTElement *ulpSymLookup( SChar *aName );

    // 특정 이름을 갖는 변수를 symbol table에서 제거한다.
    void            ulpSymDelete( SChar *aName );

    // print symbol table for debug
    void            ulpPrintSymT( SInt aScopeD );

/* ATTRIBUTES */
public:
    SInt mCnt;  // m_SymbolTable에 저장된 host variables의 개수
    SInt mSize; // max number of symbol table buckets

    ulpSymTNode *mInOrderList;  // structure의 field 선언 순서대로 순회하기 위한 list.
                                // 코드 생성시 사용됨.

private:
    UInt     (*mHash) (UChar *);       /* hash function */

    // mSymbolTable : host variables을 저장할 symbol table(hash table)
    ulpSymTNode *mSymbolTable[MAX_SYMTABLE_ELEMENTS];
};

#endif

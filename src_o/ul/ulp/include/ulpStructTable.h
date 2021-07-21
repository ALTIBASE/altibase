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

#ifndef _ULP_STRUCTTABLE_H_
#define _ULP_STRUCTTABLE_H_ 1

#include <idl.h>
#include <ide.h>
#include <ulpSymTable.h>
#include <ulpErrorMgr.h>
#include <ulpHash.h>
#include <ulpTypes.h>
#include <ulpMacro.h>

class ulpSymTable;

typedef struct ulpStructTNode
{
    // struct tag name
    SChar             mName[MAX_HOSTVAR_NAME_SIZE];
    // struct field symbol table.
    ulpSymTable      *mChild;
    // scope depth
    SInt              mScope;
    // doubly linked-list
    ulpStructTNode   *mNext;
    ulpStructTNode   *mPrev;
    // Scope link (list)
    ulpStructTNode   *mSLink;
    // bucket index
    // bucket-list 처음 node일 경우 mPrev가 null이기 때문에
    // ulpStructDelScope()에서 다음 변수가 사용된다.
    UInt              mIndex;
} ulpStructTNode;


/******************************
 *
 * structure type을 저장, 관리한기 위한 class
 ******************************/
class ulpStructTable
{
/* Method */
public:
    ulpStructTable();
    ~ulpStructTable();

    void ulpInit();

    void ulpFinalize();

    // struct table에 새로운 tag를 저장한다.
    ulpStructTNode *ulpStructAdd ( SChar *aTag, SInt aScope );

    // tag가 없는 경우 struct table 마지막 buket에 저장한다.
    ulpStructTNode *ulpNoTagStructAdd ( void );

    // 특정 이름을 갖는 tag를 struct table에서 scope를 줄여가면서 검색한다.
    ulpStructTNode *ulpStructLookupAll( SChar *aTag, SInt aScope );

    // 특정 이름을 갖는 tag를 특정 scope에서 존재하는지 struct table을 검색한다.
    ulpStructTNode *ulpStructLookup( SChar *aTag, SInt aScope );

    // 특정 이름을 갖는 tag를 struct table에서 제거한다.
    //void            ulpStructDelete( SChar *aTag, SInt aScope );

    // 같은 scope상의 모든 변수정보를 struct table에서 제거한다.
    void            ulpStructDelScope( SInt aScope );

    // print struct table for debug
    void            ulpPrintStructT( void );

/* ATTRIBUTES */
public:
    SInt mCnt;  // m_SymbolTable에 저장된 host variables의 개수
    SInt mSize; // max number of symbol table buckets

private:
    UInt     (*mHash) (UChar *);       /* hash function */

    // Struct table
    ulpStructTNode* mStructTable[MAX_SYMTABLE_ELEMENTS + 1];
    // struct Scope table
    ulpStructTNode* mStructScopeT[MAX_SCOPE_DEPTH + 1];
};

#endif

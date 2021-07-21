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

#ifndef _ULP_MACROTABLE_H_
#define _ULP_MACROTABLE_H_ 1

#include <idl.h>
#include <ide.h>
#include <ulpErrorMgr.h>
#include <ulpHash.h>
#include <ulpTypes.h>
#include <ulpMacro.h>

// the node for macro hash table
typedef struct ulpMacroNode
{
    // macro define name
    SChar  mName[MAX_MACRO_DEFINE_NAME_LEN];
    // macro define text
    SChar  mText[MAX_MACRO_DEFINE_CONTENT_LEN];
    // Is it macro function?
    idBool mIsFunc;

    ulpMacroNode *mNext;
} ulpMacroNode;


/******************************
 *
 * macro를 저장, 관리한기 위한 class
 ******************************/
class ulpMacroTable
{
/* METHODS */
public:
    ulpMacroTable();
    ~ulpMacroTable();

    void ulpInit();

    void ulpFinalize();

    // defined macro를 hash table에 저장한다.
    IDE_RC          ulpMDefine ( SChar *aName, SChar *aText, idBool aIsFunc );

    // 특정 이름을 갖는 defined macro를 hash table에서 검색한다.
    ulpMacroNode   *ulpMLookup( SChar *aName );

    // 특정 이름을 갖는 defined macro를 hash table에서 제거한다.
    void            ulpMUndef( SChar *aName );

    /* BUG-28118 : system 헤더파일들도 파싱돼야함.                    *
     * 11th. problem : C preoprocessor에서 매크로 함수 인자 처리 못함. *
     * 12th. problem : C preprocessor에서 두 토큰을 concatenation할때 사용되는 '##' 토큰 처리해야함. */
     void ulpMEraseSharp4MFunc( SChar *aText );

    // for debugging
    void            ulpMPrint( void );

private:

/* ATTRIBUTES */
public:
    // ulpMacroTable 저장된 defined macro의 개수
    SInt mCnt;

    // max number of ulpMacroTable buckets
    SInt mSize;

private:
    // hash function
    UInt     (*mHash) (UChar *);

    // marco를 저장할 hash table
    ulpMacroNode* mMacroTable[MAX_SYMTABLE_ELEMENTS];
};

#endif

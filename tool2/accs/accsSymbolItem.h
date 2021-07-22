/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef _O_ACCS_SYMBOL_ITEM_H_
#define _O_ACCS_SYMBOL_ITEM_H_ 1

#include <idl.h>

typedef enum
{
    SCOPE_NONE   = 0,
    SCOPE_GLOBAL,
    SCOPE_LOCAL,
    SCOPE_CLASS
    
} SYMBOL_SCOPE;

typedef enum
{
    KIND_NONE   = 0,
    KIND_FUNC,
    KIND_VAR,
    KIND_MACRO,
    KIND_CLASS,
    KIND_TYPEDEF
    
} SYMBOL_KIND;

typedef enum
{
    DATATYPE_NONE   = 0,
    DATATYPE_SCHAR,
    DATATYPE_UCHAR,
    DATATYPE_SSHORT,
    DATATYPE_USHORT,
    DATATYPE_SINT,
    DATATYPE_UINT,
    DATATYPE_SLONG,
    DATATYPE_ULONG,
    DATATYPE_VOID,
    DATATYPE_TYPEDEF // 생성된 데이타 타입
    
} SYMBOL_DATATYPE;

typedef struct
{
    idBool isArgument; // 인자 존재?
    SChar *target_pattern; // 패턴 리스트 
} MacroData;

typedef struct
{
    SYMBOL_DATATYPE return_type;
    SYMBOL_DATATYPE arguemnt_type[128];
} FuncData;

typedef struct
{
    SYMBOL_DATATYPE  type;
    UInt             pointer; // pointer count 
    UInt             depth; // local variable depth 
} VarData;

typedef struct
{
    SChar className[256];
} ClassData;

class accsSymbolItem
{
    friend class accsSymbolMgr;
    SChar       *name_;
    SYMBOL_SCOPE scope_;
    SYMBOL_KIND  kind_;
    union 
    {
        MacroData macroData_;
        FuncData  funcData_;
        VarData   varData_;
        ClassData classData_;
    } kind_data;
    
    accsSymbolItem *next_;
    
public:
    accsSymbolItem(SYMBOL_SCOPE, SChar *name, SYMBOL_KIND);
    
    MacroData* getMacroData();
    FuncData*  getFuncData();
    VarData*   getVarData();
    ClassData* getClassData();
};


#endif

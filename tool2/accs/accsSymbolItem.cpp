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
 
#include <idl.h>
#include <ideErrorMgr.h>
#include <accsSymbolMgr.h>

accsSymbolItem::accsSymbolItem(SYMBOL_SCOPE scope, SChar *name, SYMBOL_KIND kind)
{
    name_  = idlOS::strdup(name);
    scope_ = scope;
    kind_  = kind;
}


MacroData* accsSymbolItem::getMacroData()
{
    return &kind_data.macroData_;
}

FuncData*  accsSymbolItem::getFuncData()
{
    return &kind_data.funcData_;
}

VarData*   accsSymbolItem::getVarData()
{
    return &kind_data.varData_;
}

ClassData* accsSymbolItem::getClassData()
{
    return &kind_data.classData_;
}

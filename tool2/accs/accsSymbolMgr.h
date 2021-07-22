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
 
#ifndef _O_ACCS_SYMBOL_MGR_H_
#define _O_ACCS_SYMBOL_MGR_H_ 1

#include <idl.h>
#include <accsSymbolItem.h>

class accsSymbolMgr
{
    accsSymbolItem *baseSymbol_;
    accsSymbolItem *currSymbol_;
public:
    accsSymbolMgr();
    SInt loadSymbolFile(SChar *file);
    // position

    void resetFindPosition();
    
    void addSymbolItem(accsSymbolItem *);
    
    accsSymbolItem* findSymbolItem(SYMBOL_SCOPE,
                                   SChar *name,
                                   SYMBOL_KIND);

    void dump();
private:
    void setLexStdin(FILE *fd);
    SInt doParse();
};

extern accsSymbolMgr accsg_symbol;

#endif

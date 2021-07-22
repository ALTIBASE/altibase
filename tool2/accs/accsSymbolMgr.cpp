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

accsSymbolMgr::accsSymbolMgr()
{
    baseSymbol_ = NULL;
    currSymbol_ = NULL;
}

SInt accsSymbolMgr::loadSymbolFile(SChar *filename)
{
    FILE *fp;

    fp = idlOS::fopen(filename, "r");
    IDE_TEST_RAISE(fp == NULL, fopen_error);

    setLexStdin(fp);

    IDE_TEST_RAISE(doParse() != IDE_SUCCESS, get_property_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(fopen_error);
    {
        idlOS::printf("open symbol file error[%s]\n", filename);
    }
    IDE_EXCEPTION(get_property_error);
    {
        idlOS::printf("get symbol error in parser\n");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}


void accsSymbolMgr::resetFindPosition()
{
    currSymbol_ = baseSymbol_;
}

void accsSymbolMgr::addSymbolItem(accsSymbolItem *item)
{
    if (baseSymbol_ == NULL)
    {
        baseSymbol_  = item;
        item->next_  = NULL;
    }
    else
    {
        accsSymbolItem *tmp = baseSymbol_;
        baseSymbol_         = item;
        item->next_         = tmp;
    }
}

/* and Á¶°ÇÀÓ */
accsSymbolItem* accsSymbolMgr::findSymbolItem(SYMBOL_SCOPE scope,
                                              SChar *name,
                                              SYMBOL_KIND kind)
{
    SInt i;
    accsSymbolItem *cur = NULL;
    
    for (;currSymbol_ != NULL; currSymbol_ = currSymbol_->next_)
    {
        if (scope != SCOPE_NONE)
        {
            if (currSymbol_->scope_ != scope)
            {
                continue;
            }
        }

        if (name != NULL)
        {
            if (idlOS::strcmp(name, currSymbol_->name_) != 0)
            {
                continue;
            }
        }

        if (kind != KIND_NONE)
        {
            if (currSymbol_->kind_ != kind)
            {
                continue;
            }
        }
        cur = currSymbol_;
        break;
    }
    return cur;
}



void accsSymbolMgr::dump()
{
    static const SChar *_scope_name_[] =
    {
        "NONE", "GLOBAL", "LOCAL", "CLASS"
    };
    
    static const SChar *_kind_name_[] =
    {
        "NONE", "FUNC", "VAR",
        "MACRO", "CLASS", "TYPEDEF"
    };
    
    accsSymbolItem *cur = NULL;

    resetFindPosition();
    
    idlOS::printf(" =================== DUMP OF SYMBOL TABLE ================= \n");
    
    for (cur = baseSymbol_; cur != NULL; cur = cur->next_)
    {
        idlOS::printf("%10s %10s %10s \n",
                      _scope_name_[cur->scope_],
                      cur->name_,
                      _kind_name_[cur->kind_]);
    }
    idlOS::printf(" =================== END OF SYMBOL TABLE ================= \n");
}

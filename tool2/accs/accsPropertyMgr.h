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
 
#ifndef _O_ACCS_PROPERTY_MGR_H_
#define _O_ACCS_PROPERTY_MGR_H_ 1

#include <idl.h>

#define ACCS_CONF_FILE_LEN  256
#define ACCS_MAX_PREFIX_LEN 8

#define ACCS_MAX_PRELOAD_LIST 128
#define ACCS_MAX_PREFIX_LIST 128

#define ACCS_MAX_ITEM_COUNT  1024 // for yacc
#define ACCS_MAX_ITEM_LEN    128 // for yacc


class accsPropertyMgr
{
    SChar Filename_[ACCS_CONF_FILE_LEN];

    UInt  PreloadCount_;
    UInt  PrefixCount_;
    SChar PreloadList_[ACCS_MAX_PRELOAD_LIST][ACCS_CONF_FILE_LEN];
    SChar PrefixList_[ACCS_MAX_PRELOAD_LIST][ACCS_MAX_PREFIX_LEN];
    SInt  TabSize_;
    SInt  LineWidth_;
    SInt  ColStart_;

    
public:
    accsPropertyMgr();
    void setPropertyFile(SChar *filename);
    SInt doIt();

    void   setPreloadCount(SInt count) { PreloadCount_ = count; }
    void   setPrefixCount(SInt count)  { PrefixCount_ = count;  }
    void   setTabSize(SInt size)       { TabSize_ = size;       }
    void   setLineWidth(SInt width)    { LineWidth_ = width;    }
    void   setColStart(SInt start)     { ColStart_ = start;     }
    SInt   addPreloadItem(SChar *name);
    SInt   addPrefixItem (SChar *name);
    
    SInt   getPreloadCount()           { return PreloadCount_;  }
    SInt   getPrefixCount()            { return PrefixCount_;   }
    SInt   getTabSize()                { return TabSize_;       }
    SInt   getLineWidth()              { return LineWidth_;     }
    SInt   getColStart()               { return ColStart_;      }
    SChar *getPreloadItem(SInt number);
    SChar *getPrefixItem(SInt number);

    void   dump();
    
private:    
    void setLexStdin(FILE *); // call lexer api
    SInt doParse();           // call yacc(bison) api
};

extern accsPropertyMgr accsg_property;

#endif

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
 
%{
#include <idl.h>
#include <accsPropertyMgr.h>

extern int property_lex();
void property_error(SChar *);

#define YACC_DEBUG(a)  idlOS::printf("%s\n", a);

#ifdef YACC_DEBUG
SInt prop_dump(const SChar *msg);
#define PROP_DUMP(msg)  prop_dump(msg)
#else
#define YACC_DUMP  
#endif

SInt ItemCount = 0;

%}

%union
{
    SChar  name[128];
    SInt   number;
    double fnumber;
    SChar  *ItemList_[ACCS_MAX_ITEM_COUNT];
}


%token PRELOAD_LIST
%token IDENTIFIER /* file name to preload */
%token NUMBER     /* non-float number */
%token FNUMBER    /* float number */
%token STRING

%token TAB_SIZE
%token PREFIX_LIST
%token LINE_WIDTH
%token COL_START
%token END_OF_LINE
%token PREFIX_LIST
%token END_OF_FILE

%start property_list
%%

property_list :
    property_list property_item {
        //printf("*****************************************************************************\n\n");
    }
    | property_item {
        //printf("*****************************************************************************\n\n");
    }

property_item :
    '\n' { }

    | PRELOAD_LIST '=' file_list '\n'   {
        SInt i;
        //printf("PRELOAD_LIST '=' file_list [%d]\\n \n", ItemCount);
        accsg_property.setPreloadCount(0);
        for (i = 0; i < ItemCount; i++)
        {
            accsg_property.addPreloadItem($<ItemList_>3[i]);
            //printf("Preload Item %s %s\n", $<ItemList_>3[i], accsg_property.getPreloadItem(i));
        }
        
    }

    | PREFIX_LIST '=' file_list '\n'   {
        SInt i;
        //printf("PREFIX_LIST '=' file_list \\n \n");
        accsg_property.setPrefixCount(0);
        for (i = 0; i < ItemCount; i++)
        {
            accsg_property.addPrefixItem($<ItemList_>3[i]);
            //printf("Prefix Item %s\n", accsg_property.getPrefixItem(i));
        }
    }

    | TAB_SIZE '=' NUMBER '\n' {
        //printf("TAB_SIZE = %d \n", $<number>3);
        accsg_property.setTabSize($<number>3);
        
    }

    | LINE_WIDTH '=' NUMBER '\n' {
        //printf("LINE_WIDTH = %d \n", $<number>3);
        accsg_property.setLineWidth($<number>3);
        
    }

    | COL_START '=' NUMBER '\n' {
        //printf("COL_START = %d \n", $<number>3);
        accsg_property.setColStart($<number>3);
    }

file_list :
    IDENTIFIER {
    //printf( "IDENTIFIER [%s]\n", $<name>1);
        ItemCount = 0;
//         idlOS::memset($<ItemList_>$, 0, ACCS_MAX_ITEM_LEN * ACCS_MAX_ITEM_COUNT);
//         idlOS::strcpy($<ItemList_>$[ItemCount++], $<name>1);
        $<ItemList_>$[ItemCount++] = idlOS::strdup($<name>1);
	}
    | file_list IDENTIFIER {
        //printf( "file_list IDENTIFIER [%s]\n", $<name>2);
        //idlOS::strcpy($<ItemList_>$[ItemCount++], $<name>2);
        $<ItemList_>$[ItemCount++] = idlOS::strdup($<name>2);
    }
    
%%
void property_error(SChar *msg)
{
    idlOS::printf("property syntax error[%s]\n", msg);
}

SInt accsPropertyMgr::doParse()
{
    return (yyparse() == 0 ? IDE_SUCCESS : IDE_FAILURE);
}
    

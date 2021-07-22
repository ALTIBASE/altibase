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
#include <accsSymbolMgr.h>
#include <accsSymbolItem.h>

extern int symbol_lex();
void symbol_error(SChar *);

#define YACC_DEBUG(a)  idlOS::printf("%s\n", a);

#ifdef YACC_DEBUG
SInt prop_dump(const SChar *msg);
#define PROP_DUMP(msg)  prop_dump(msg)
#else
#define YACC_DUMP  
#endif

%}

%union
{
    SChar  name[128];
    SInt   number;
    double fnumber;

    SYMBOL_SCOPE    scope;
    SYMBOL_KIND     kind;
    SYMBOL_DATATYPE datatype;

    MacroData       macro;
    FuncData        func;
    VarData         var;
    ClassData       classdata;
}

%token IDENTIFIER /* name */
%token NUMBER     /* non-float number */
%token FNUMBER    /* float number */
%token STRING

%token TK_CLASS
%token TK_SCOPE_GLOBAL
%token TK_SCOPE_LOCAL

%token TK_KIND_FUNC
%token TK_KIND_VAR
%token TK_KIND_MACRO
%token TK_KIND_TYPEDEF
%token TK_POINTER

%token TK_DATATYPE_SCHAR
%token TK_DATATYPE_UCHAR
%token TK_DATATYPE_SSHORT
%token TK_DATATYPE_USHORT
%token TK_DATATYPE_SINT
%token TK_DATATYPE_UINT
%token TK_DATATYPE_SLONG
%token TK_DATATYPE_ULONG
%token TK_DATATYPE_VOID
%token TK_DATATYPE_TYPEDEF

%start symbol_list
%%

symbol_list :
    symbol_list symbol_item {
        //printf("*************************************** list item \n\n");
    }
    | symbol_item {
        //printf("******************************************** item \n\n");
    }

symbol_item :
    scope_item name_item kind_item '\n' {
        accsSymbolItem *p    = new accsSymbolItem($<scope>1, $<name>2, $<kind>3);
        accsSymbolItem *same;

        assert( p != NULL);
        accsg_symbol.resetFindPosition();
        same = accsg_symbol.findSymbolItem($<scope>1, $<name>2, KIND_NONE);
        if ( same != NULL)
        {
            idlOS::printf("[WARNING] same symbol added!! [%d] [%s]\n", $<scope>1, $<name>2);
        }
        accsg_symbol.addSymbolItem(p);
	}
    | '\n' { /* printf("just \\n \n"); */}

name_item :
	IDENTIFIER { /*printf("name_item IDENTIFIER \n");*/ }

scope_item :
	TK_SCOPE_GLOBAL {
        $<scope>$ = SCOPE_GLOBAL;
     	//printf("TK_SCOPE_GLOBAL \n");
	}
    | TK_SCOPE_LOCAL {
        $<scope>$ = SCOPE_LOCAL;
        //printf("TK_SCOPE_LOCAL \n");
    }
    | TK_CLASS {
        $<scope>$ = SCOPE_CLASS;
        //printf("TK_SCOPE_CLASS \n");
    }

kind_item :
	TK_KIND_FUNC      '{' func_arg_list '}'    {
        //printf("TK_KIND_FUNC \n");
        $<kind>$ = KIND_FUNC;
        // add list info
	}
    | TK_KIND_VAR     '{' var_arg_list '}'     {
        //printf("TK_KIND_VAR \n");
        $<kind>$ = KIND_VAR;
        // add list info
    }
    | TK_KIND_MACRO   '{' macro_arg_list '}'   {
        //printf("TK_KIND_MACRO \n");
        $<kind>$ = KIND_MACRO;
        // add list info
    }
    | TK_CLASS        '{' class_arg_list '}'   {
        //printf("TK_KIND_CLASS \n");
        $<kind>$ = KIND_CLASS;
        // add list info
    }
    | TK_KIND_TYPEDEF '{' datatype_list '}'    {
        //printf("TK_KIND_TYPEDEF \n");
        $<kind>$ = KIND_TYPEDEF;
        // add list info
    }

macro_arg_list:
    NUMBER /* count of argument */ { /*printf("NUMBER\n");*/ }

func_arg_list:
    datatype_list { /*printf(" \n");*/ }

var_arg_list:
    datatype { /*printf("datatype \n");*/ }

class_arg_list:
    IDENTIFIER /* name of class */ { /*printf("class_arg_list IDENTIFIER \n");*/ }


datatype_list:
    datatype  {
        /*printf("datatype pointer_opt \n");*/
    }
    | datatype_list datatype pointer_opt {
        /*printf("datatype_list datatype pointer_opt \n"); */
    }

datatype:
    TK_DATATYPE_SCHAR    pointer_opt  {
        //printf("TK_DATATYPE_SCHAR   pointer_opt\n");
        $<datatype>$ = DATATYPE_SCHAR;
        // add pointer info 
    }
    |TK_DATATYPE_UCHAR   pointer_opt  {
        //printf("TK_DATATYPE_UCHAR   pointer_opt\n");
        $<datatype>$ = DATATYPE_UCHAR;
        // add pointer info 
    }
    |TK_DATATYPE_SSHORT  pointer_opt  {
        //printf("TK_DATATYPE_SSHORT  pointer_opt\n");
        $<datatype>$ = DATATYPE_SSHORT;
        // add pointer info 
    }
    |TK_DATATYPE_USHORT  pointer_opt  {
        //printf("TK_DATATYPE_USHORT  pointer_opt\n");
        $<datatype>$ = DATATYPE_USHORT;
        // add pointer info 
    }
    |TK_DATATYPE_SINT    pointer_opt  {
        //printf("TK_DATATYPE_SINT    pointer_opt\n");
        $<datatype>$ = DATATYPE_SINT;
        // add pointer info 
    }
    |TK_DATATYPE_UINT    pointer_opt  {
        // printf("TK_DATATYPE_UINT    pointer_opt\n");
        $<datatype>$ = DATATYPE_UINT;
        // add pointer info 
    }
    |TK_DATATYPE_SLONG   pointer_opt  {
        // printf("TK_DATATYPE_SLONG   pointer_opt\n");
        $<datatype>$ = DATATYPE_SLONG;
        // add pointer info 
    }
    |TK_DATATYPE_ULONG   pointer_opt  {
        // printf("TK_DATATYPE_ULONG   pointer_opt\n");
        $<datatype>$ = DATATYPE_ULONG;
        // add pointer info 
    }
    |TK_DATATYPE_VOID    pointer_opt  {
        // printf("TK_DATATYPE_VOID    pointer_opt\n");
        $<datatype>$ = DATATYPE_VOID;
        // add pointer info 
    }
    |TK_DATATYPE_TYPEDEF pointer_opt  {
        // printf("TK_DATATYPE_TYPEDEF pointer_opt\n");
        $<datatype>$ = DATATYPE_TYPEDEF;
        // add pointer info 
    }

pointer_opt:
    /* nothing*/ { /* printf("pointer_opt nothing.. \n", $<name>1); */}
    | TK_POINTER { /* printf("pointer_opt[%s] \n", $<name>1);*/ }


%%
void symbol_error(SChar *msg)
{
    idlOS::printf("property syntax error[%s]\n", msg);
}

SInt accsSymbolMgr::doParse()
{
    return (yyparse() == 0 ? IDE_SUCCESS : IDE_FAILURE);
}
    

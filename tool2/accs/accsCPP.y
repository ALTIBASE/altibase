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

/*****************************************************************************
 * $Id: accsCPP.y 82075 2018-01-17 06:39:52Z jina.kim $
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <accs.h>    
#include <ideErrorMgr.h>
    
int CPP_error(char * string);
extern int      CPP_lex();
#define YYDEBUG 1
#define YYERROR_VERBOSE 1

%}
%union
{
    char *text;
}
/*

Interesting ambiguity:
Usually
        typename ( typename2 ) ...
or
        typename ( typename2 [4] ) ...
etc.
is a redeclaration of typename2.

Inside  a structure elaboration, it is sometimes the declaration of a 
constructor!  Note, this only  counts  if  typename  IS  the  current 
containing  class name. (Note this can't conflict with ANSI C because 
ANSI C would call it a redefinition, but  claim  it  is  semantically 
illegal because you can't have a member declared the same type as the 
containing struct!) Since the ambiguity is only reached when a ';' is 
found,   there  is  no  problem  with  the  fact  that  the  semantic 
interpretation  is  providing  the  true  resolution.   As  currently 
implemented, the constructor semantic actions must be able to process 
an  ordinary  declaration.  I may reverse this in the future, to ease 
semantic implementation.

*/



/*

INTRO TO ANSI C GRAMMAR (provided in a separate file):

The refined grammar resolves several typedef ambiguities in the draft 
proposed ANSI C standard syntax down to 1 shift/reduce  conflict,  as 
reported by a YACC process.  Note that the one shift reduce conflicts 
is  the  traditional  if-if-else conflict that is not resolved by the 
grammar.  This ambiguity can be removed using the method described in 
the Dragon Book (2nd edition), but this does  not  appear  worth  the 
effort.

There  was quite a bit of effort made to reduce the conflicts to this 
level, and an additional effort was made to make  the  grammar  quite 
similar  to  the  C++ grammar being developed in parallel.  Note that 
this grammar resolves the following ANSI C ambiguities:

ANSI C section 3.5.6, "If the [typedef  name]  is  redeclared  at  an 
inner  scope,  the  type specifiers shall not be omitted in the inner 
declaration".  Supplying type specifiers prevents consideration of  T 
as a typedef name in this grammar.  Failure to supply type specifiers 
forced the use of the TYPEDEFname as a type specifier.  This is taken 
to an (unnecessary) extreme by this implementation.  The ambiguity is 
only  a  problem  with  the first declarator in a declaration, but we 
restrict  ALL  declarators  whenever  the  users  fails  to   use   a 
type_specifier.

ANSI C section 3.5.4.3, "In a parameter declaration, a single typedef 
name  in  parentheses  is  taken  to  be  an abstract declarator that 
specifies a function  with  a  single  parameter,  not  as  redundant 
parentheses  around  the  identifier".  This is extended to cover the 
following cases:

typedef float T;
int noo(const (T[5]));
int moo(const (T(int)));
...

Where again the '(' immediately to the left of 'T' is interpreted  as 
being  the  start  of  a  parameter type list, and not as a redundant 
paren around a redeclaration of T.  Hence an equivalent code fragment 
is:

typedef float T;
int noo(const int identifier1 (T identifier2 [5]));
int moo(const int identifier1 (T identifier2 (int identifier3)));
...

*/


%{
/*************** Includes and Defines *****************************/
#define YYDEBUG_LEXER_TEXT (yylval) /* our lexer loads this up each time.
				     We are telling the graphical debugger
				     where to find the spelling of the 
				     tokens.*/
#define YYDEBUG 1        /* get the pretty debugging code to compile*/
#define YYSTYPE  char *  /* interface with flex: should be in header file */

/*************** Standard ytab.c continues here *********************/
%}

/*************************************************************************/


/* This group is used by the C/C++ language parser */
%token AUTO            DOUBLE          INT             STRUCT
%token BREAK           ELSE            LONG            SWITCH
%token CASE            ENUM            REGISTER        TYPEDEF
%token CHAR            EXTERN          RETURN          UNION
%token CONST           FLOAT           SHORT           UNSIGNED
%token CONTINUE        FOR             SIGNED          VOID
%token DEFAULT         GOTO            SIZEOF          VOLATILE
%token DO              IF              STATIC          WHILE

/* The following are used in C++ only.  ANSI C would call these IDENTIFIERs */
%token NEW             DELETE
%token THIS
%token OPERATOR
%token CLASS
%token PUBLIC          PROTECTED       PRIVATE
%token VIRTUAL         FRIEND
%token INLINE          OVERLOAD

/* ANSI C Grammar suggestions */
%token IDENTIFIER              STRINGliteral
%token FLOATINGconstant        INTEGERconstant        CHARACTERconstant
%token OCTALconstant           HEXconstant

/* New Lexical element, whereas ANSI C suggested non-terminal */
%token TYPEDEFname

/* Multi-Character operators */
%token  ARROW            /*    ->                              */
%token  ICR DECR         /*    ++      --                      */
%token  LS RS            /*    <<      >>                      */
%token  LE GE EQ NE      /*    <=      >=      ==      !=      */
%token  ANDAND OROR      /*    &&      ||                      */
%token  ELLIPSIS         /*    ...                             */
                 /* Following are used in C++, not ANSI C        */
%token  CLCL             /*    ::                              */
%token  DOTstar ARROWstar/*    .*       ->*                    */

/* modifying assignment operators */
%token MULTassign  DIVassign    MODassign   /*   *=      /=      %=      */
%token PLUSassign  MINUSassign              /*   +=      -=              */
%token LSassign    RSassign                 /*   <<=     >>=             */
%token ANDassign   ERassign     ORassign    /*   &=      ^=      |=      */

/*************************************************************************/

%start translation_unit

/*************************************************************************/

%%

/*********************** CONSTANTS *********************************/
constant:
        INTEGERconstant { printf("INTEGERconstant - %d\n", __LINE__); }
        | FLOATINGconstant { printf("FLOATINGconstant - %d\n", __LINE__); }
        /*  We  are not including ENUMERATIONconstant here because we 
          are treating it like a variable with a type of "enumeration 
          constant".  */
        | OCTALconstant { printf("OCTALconstant - %d\n", __LINE__); }
        | HEXconstant { printf("HEXconstant - %d\n", __LINE__); }
        | CHARACTERconstant { printf("CHARACTERconstant - %d\n", __LINE__); }
        ;

string_literal_list:
        STRINGliteral { printf("STRINGliteral - %d\n", __LINE__); }
        | string_literal_list STRINGliteral { printf("string_literal_list STRINGliteral - %d\n", __LINE__); }
                ;


/************************* EXPRESSIONS ********************************/


    /* Note that I provide  a  "scope_opt_identifier"  that  *cannot* 
    begin  with ::.  This guarantees we have a viable declarator, and 
    helps to disambiguate :: based uses in the grammar.  For example:

            ...
            {
            int (* ::b()); // must be an expression
            int (T::b); // officially a declaration, which fails on constraint grounds

    This *syntax* restriction reflects the current syntax in the ANSI 
    C++ Working Papers.   This  means  that  it  is  *incorrect*  for 
    parsers to misparse the example:

            int (* ::b()); // must be an expression

    as a declaration, and then report a constraint error.

    In contrast, declarations such as:

        class T;
        class A;
        class B;
        main(){
              T( F());  // constraint error: cannot declare local function
              T (A::B::a); // constraint error: cannot declare member as a local value

    are  *parsed*  as  declarations,  and *then* given semantic error 
    reports.  It is incorrect for a parser to "change its mind" based 
    on constraints.  If your C++ compiler claims  that  the  above  2 
    lines are expressions, then *I* claim that they are wrong. */

paren_identifier_declarator:
        scope_opt_identifier { printf("scope_opt_identifier %d\n", __LINE__); }
        | scope_opt_complex_name { printf("| scope_opt_complex_name %d\n", __LINE__); }
        | '(' paren_identifier_declarator ')' { printf("| '(' paren_identifier_declarator ')' %d\n", __LINE__); }
        ;


    /* Note that CLCL IDENTIFIER is NOT part of scope_opt_identifier, 
    but  it  is  part of global_opt_scope_opt_identifier.  It is ONLY 
    valid for referring to an identifier, and NOT valid for declaring 
    (or importing an external declaration of)  an  identifier.   This 
    disambiguates  the  following  code,  which  would  otherwise  be 
    syntactically and semantically ambiguous:

            class base {
                static int i; // element i;
                float member_function(void);
                };
            base i; // global i
            float base::member_function(void) {
                i; // refers to static int element "i" of base
                ::i; // refers to global "i", with type "base"
                    {
                    base :: i; // import of global "i", like "base (::i);"?
                                // OR reference to global??
                    }
                }
        */

primary_expression:
        global_opt_scope_opt_identifier { printf("global_opt_scope_opt_identifier %d\n", __LINE__); }
        | global_opt_scope_opt_complex_name { printf("| global_opt_scope_opt_complex_name %d\n", __LINE__); }
        | THIS   /* C++, not ANSI C */ { printf("| THIS   /* C++, not ANSI C */ %d\n", __LINE__); }
        | constant { printf("| constant %d\n", __LINE__); }
        | string_literal_list { printf("| string_literal_list %d\n", __LINE__); }
        | '(' comma_expression ')' { printf("| '(' comma_expression ')' %d\n", __LINE__); }
        ;


    /* I had to disallow struct, union, or enum  elaborations  during 
    operator_function_name.   The  ANSI  C++  Working  paper is vague 
    about whether this should be part of the syntax, or a constraint.  
    The ambiguities that resulted were more than LALR  could  handle, 
    so  the  easiest  fix was to be more specific.  This means that I 
    had to in-line expand type_specifier_or_name far  enough  that  I 
    would  be  able to exclude elaborations.  This need is what drove 
    me to distinguish a whole series of tokens based on whether  they 
    include elaborations:

         struct A { ... }

    or simply a reference to an aggregate or enumeration:

         enum A

    The  latter,  as  well  an  non-aggregate  types are what make up 
    non_elaborating_type_specifier */

    /* Note that the following does not include  Const_Volatile_list. 
    Hence,   whenever   non_elaborating_type_specifier  is  used,  an 
    adjacent rule is supplied containing Const_Volatile_list.  It  is 
    not  generally  possible  to  know  immediately  (i_e., reduce) a 
    Const_Volatile_list, as a TYPEDEFname that follows might  not  be 
    part of a type specifier, but might instead be "TYPEDEFname ::*".  
    */

non_elaborating_type_specifier:
        sue_type_specifier { printf("sue_type_specifier %d\n", __LINE__); }
        | basic_type_specifier { printf("| basic_type_specifier %d\n", __LINE__); }
        | typedef_type_specifier { printf("| typedef_type_specifier %d\n", __LINE__); }
        | basic_type_name { printf("| basic_type_name %d\n", __LINE__); }
        | TYPEDEFname { printf("| TYPEDEFname %d\n", __LINE__); }
        | global_or_scoped_typedefname { printf("| global_or_scoped_typedefname %d\n", __LINE__); }
        ;


    /*  The  following  introduces  MANY  conflicts.   Requiring  and 
    allowing '(' ')' around the `type' when the type is complex would 
    help a lot. */

operator_function_name:
        OPERATOR any_operator { printf("OPERATOR any_operator %d\n", __LINE__); }
        | OPERATOR Const_Volatile_list            operator_function_ptr_opt { printf("| OPERATOR Const_Volatile_list            operator_function_ptr_opt %d\n", __LINE__); }
        | OPERATOR non_elaborating_type_specifier operator_function_ptr_opt { printf("| OPERATOR non_elaborating_type_specifier operator_function_ptr_opt %d\n", __LINE__); }
        ;


    /* The following causes several ambiguities on *  and  &.   These 
    conflicts  would also be removed if parens around the `type' were 
    required in the derivations for operator_function_name */

    /*  Interesting  aside:  The  use  of  right  recursion  in   the 
    production  for  operator_function_ptr_opt gives both the correct 
    parsing, AND removes a conflict!   Right  recursion  permits  the 
    parser  to  defer  reductions  (a.k.a.:  delay  resolution),  and 
    effectively make a second pass! */

operator_function_ptr_opt:
        /* nothing */
        | unary_modifier        operator_function_ptr_opt { printf("| unary_modifier        operator_function_ptr_opt %d\n", __LINE__); }
| asterisk_or_ampersand operator_function_ptr_opt { printf("| asterisk_or_ampersand operator_function_ptr_opt %d\n", __LINE__); }
        ;


    /* List of operators we can overload */
any_operator:
'+' { printf("'+' %d\n", __LINE__); }
| '-' { printf("| '-' %d\n", __LINE__); }
| '*' { printf("| '*' %d\n", __LINE__); }
| '/' { printf("| '/' %d\n", __LINE__); }
| '%' { printf("| '%' %d\n", __LINE__); }
| '^' { printf("| '^' %d\n", __LINE__); }
| '&' { printf("| '&' %d\n", __LINE__); }
| '|' { printf("| '|' %d\n", __LINE__); }
| '~' { printf("| '~' %d\n", __LINE__); }
| '!' { printf("| '!' %d\n", __LINE__); }
| '<' { printf("| '<' %d\n", __LINE__); }
| '>' { printf("| '>' %d\n", __LINE__); }
| LS { printf("| LS %d\n", __LINE__); }
| RS { printf("| RS %d\n", __LINE__); }
| ANDAND { printf("| ANDAND %d\n", __LINE__); }
| OROR { printf("| OROR %d\n", __LINE__); }
| ARROW { printf("| ARROW %d\n", __LINE__); }
| ARROWstar { printf("| ARROWstar %d\n", __LINE__); }
| '.' { printf("| '.' %d\n", __LINE__); }
| DOTstar { printf("| DOTstar %d\n", __LINE__); }
| ICR { printf("| ICR %d\n", __LINE__); }
| DECR { printf("| DECR %d\n", __LINE__); }
| LE { printf("| LE %d\n", __LINE__); }
| GE { printf("| GE %d\n", __LINE__); }
| EQ { printf("| EQ %d\n", __LINE__); }
| NE { printf("| NE %d\n", __LINE__); }
| assignment_operator { printf("| assignment_operator %d\n", __LINE__); }
| '(' ')' { printf("| '(' ')' %d\n", __LINE__); }
| '[' ']' { printf("| '[' ']' %d\n", __LINE__); }
| NEW { printf("| NEW %d\n", __LINE__); }
| DELETE { printf("| DELETE %d\n", __LINE__); }
| ',' { printf("| ',' %d\n", __LINE__); }
        ;


    /* The following production for Const_Volatile_list was specially 
    placed BEFORE the definition of postfix_expression to  resolve  a 
    reduce-reduce    conflict    set    correctly.    Note   that   a 
    Const_Volatile_list is only used  in  a  declaration,  whereas  a 
    postfix_expression is clearly an example of an expression.  Hence 
    we  are helping with the "if it can be a declaration, then it is" 
    rule.  The reduce conflicts are on ')', ',' and '='.  Do not move 
    the following productions */

Const_Volatile_list_opt:
        /* Nothing */
| Const_Volatile_list { printf("| Const_Volatile_list %d\n", __LINE__); }
        ;


    /*  Note  that  the next set of productions in this grammar gives 
    post-increment a higher precedence that pre-increment.   This  is 
    not  clearly  stated  in  the  C++  Reference manual, and is only 
    implied by the grammar in the ANSI C Standard. */

    /* I *DON'T* use  argument_expression_list_opt  to  simplify  the 
    grammar  shown  below.   I am deliberately deferring any decision 
    until    *after*     the     closing     paren,     and     using 
    "argument_expression_list_opt" would commit prematurely.  This is 
    critical to proper conflict resolution. */

    /*  The  {}  in  the following rules allow the parser to tell the 
    lexer to search for the member name  in  the  appropriate  scope, 
    much the way the CLCL operator works.*/

postfix_expression:
primary_expression { printf("primary_expression %d\n", __LINE__); }
| postfix_expression '[' comma_expression ']' { printf("| postfix_expression '[' comma_expression ']' %d\n", __LINE__); }
| postfix_expression '(' ')' { printf("| postfix_expression '(' ')' %d\n", __LINE__); }
| postfix_expression '(' argument_expression_list ')' { printf("| postfix_expression '(' argument_expression_list ')' %d\n", __LINE__); }
| postfix_expression {} '.'   member_name { printf("| postfix_expression {} '.'   member_name %d\n", __LINE__); }
| postfix_expression {} ARROW member_name { printf("| postfix_expression {} ARROW member_name %d\n", __LINE__); }
| postfix_expression ICR { printf("| postfix_expression ICR %d\n", __LINE__); }
| postfix_expression DECR { printf("| postfix_expression DECR %d\n", __LINE__); }

                /* The next 4 rules are the source of cast ambiguity */
| TYPEDEFname                  '(' ')' { printf("| TYPEDEFname                  '(' ')' %d\n", __LINE__); }
| global_or_scoped_typedefname '(' ')' { printf("| global_or_scoped_typedefname '(' ')' %d\n", __LINE__); }
| TYPEDEFname                  '(' argument_expression_list ')' { printf("| TYPEDEFname                  '(' argument_expression_list ')' %d\n", __LINE__); }
| global_or_scoped_typedefname '(' argument_expression_list ')' { printf("| global_or_scoped_typedefname '(' argument_expression_list ')' %d\n", __LINE__); }
| basic_type_name '(' assignment_expression ')' { printf("| basic_type_name '(' assignment_expression ')' %d\n", __LINE__); }
            /* If the following rule is added to the  grammar,  there 
            will  be 3 additional reduce-reduce conflicts.  They will 
            all be resolved in favor of NOT using the following rule, 
            so no harm will be done.   However,  since  the  rule  is 
            semantically  illegal  we  will  omit  it  until  we  are 
            enhancing the grammar for error recovery */
/*      | basic_type_name '(' ')'  /* Illegal: no such constructor*/
        ;


    /* The last two productions in the next set are questionable, but 
    do not induce any conflicts.  I need to ask X3J16 :  Having  them 
    means that we have complex member function deletes like:

          const unsigned int :: ~ const unsigned int
    */

member_name:
scope_opt_identifier { printf("scope_opt_identifier %d\n", __LINE__); }
| scope_opt_complex_name { printf("| scope_opt_complex_name %d\n", __LINE__); }
| basic_type_name CLCL '~' basic_type_name  /* C++, not ANSI C */ { printf("| basic_type_name CLCL '~' basic_type_name  /* C++, not ANSI C */ %d\n", __LINE__); }
| declaration_qualifier_list  CLCL '~'   declaration_qualifier_list { printf("| declaration_qualifier_list  CLCL '~'   declaration_qualifier_list %d\n", __LINE__); }
| Const_Volatile_list         CLCL '~'   Const_Volatile_list { printf("| Const_Volatile_list         CLCL '~'   Const_Volatile_list %d\n", __LINE__); }
        ;

argument_expression_list:
assignment_expression { printf("assignment_expression %d\n", __LINE__); }
| argument_expression_list ',' assignment_expression { printf("| argument_expression_list ',' assignment_expression %d\n", __LINE__); }
        ;

unary_expression:
postfix_expression { printf("postfix_expression %d\n", __LINE__); }
| ICR  unary_expression { printf("| ICR  unary_expression %d\n", __LINE__); }
| DECR unary_expression { printf("| DECR unary_expression %d\n", __LINE__); }
| asterisk_or_ampersand cast_expression { printf("| asterisk_or_ampersand cast_expression %d\n", __LINE__); }
| '-'                   cast_expression { printf("| '-'                   cast_expression %d\n", __LINE__); }
| '+'                   cast_expression { printf("| '+'                   cast_expression %d\n", __LINE__); }
| '~'                   cast_expression { printf("| '~'                   cast_expression %d\n", __LINE__); }
| '!'                   cast_expression { printf("| '!'                   cast_expression %d\n", __LINE__); }
| SIZEOF unary_expression { printf("| SIZEOF unary_expression %d\n", __LINE__); }
| SIZEOF '(' type_name ')' { printf("| SIZEOF '(' type_name ')' %d\n", __LINE__); }
| allocation_expression { printf("| allocation_expression %d\n", __LINE__); }
        ;


    /* Note that I could have moved the  newstore  productions  to  a 
    lower  precedence  level  than  multiplication  (binary '*'), and 
    lower than bitwise AND (binary '&').  These moves  are  the  nice 
    way  to  disambiguate a trailing unary '*' or '&' at the end of a 
    freestore expression.  Since the freestore expression (with  such 
    a  grammar  and  hence  precedence  given)  can never be the left 
    operand of a binary '*' or '&', the ambiguity would  be  removed. 
    These  problems  really  surface when the binary operators '*' or 
    '&' are overloaded, but this must be syntactically  disambiguated 
    before the semantic checking is performed...  Unfortunately, I am 
    not  creating  the language, only writing a grammar that reflects 
    its specification, and  hence  I  cannot  change  its  precedence 
    assignments.   If  I  had  my  druthers,  I would probably prefer 
    surrounding the type with parens all the time, and  avoiding  the 
    dangling * and & problem all together.*/

       /* Following are C++, not ANSI C */
allocation_expression:
global_opt_scope_opt_operator_new '(' type_name ')' operator_new_initializer_opt { printf("global_opt_scope_opt_operator_new '(' type_name ')' operator_new_initializer_opt %d\n", __LINE__); }
| global_opt_scope_opt_operator_new '(' argument_expression_list ')' '(' type_name ')' operator_new_initializer_opt { printf("| global_opt_scope_opt_operator_new '(' argument_expression_list ')' '(' type_name ')' operator_new_initializer_opt %d\n", __LINE__); }

                /* next two rules are the source of * and & ambiguities */
| global_opt_scope_opt_operator_new operator_new_type { printf("| global_opt_scope_opt_operator_new operator_new_type %d\n", __LINE__); }
| global_opt_scope_opt_operator_new '(' argument_expression_list ')' operator_new_type { printf("| global_opt_scope_opt_operator_new '(' argument_expression_list ')' operator_new_type %d\n", __LINE__); }
        ;


       /* Following are C++, not ANSI C */
global_opt_scope_opt_operator_new:
NEW { printf("NEW %d\n", __LINE__); }
| global_or_scope NEW { printf("| global_or_scope NEW %d\n", __LINE__); }
        ;

operator_new_type:
Const_Volatile_list operator_new_declarator_opt operator_new_initializer_opt { printf("Const_Volatile_list operator_new_declarator_opt operator_new_initializer_opt %d\n", __LINE__); }
| non_elaborating_type_specifier operator_new_declarator_opt operator_new_initializer_opt { printf("| non_elaborating_type_specifier operator_new_declarator_opt operator_new_initializer_opt %d\n", __LINE__); }
        ;

    
    /*  Right  recursion  is critical in the following productions to 
    avoid a conflict on TYPEDEFname */

operator_new_declarator_opt:
        /* Nothing */
| operator_new_array_declarator { printf("| operator_new_array_declarator %d\n", __LINE__); }
| asterisk_or_ampersand operator_new_declarator_opt { printf("| asterisk_or_ampersand operator_new_declarator_opt %d\n", __LINE__); }
| unary_modifier        operator_new_declarator_opt { printf("| unary_modifier        operator_new_declarator_opt %d\n", __LINE__); }
        ;

operator_new_array_declarator:
'['                  ']' { printf("'['                  ']' %d\n", __LINE__); }
|                               '[' comma_expression ']' { printf("|                               '[' comma_expression ']' %d\n", __LINE__); }
| operator_new_array_declarator '[' comma_expression ']' { printf("| operator_new_array_declarator '[' comma_expression ']' %d\n", __LINE__); }
        ;

operator_new_initializer_opt:
/* Nothing */ { printf("/* Nothing */ %d\n", __LINE__); }
| '('                          ')' { printf("| '('                          ')' %d\n", __LINE__); }
| '(' argument_expression_list ')' { printf("| '(' argument_expression_list ')' %d\n", __LINE__); }
        ;

cast_expression:
unary_expression { printf("unary_expression %d\n", __LINE__); }
| '(' type_name ')' cast_expression { printf("| '(' type_name ')' cast_expression %d\n", __LINE__); }
        ;


    /* Following are C++, not ANSI C */
deallocation_expression:
cast_expression { printf("cast_expression %d\n", __LINE__); }
| global_opt_scope_opt_delete deallocation_expression { printf("| global_opt_scope_opt_delete deallocation_expression %d\n", __LINE__); }
| global_opt_scope_opt_delete '[' comma_expression ']' deallocation_expression  /* archaic C++, what a concept */ { printf("| global_opt_scope_opt_delete '[' comma_expression ']' deallocation_expression  /* archaic C++, what a concept */ %d\n", __LINE__); }
| global_opt_scope_opt_delete '[' ']' deallocation_expression { printf("| global_opt_scope_opt_delete '[' ']' deallocation_expression %d\n", __LINE__); }
        ;


    /* Following are C++, not ANSI C */
global_opt_scope_opt_delete:
DELETE { printf("DELETE %d\n", __LINE__); }
| global_or_scope DELETE { printf("| global_or_scope DELETE %d\n", __LINE__); }
        ;


    /* Following are C++, not ANSI C */
point_member_expression:
deallocation_expression { printf("deallocation_expression %d\n", __LINE__); }
| point_member_expression DOTstar  deallocation_expression { printf("| point_member_expression DOTstar  deallocation_expression %d\n", __LINE__); }
| point_member_expression ARROWstar  deallocation_expression { printf("| point_member_expression ARROWstar  deallocation_expression %d\n", __LINE__); }
        ;

multiplicative_expression:
point_member_expression { printf("point_member_expression %d\n", __LINE__); }
| multiplicative_expression '*' point_member_expression { printf("| multiplicative_expression '*' point_member_expression %d\n", __LINE__); }
| multiplicative_expression '/' point_member_expression { printf("| multiplicative_expression '/' point_member_expression %d\n", __LINE__); }
| multiplicative_expression '%' point_member_expression { printf("| multiplicative_expression '%' point_member_expression %d\n", __LINE__); }
        ;

additive_expression:
multiplicative_expression { printf("multiplicative_expression %d\n", __LINE__); }
| additive_expression '+' multiplicative_expression { printf("| additive_expression '+' multiplicative_expression %d\n", __LINE__); }
| additive_expression '-' multiplicative_expression { printf("| additive_expression '-' multiplicative_expression %d\n", __LINE__); }
        ;

shift_expression:
additive_expression { printf("additive_expression %d\n", __LINE__); }
| shift_expression LS additive_expression { printf("| shift_expression LS additive_expression %d\n", __LINE__); }
| shift_expression RS additive_expression { printf("| shift_expression RS additive_expression %d\n", __LINE__); }
        ;

relational_expression:
shift_expression { printf("shift_expression %d\n", __LINE__); }
| relational_expression '<' shift_expression { printf("| relational_expression '<' shift_expression %d\n", __LINE__); }
| relational_expression '>' shift_expression { printf("| relational_expression '>' shift_expression %d\n", __LINE__); }
| relational_expression LE  shift_expression { printf("| relational_expression LE  shift_expression %d\n", __LINE__); }
| relational_expression GE  shift_expression { printf("| relational_expression GE  shift_expression %d\n", __LINE__); }
        ;

equality_expression:
relational_expression { printf("relational_expression %d\n", __LINE__); }
| equality_expression EQ relational_expression { printf("| equality_expression EQ relational_expression %d\n", __LINE__); }
| equality_expression NE relational_expression { printf("| equality_expression NE relational_expression %d\n", __LINE__); }
        ;

AND_expression:
equality_expression { printf("equality_expression %d\n", __LINE__); }
| AND_expression '&' equality_expression { printf("| AND_expression '&' equality_expression %d\n", __LINE__); }
        ;

exclusive_OR_expression:
AND_expression { printf("AND_expression %d\n", __LINE__); }
| exclusive_OR_expression '^' AND_expression { printf("| exclusive_OR_expression '^' AND_expression %d\n", __LINE__); }
        ;

inclusive_OR_expression:
exclusive_OR_expression { printf("exclusive_OR_expression %d\n", __LINE__); }
| inclusive_OR_expression '|' exclusive_OR_expression { printf("| inclusive_OR_expression '|' exclusive_OR_expression %d\n", __LINE__); }
        ;

logical_AND_expression:
inclusive_OR_expression { printf("inclusive_OR_expression %d\n", __LINE__); }
| logical_AND_expression ANDAND inclusive_OR_expression { printf("| logical_AND_expression ANDAND inclusive_OR_expression %d\n", __LINE__); }
        ;

logical_OR_expression:
logical_AND_expression { printf("logical_AND_expression %d\n", __LINE__); }
| logical_OR_expression OROR logical_AND_expression { printf("| logical_OR_expression OROR logical_AND_expression %d\n", __LINE__); }
        ;

conditional_expression:
logical_OR_expression { printf("logical_OR_expression %d\n", __LINE__); }
| logical_OR_expression '?' comma_expression ':' conditional_expression { printf("| logical_OR_expression '?' comma_expression ':' conditional_expression %d\n", __LINE__); }
        ;

assignment_expression:
conditional_expression { printf("conditional_expression %d\n", __LINE__); }
| unary_expression assignment_operator assignment_expression { printf("| unary_expression assignment_operator assignment_expression %d\n", __LINE__); }
        ;

assignment_operator:
'=' { printf("'=' %d\n", __LINE__); }
| MULTassign { printf("| MULTassign %d\n", __LINE__); }
| DIVassign { printf("| DIVassign %d\n", __LINE__); }
| MODassign { printf("| MODassign %d\n", __LINE__); }
| PLUSassign { printf("| PLUSassign %d\n", __LINE__); }
| MINUSassign { printf("| MINUSassign %d\n", __LINE__); }
| LSassign { printf("| LSassign %d\n", __LINE__); }
| RSassign { printf("| RSassign %d\n", __LINE__); }
| ANDassign { printf("| ANDassign %d\n", __LINE__); }
| ERassign { printf("| ERassign %d\n", __LINE__); }
| ORassign { printf("| ORassign %d\n", __LINE__); }
        ;

comma_expression:
assignment_expression { printf("assignment_expression %d\n", __LINE__); }
| comma_expression ',' assignment_expression { printf("| comma_expression ',' assignment_expression %d\n", __LINE__); }
        ;

constant_expression:
conditional_expression { printf("conditional_expression %d\n", __LINE__); }
        ;


    /* The following was used for clarity */
comma_expression_opt:
/* Nothing */ { printf("/* Nothing */ %d\n", __LINE__); }
| comma_expression { printf("| comma_expression %d\n", __LINE__); }
        ;


/******************************* DECLARATIONS *********************************/


    /*  The  following are notably different from the ANSI C Standard 
    specified grammar, but  are  present  in  my  ANSI  C  compatible 
    grammar.  The changes were made to disambiguate typedefs presence 
    in   declaration_specifiers   (vs.    in   the   declarator   for 
    redefinition); to allow struct/union/enum/class tag  declarations 
    without  declarators,  and  to  better  reflect  the  parsing  of 
    declarations    (declarators    must     be     combined     with 
    declaration_specifiers  ASAP, so that they can immediately become 
    visible in the current scope). */

declaration:
declaring_list ';' { printf("declaring_list ';' %d\n", __LINE__); }
| default_declaring_list ';' { printf("| default_declaring_list ';' %d\n", __LINE__); }
| sue_declaration_specifier ';' { printf("| sue_declaration_specifier ';'  %d\n", __LINE__); }
| sue_type_specifier ';' { printf("| sue_type_specifier ';' %d\n", __LINE__); }
| sue_type_specifier_elaboration ';' { printf("| sue_type_specifier_elaboration ';' %d\n", __LINE__); }
        ;


    /* Note that if a typedef were  redeclared,  then  a  declaration 
    specifier  must be supplied (re: ANSI C spec).  The following are 
    declarations wherein no declaration_specifier  is  supplied,  and 
    hence the 'default' must be used.  An example of this is

        const a;

    which by default, is the same as:

        const int a;

    `a' must NOT be a typedef in the above example. */


    /*  The  presence of `{}' in the following rules indicates points 
    at which the symbol table MUST be updated so that  the  tokenizer 
    can  IMMEDIATELY  continue  to  maintain  the  proper distinction 
    between a TYPEDEFname and an IDENTIFIER. */

default_declaring_list:  /* Can't  redeclare typedef names */
declaration_qualifier_list   identifier_declarator {} initializer_opt { printf("declaration_qualifier_list   identifier_declarator {} initializer_opt %d\n", __LINE__); }
| Const_Volatile_list        identifier_declarator {} initializer_opt { printf("| Const_Volatile_list        identifier_declarator {} initializer_opt %d\n", __LINE__); }
| default_declaring_list ',' identifier_declarator {} initializer_opt { printf("| default_declaring_list ',' identifier_declarator {} initializer_opt %d\n", __LINE__); }

| declaration_qualifier_list constructed_identifier_declarator { printf("| declaration_qualifier_list constructed_identifier_declarator %d\n", __LINE__); }
| Const_Volatile_list        constructed_identifier_declarator { printf("| Const_Volatile_list        constructed_identifier_declarator %d\n", __LINE__); }
| default_declaring_list ',' constructed_identifier_declarator { printf("| default_declaring_list ',' constructed_identifier_declarator %d\n", __LINE__); }
        ;


    /* Note how Const_Volatile_list is  NOT  used  in  the  following 
    productions.    Qualifiers   are   NOT   sufficient  to  redefine 
    typedef-names (as prescribed by the ANSI C standard).*/

declaring_list:
declaration_specifier          declarator {} initializer_opt { printf("declaration_specifier          declarator {} initializer_opt %d\n", __LINE__); }
| type_specifier               declarator {} initializer_opt { printf("| type_specifier               declarator {} initializer_opt %d\n", __LINE__); }
| basic_type_name              declarator {} initializer_opt { printf("| basic_type_name              declarator {} initializer_opt %d\n", __LINE__); }
| TYPEDEFname                  declarator {} initializer_opt { printf("| TYPEDEFname                  declarator {} initializer_opt %d\n", __LINE__); }
| global_or_scoped_typedefname declarator {} initializer_opt { printf("| global_or_scoped_typedefname declarator {} initializer_opt %d\n", __LINE__); }
| declaring_list ','           declarator {} initializer_opt { printf("| declaring_list ','           declarator {} initializer_opt %d\n", __LINE__); }

| declaration_specifier        constructed_declarator { printf("| declaration_specifier        constructed_declarator %d\n", __LINE__); }
| type_specifier               constructed_declarator { printf("| type_specifier               constructed_declarator %d\n", __LINE__); }
| basic_type_name              constructed_declarator { printf("| basic_type_name              constructed_declarator %d\n", __LINE__); }
| TYPEDEFname                  constructed_declarator { printf("| TYPEDEFname                  constructed_declarator %d\n", __LINE__); }
| global_or_scoped_typedefname constructed_declarator { printf("| global_or_scoped_typedefname constructed_declarator %d\n", __LINE__); }
| declaring_list ','           constructed_declarator { printf("| declaring_list ','           constructed_declarator %d\n", __LINE__); }
        ;


    /* Declarators with  parenthesized  initializers  present  a  big 
    problem.  Typically  a  declarator  that looks like: "*a(...)" is 
    supposed to bind FIRST to the "(...)", and then to the "*".  This 
    binding  presumes  that  the  "(...)" stuff is a prototype.  With 
    constructed declarators, we must (officially) finish the  binding 
    to the "*" (finishing forming a good declarator) and THEN connect 
    with  the argument list. Unfortunately, by the time we realize it 
    is an argument list (and not a  prototype)  we  have  pushed  the 
    separate  declarator  tokens  "*"  and  "a"  onto  the yacc stack 
    WITHOUT combining them. The solution is to use odd productions to 
    carry  the  incomplete  declarator  along  with   the   "argument 
    expression  list" back up the yacc stack.  We would then actually 
    instantiate the symbol table after we have  fully  decorated  the 
    symbol  with all the leading "*" stuff.  Actually, since we don't 
    have all the type information in one spot till  we  reduce  to  a 
    declaring_list,  this delay is not a problem.  Note that ordinary 
    initializers REQUIRE (ANSI C Standard) that the symbol be  placed 
    into  the symbol table BEFORE its initializer is read, but in the 
    case of parenthesized initializers,  this  is  not  possible  (we 
    don't  even  know  we  have  an  initializer till have passed the 
    opening "(". ) */

constructed_declarator:
nonunary_constructed_identifier_declarator { printf("nonunary_constructed_identifier_declarator %d\n", __LINE__); }
| constructed_paren_typedef_declarator { printf("| constructed_paren_typedef_declarator %d\n", __LINE__); }
| simple_paren_typedef_declarator '(' argument_expression_list ')' { printf("| simple_paren_typedef_declarator '(' argument_expression_list ')' %d\n", __LINE__); }

| simple_paren_typedef_declarator postfixing_abstract_declarator '(' argument_expression_list ')'  /* constraint error */ { printf("| simple_paren_typedef_declarator postfixing_abstract_declarator '(' argument_expression_list ')'  /* constraint error */ %d\n", __LINE__); }

| constructed_parameter_typedef_declarator { printf("| constructed_parameter_typedef_declarator %d\n", __LINE__); }
| asterisk_or_ampersand constructed_declarator { printf("| asterisk_or_ampersand constructed_declarator %d\n", __LINE__); }
| unary_modifier        constructed_declarator { printf("| unary_modifier        constructed_declarator %d\n", __LINE__); }
        ;

constructed_paren_typedef_declarator:
'(' paren_typedef_declarator ')' '(' argument_expression_list ')' { printf("'(' paren_typedef_declarator ')' '(' argument_expression_list ')' %d\n", __LINE__); }
| '(' paren_typedef_declarator ')' postfixing_abstract_declarator '(' argument_expression_list ')' { printf("| '(' paren_typedef_declarator ')' postfixing_abstract_declarator '(' argument_expression_list ')' %d\n", __LINE__); }
| '(' simple_paren_typedef_declarator postfixing_abstract_declarator ')' '(' argument_expression_list ')' { printf("| '(' simple_paren_typedef_declarator postfixing_abstract_declarator ')' '(' argument_expression_list ')' %d\n", __LINE__); }
| '(' TYPEDEFname postfixing_abstract_declarator ')' '(' argument_expression_list ')' { printf("| '(' TYPEDEFname postfixing_abstract_declarator ')' '(' argument_expression_list ')' %d\n", __LINE__); }
        ;


constructed_parameter_typedef_declarator:
TYPEDEFname    '(' argument_expression_list ')' { printf("TYPEDEFname    '(' argument_expression_list ')' %d\n", __LINE__); }
| TYPEDEFname  postfixing_abstract_declarator '(' argument_expression_list ')' { printf("| TYPEDEFname  postfixing_abstract_declarator '(' argument_expression_list ')' %d\n", __LINE__); }
| '(' clean_typedef_declarator ')' '(' argument_expression_list ')' { printf("| '(' clean_typedef_declarator ')' '(' argument_expression_list ')' %d\n", __LINE__); }
| '(' clean_typedef_declarator ')'  postfixing_abstract_declarator '(' argument_expression_list ')' { printf("| '(' clean_typedef_declarator ')'  postfixing_abstract_declarator '(' argument_expression_list ')' %d\n", __LINE__); }
        ;


constructed_identifier_declarator:
nonunary_constructed_identifier_declarator { printf("nonunary_constructed_identifier_declarator %d\n", __LINE__); }
| asterisk_or_ampersand constructed_identifier_declarator { printf("| asterisk_or_ampersand constructed_identifier_declarator %d\n", __LINE__); }
| unary_modifier        constructed_identifier_declarator { printf("| unary_modifier        constructed_identifier_declarator %d\n", __LINE__); }
        ;


    /* The following are restricted to NOT  begin  with  any  pointer 
    operators.   This  includes both "*" and "T::*" modifiers.  Aside 
    from  this  restriction,   the   following   would   have   been: 
    identifier_declarator '(' argument_expression_list ')' */

nonunary_constructed_identifier_declarator:
paren_identifier_declarator   '(' argument_expression_list ')' { printf("paren_identifier_declarator   '(' argument_expression_list ')' %d\n", __LINE__); }
| paren_identifier_declarator postfixing_abstract_declarator '(' argument_expression_list ')' { printf("| paren_identifier_declarator postfixing_abstract_declarator '(' argument_expression_list ')'  %d\n", __LINE__); }
| '(' unary_identifier_declarator ')' '(' argument_expression_list ')' { printf("| '(' unary_identifier_declarator ')' '(' argument_expression_list ')' %d\n", __LINE__); }
| '(' unary_identifier_declarator ')' postfixing_abstract_declarator '(' argument_expression_list ')' { printf("| '(' unary_identifier_declarator ')' postfixing_abstract_declarator '(' argument_expression_list ')' %d\n", __LINE__); }
        ;


declaration_specifier:
basic_declaration_specifier          /* Arithmetic or void */ { printf("basic_declaration_specifier          /* Arithmetic or void */ %d\n", __LINE__); }
| sue_declaration_specifier          /* struct/union/enum/class */ { printf("| sue_declaration_specifier          /* struct/union/enum/class */ %d\n", __LINE__); }
| typedef_declaration_specifier      /* typedef*/ { printf("| typedef_declaration_specifier      /* typedef*/ %d\n", __LINE__); }
        ;

type_specifier:
basic_type_specifier                 /* Arithmetic or void */ { printf("basic_type_specifier                 /* Arithmetic or void */ %d\n", __LINE__); }
| sue_type_specifier                 /* Struct/Union/Enum/Class */ { printf("| sue_type_specifier                 /* Struct/Union/Enum/Class */ %d\n", __LINE__); }
| sue_type_specifier_elaboration     /* elaborated Struct/Union/Enum/Class */ { printf("| sue_type_specifier_elaboration     /* elaborated Struct/Union/Enum/Class */ %d\n", __LINE__); }
| typedef_type_specifier             /* Typedef */ { printf("| typedef_type_specifier             /* Typedef */ %d\n", __LINE__); }
        ;

declaration_qualifier_list:  /* storage class and optional const/volatile */
storage_class { printf("storage_class %d\n", __LINE__); }
| Const_Volatile_list storage_class { printf("| Const_Volatile_list storage_class %d\n", __LINE__); }
| declaration_qualifier_list declaration_qualifier { printf("| declaration_qualifier_list declaration_qualifier %d\n", __LINE__); }
        ;

Const_Volatile_list:
Const_Volatile { printf("Const_Volatile %d\n", __LINE__); }
| Const_Volatile_list Const_Volatile { printf("| Const_Volatile_list Const_Volatile %d\n", __LINE__); }
        ;

declaration_qualifier:
storage_class { printf("storage_class %d\n", __LINE__); }
| Const_Volatile { printf("| Const_Volatile %d\n", __LINE__); }
        ;

Const_Volatile:
CONST { printf("CONST %d\n", __LINE__); }
| VOLATILE { printf("| VOLATILE %d\n", __LINE__); }
        ;

basic_declaration_specifier:      /*Storage Class+Arithmetic or void*/
declaration_qualifier_list    basic_type_name { printf("declaration_qualifier_list    basic_type_name %d\n", __LINE__); }
| basic_type_specifier        storage_class { printf("| basic_type_specifier        storage_class %d\n", __LINE__); }
| basic_type_name             storage_class { printf("| basic_type_name             storage_class %d\n", __LINE__); }
| basic_declaration_specifier declaration_qualifier { printf("| basic_declaration_specifier declaration_qualifier %d\n", __LINE__); }
| basic_declaration_specifier basic_type_name { printf("| basic_declaration_specifier basic_type_name %d\n", __LINE__); }
        ;

basic_type_specifier:
Const_Volatile_list    basic_type_name /* Arithmetic or void */ { printf("Const_Volatile_list    basic_type_name /* Arithmetic or void */ %d\n", __LINE__); }
| basic_type_name      basic_type_name { printf("| basic_type_name      basic_type_name %d\n", __LINE__); }
| basic_type_name      Const_Volatile { printf("| basic_type_name      Const_Volatile %d\n", __LINE__); }
| basic_type_specifier Const_Volatile { printf("| basic_type_specifier Const_Volatile %d\n", __LINE__); }
| basic_type_specifier basic_type_name { printf("| basic_type_specifier basic_type_name %d\n", __LINE__); }
        ;

sue_declaration_specifier:          /* Storage Class + struct/union/enum/class */
declaration_qualifier_list       elaborated_type_name { printf("declaration_qualifier_list       elaborated_type_name %d\n", __LINE__); }
| declaration_qualifier_list     elaborated_type_name_elaboration { printf("| declaration_qualifier_list     elaborated_type_name_elaboration %d\n", __LINE__); }
| sue_type_specifier             storage_class { printf("| sue_type_specifier             storage_class %d\n", __LINE__); }
| sue_type_specifier_elaboration storage_class { printf("| sue_type_specifier_elaboration storage_class %d\n", __LINE__); }
| sue_declaration_specifier      declaration_qualifier { printf("| sue_declaration_specifier      declaration_qualifier %d\n", __LINE__); }
        ;

sue_type_specifier_elaboration:
elaborated_type_name_elaboration     /* elaborated struct/union/enum/class */ { printf("elaborated_type_name_elaboration     /* elaborated struct/union/enum/class */ %d\n", __LINE__); }
| Const_Volatile_list elaborated_type_name_elaboration { printf("| Const_Volatile_list elaborated_type_name_elaboration %d\n", __LINE__); }
| sue_type_specifier_elaboration Const_Volatile { printf("| sue_type_specifier_elaboration Const_Volatile %d\n", __LINE__); }
        ;

sue_type_specifier:
elaborated_type_name              /* struct/union/enum/class */ { printf("elaborated_type_name              /* struct/union/enum/class */ %d\n", __LINE__); }
| Const_Volatile_list elaborated_type_name { printf("| Const_Volatile_list elaborated_type_name %d\n", __LINE__); }
| sue_type_specifier Const_Volatile { printf("| sue_type_specifier Const_Volatile %d\n", __LINE__); }
        ;

typedef_declaration_specifier:       /*Storage Class + typedef types */
declaration_qualifier_list   TYPEDEFname { printf("declaration_qualifier_list   TYPEDEFname %d\n", __LINE__); }
        | declaration_qualifier_list   IDENTIFIER  {
             /* added by gamestar
              * for => typedef NotType(IDENTIFIER)  IDENTIFIER..etc; 
              */
            printf("declaration_qualifier_list   IDENTIFIER %d\n", __LINE__);
        }
| declaration_qualifier_list global_or_scoped_typedefname { printf("| declaration_qualifier_list global_or_scoped_typedefname %d\n", __LINE__); }

| typedef_type_specifier       storage_class { printf("| typedef_type_specifier       storage_class %d\n", __LINE__); }
| TYPEDEFname                  storage_class { printf("| TYPEDEFname                  storage_class %d\n", __LINE__); }
| global_or_scoped_typedefname storage_class { printf("| global_or_scoped_typedefname storage_class %d\n", __LINE__); }
| typedef_declaration_specifier declaration_qualifier { printf("| typedef_declaration_specifier declaration_qualifier %d\n", __LINE__); }
        ;

typedef_type_specifier:              /* typedef types */
Const_Volatile_list      TYPEDEFname { printf("Const_Volatile_list      TYPEDEFname %d\n", __LINE__); }
| Const_Volatile_list    global_or_scoped_typedefname { printf("| Const_Volatile_list    global_or_scoped_typedefname %d\n", __LINE__); }
| TYPEDEFname                  Const_Volatile { printf("| TYPEDEFname                  Const_Volatile %d\n", __LINE__); }
| global_or_scoped_typedefname Const_Volatile { printf("| global_or_scoped_typedefname Const_Volatile %d\n", __LINE__); }
| typedef_type_specifier Const_Volatile { printf("| typedef_type_specifier Const_Volatile %d\n", __LINE__); }
        ;


/*  There  are  really  several distinct sets of storage_classes. The 
sets vary depending on whether the declaration is at file scope, is a 
declaration within a struct/class, is within a function body, or in a 
function declaration/definition (prototype  parameter  declarations).  
They   are   grouped  here  to  simplify  the  grammar,  and  can  be 
semantically checked.  Note that this  approach  tends  to  ease  the 
syntactic restrictions in the grammar slightly, but allows for future 
language  development,  and tends to provide superior diagnostics and 
error recovery (i_e.: a syntax error does not disrupt the parse).


                File    File    Member  Member  Local   Local  Formal
                Var     Funct   Var     Funct   Var     Funct  Params
TYPEDEF         x       x       x       x       x       x
EXTERN          x       x                       x       x
STATIC          x       x       x       x       x
AUTO                                            x              x
REGISTER                                        x              x
FRIEND                                  x
OVERLOAD                x               x               x
INLINE                  x               x               x
VIRTUAL                                 x               x
*/

storage_class:
EXTERN { printf("EXTERN %d\n", __LINE__); }
| TYPEDEF { printf("| TYPEDEF %d\n", __LINE__); }
| STATIC { printf("| STATIC %d\n", __LINE__); }
| AUTO { printf("| AUTO %d\n", __LINE__); }
| REGISTER { printf("| REGISTER %d\n", __LINE__); }
| FRIEND   /* C++, not ANSI C */ { printf("| FRIEND   /* C++, not ANSI C */ %d\n", __LINE__); }
| OVERLOAD /* C++, not ANSI C */ { printf("| OVERLOAD /* C++, not ANSI C */ %d\n", __LINE__); }
| INLINE   /* C++, not ANSI C */ { printf("| INLINE   /* C++, not ANSI C */ %d\n", __LINE__); }
| VIRTUAL  /* C++, not ANSI C */ { printf("| VIRTUAL  /* C++, not ANSI C */ %d\n", __LINE__); }
        ;

basic_type_name:
INT { printf("INT %d\n", __LINE__); }
| CHAR { printf("| CHAR %d\n", __LINE__); }
| SHORT { printf("| SHORT %d\n", __LINE__); }
| LONG { printf("| LONG %d\n", __LINE__); }
| FLOAT { printf("| FLOAT %d\n", __LINE__); }
| DOUBLE { printf("| DOUBLE %d\n", __LINE__); }
| SIGNED { printf("| SIGNED %d\n", __LINE__); }
| UNSIGNED { printf("| UNSIGNED %d\n", __LINE__); }
| VOID { printf("| VOID %d\n", __LINE__); }
        ;

elaborated_type_name_elaboration:
aggregate_name_elaboration { printf("aggregate_name_elaboration %d\n", __LINE__); }
| enum_name_elaboration { printf("| enum_name_elaboration %d\n", __LINE__); }
        ;

elaborated_type_name:
aggregate_name { printf("aggregate_name %d\n", __LINE__); }
| enum_name { printf("| enum_name %d\n", __LINE__); }
        ;


    /* Since the expression "new type_name" MIGHT use  an  elaborated 
    type  and a derivation, it MIGHT have a ':'.  This fact conflicts 
    with the requirement that a new expression can be placed  between 
    a '?' and a ':' in a conditional expression (at least it confuses 
    LR(1)   parsers).   Hence   the   aggregate_name_elaboration   is 
    responsible for a series of SR conflicts on ':'.*/

    /* The intermediate actions {}  represent  points  at  which  the 
    database  of  typedef  names  must  be  updated  in C++.  This is 
    critical to the lexer, which must begin to tokenize based on this 
    new information. */

aggregate_name_elaboration:
aggregate_name derivation_opt  '{' member_declaration_list_opt '}' { printf("aggregate_name derivation_opt  '{' member_declaration_list_opt '}' %d\n", __LINE__); }
| aggregate_key derivation_opt '{' member_declaration_list_opt '}' { printf("| aggregate_key derivation_opt '{' member_declaration_list_opt '}' %d\n", __LINE__); }
        ;


    /* We distinguish between the above, which  support  elaboration, 
    and  this  set  of  productions  so  that  we can provide special 
    declaration specifiers for operator_new_type, and for  conversion 
    functions.  Note that without this restriction a large variety of 
    conflicts  appear  when  processing  operator_new and conversions 
    operators (which can be  followed  by  a  ':'  in  a  ternary  ?: 
    expression) */

    /*  Note that at the end of each of the following rules we should 
    be sure that the tag name is  in,  or  placed  in  the  indicated 
    scope.   If  no  scope  is  specified, then we must add it to our 
    current scope IFF it cannot  be  found  in  an  external  lexical 
    scope. */

aggregate_name:
aggregate_key tag_name { printf("aggregate_key tag_name %d\n", __LINE__); }
| global_scope scope aggregate_key tag_name { printf("| global_scope scope aggregate_key tag_name %d\n", __LINE__); }
| global_scope       aggregate_key tag_name { printf("| global_scope       aggregate_key tag_name %d\n", __LINE__); }
| scope              aggregate_key tag_name { printf("| scope              aggregate_key tag_name %d\n", __LINE__); }
        ;

derivation_opt:
/* nothing */ { printf("/* nothing */ %d\n", __LINE__); }
| ':' derivation_list { printf("| ':' derivation_list %d\n", __LINE__); }
        ;

derivation_list:
parent_class { printf("parent_class %d\n", __LINE__); }
| derivation_list ',' parent_class { printf("| derivation_list ',' parent_class %d\n", __LINE__); }
        ;

parent_class:
global_opt_scope_opt_typedefname { printf("global_opt_scope_opt_typedefname %d\n", __LINE__); }
| VIRTUAL access_specifier_opt global_opt_scope_opt_typedefname { printf("| VIRTUAL access_specifier_opt global_opt_scope_opt_typedefname %d\n", __LINE__); }
| access_specifier virtual_opt global_opt_scope_opt_typedefname { printf("| access_specifier virtual_opt global_opt_scope_opt_typedefname %d\n", __LINE__); }
        ;

virtual_opt:
/* nothing */ { printf("/* nothing */ %d\n", __LINE__); }
| VIRTUAL { printf("| VIRTUAL %d\n", __LINE__); }
        ;

access_specifier_opt:
/* nothing */ { printf("/* nothing */ %d\n", __LINE__); }
| access_specifier { printf("| access_specifier %d\n", __LINE__); }
        ;

access_specifier:
PUBLIC { printf("PUBLIC %d\n", __LINE__); }
| PRIVATE { printf("| PRIVATE %d\n", __LINE__); }
| PROTECTED { printf("| PROTECTED %d\n", __LINE__); }
        ;

aggregate_key:
STRUCT { printf("STRUCT %d\n", __LINE__); }
| UNION { printf("| UNION %d\n", __LINE__); }
| CLASS /* C++, not ANSI C */ { printf("| CLASS /* C++, not ANSI C */ %d\n", __LINE__); }
        ;


    /* Note that an empty list is ONLY allowed under C++. The grammar 
    can  be modified so that this stands out.  The trick is to define 
    member_declaration_list, and have that referenced for non-trivial 
    lists. */

member_declaration_list_opt:
/* nothing */ { printf("/* nothing */ %d\n", __LINE__); }
| member_declaration_list_opt member_declaration { printf("| member_declaration_list_opt member_declaration %d\n", __LINE__); }
        ;

member_declaration:
member_declaring_list ';' { printf("member_declaring_list ';' %d\n", __LINE__); }
| member_default_declaring_list ';' { printf("| member_default_declaring_list ';' %d\n", __LINE__); }
| access_specifier ':' { printf("| access_specifier ':' %d\n", __LINE__); }
| new_function_definition { printf("| new_function_definition %d\n", __LINE__); }
| constructor_function_in_class { printf("| constructor_function_in_class %d\n", __LINE__); }
| sue_type_specifier             ';' { printf("| sue_type_specifier             ';' %d\n", __LINE__); }
| sue_type_specifier_elaboration ';' { printf("| sue_type_specifier_elaboration ';' %d\n", __LINE__); }
| identifier_declarator          ';' { printf("| identifier_declarator          ';' %d\n", __LINE__); }
| typedef_declaration_specifier ';' { printf("| typedef_declaration_specifier ';' %d\n", __LINE__); }
| sue_declaration_specifier ';' { printf("| sue_declaration_specifier ';' %d\n", __LINE__); }
        ;

member_default_declaring_list:        /* doesn't redeclare typedef*/
Const_Volatile_list identifier_declarator member_pure_opt { printf("Const_Volatile_list identifier_declarator member_pure_opt %d\n", __LINE__); }
| declaration_qualifier_list identifier_declarator member_pure_opt { printf("| declaration_qualifier_list identifier_declarator member_pure_opt  %d\n", __LINE__); }
| member_default_declaring_list ',' identifier_declarator member_pure_opt { printf("| member_default_declaring_list ',' identifier_declarator member_pure_opt %d\n", __LINE__); }
| Const_Volatile_list                bit_field_identifier_declarator { printf("| Const_Volatile_list                bit_field_identifier_declarator %d\n", __LINE__); }
| declaration_qualifier_list         bit_field_identifier_declarator { printf("| declaration_qualifier_list         bit_field_identifier_declarator %d\n", __LINE__); }
| member_default_declaring_list ','  bit_field_identifier_declarator { printf("| member_default_declaring_list ','  bit_field_identifier_declarator %d\n", __LINE__); }
        ;


    /* There is a conflict when "struct A" is used as  a  declaration 
    specifier,  and  there  is a chance that a bit field name will be 
    provided.  To fix this syntactically would require distinguishing 
    non_elaborating_declaration_specifiers   the   way   I    handled 
    non_elaborating_type_specifiers.   I   think  this  should  be  a 
    constraint error anyway :-). */

member_declaring_list:        /* Can possibly redeclare typedefs */
type_specifier                 declarator member_pure_opt { printf("type_specifier                 declarator member_pure_opt %d\n", __LINE__); }
| basic_type_name              declarator member_pure_opt { printf("| basic_type_name              declarator member_pure_opt %d\n", __LINE__); }
| global_or_scoped_typedefname declarator member_pure_opt { printf("| global_or_scoped_typedefname declarator member_pure_opt %d\n", __LINE__); }
| member_conflict_declaring_item { printf("| member_conflict_declaring_item %d\n", __LINE__); }
| member_declaring_list ','    declarator member_pure_opt { printf("| member_declaring_list ','    declarator member_pure_opt %d\n", __LINE__); }
| type_specifier                bit_field_declarator { printf("| type_specifier                bit_field_declarator %d\n", __LINE__); }
| basic_type_name               bit_field_declarator { printf("| basic_type_name               bit_field_declarator %d\n", __LINE__); }
| TYPEDEFname                   bit_field_declarator { printf("| TYPEDEFname                   bit_field_declarator %d\n", __LINE__); }
| global_or_scoped_typedefname  bit_field_declarator { printf("| global_or_scoped_typedefname  bit_field_declarator %d\n", __LINE__); }
| declaration_specifier         bit_field_declarator /* constraint violation: storage class used */ { printf("| declaration_specifier         bit_field_declarator /* constraint violation: storage class used */ %d\n", __LINE__); }
| member_declaring_list ','     bit_field_declarator { printf("| member_declaring_list ','     bit_field_declarator %d\n", __LINE__); }
        ;


    /* The following conflict with constructors-
      member_conflict_declaring_item:
        TYPEDEFname             declarator member_pure_opt
        | declaration_specifier declarator member_pure_opt /* C++, not ANSI C * /
        ;
    so we inline expand declarator to get the following productions...
    */
member_conflict_declaring_item:
TYPEDEFname             identifier_declarator            member_pure_opt { printf("TYPEDEFname             identifier_declarator            member_pure_opt %d\n", __LINE__); }
| TYPEDEFname           parameter_typedef_declarator     member_pure_opt { printf("| TYPEDEFname           parameter_typedef_declarator     member_pure_opt %d\n", __LINE__); }
| TYPEDEFname           simple_paren_typedef_declarator  member_pure_opt { printf("| TYPEDEFname           simple_paren_typedef_declarator  member_pure_opt %d\n", __LINE__); }
| declaration_specifier identifier_declarator            member_pure_opt { printf("| declaration_specifier identifier_declarator            member_pure_opt %d\n", __LINE__); }
| declaration_specifier parameter_typedef_declarator     member_pure_opt { printf("| declaration_specifier parameter_typedef_declarator     member_pure_opt %d\n", __LINE__); }
| declaration_specifier simple_paren_typedef_declarator  member_pure_opt { printf("| declaration_specifier simple_paren_typedef_declarator  member_pure_opt %d\n", __LINE__); }
| member_conflict_paren_declaring_item { printf("| member_conflict_paren_declaring_item %d\n", __LINE__); }
        ;


    /* The following still conflicts with constructors-
      member_conflict_paren_declaring_item:
        TYPEDEFname             paren_typedef_declarator     member_pure_opt
        | declaration_specifier paren_typedef_declarator     member_pure_opt
        ;
    so paren_typedef_declarator is expanded inline to get...*/

member_conflict_paren_declaring_item:
    TYPEDEFname   asterisk_or_ampersand '(' simple_paren_typedef_declarator ')' member_pure_opt { printf("TYPEDEFname   asterisk_or_ampersand '(' simple_paren_typedef_declarator ')' member_pure_opt %d\n", __LINE__); }
    | TYPEDEFname unary_modifier '(' simple_paren_typedef_declarator ')' member_pure_opt { printf("| TYPEDEFname unary_modifier '(' simple_paren_typedef_declarator ')' member_pure_opt %d\n", __LINE__); }
    | TYPEDEFname asterisk_or_ampersand '(' TYPEDEFname ')' member_pure_opt { printf("| TYPEDEFname asterisk_or_ampersand '(' TYPEDEFname ')' member_pure_opt %d\n", __LINE__); }
    | TYPEDEFname unary_modifier '(' TYPEDEFname ')' member_pure_opt { printf("| TYPEDEFname unary_modifier '(' TYPEDEFname ')' member_pure_opt %d\n", __LINE__); }
    | TYPEDEFname asterisk_or_ampersand paren_typedef_declarator member_pure_opt { printf("| TYPEDEFname asterisk_or_ampersand paren_typedef_declarator member_pure_opt %d\n", __LINE__); }
    | TYPEDEFname unary_modifier paren_typedef_declarator member_pure_opt { printf("| TYPEDEFname unary_modifier paren_typedef_declarator member_pure_opt %d\n", __LINE__); }
    | declaration_specifier asterisk_or_ampersand '(' simple_paren_typedef_declarator ')' member_pure_opt { printf("| declaration_specifier asterisk_or_ampersand '(' simple_paren_typedef_declarator ')' member_pure_opt %d\n", __LINE__); }
    | declaration_specifier unary_modifier '(' simple_paren_typedef_declarator ')' member_pure_opt { printf("| declaration_specifier unary_modifier '(' simple_paren_typedef_declarator ')' member_pure_opt %d\n", __LINE__); }
    | declaration_specifier asterisk_or_ampersand '(' TYPEDEFname ')' member_pure_opt { printf("| declaration_specifier asterisk_or_ampersand '(' TYPEDEFname ')' member_pure_opt %d\n", __LINE__); }
    | declaration_specifier unary_modifier '(' TYPEDEFname ')' member_pure_opt { printf("| declaration_specifier unary_modifier '(' TYPEDEFname ')' member_pure_opt %d\n", __LINE__); }
    | declaration_specifier asterisk_or_ampersand paren_typedef_declarator member_pure_opt { printf("| declaration_specifier asterisk_or_ampersand paren_typedef_declarator member_pure_opt %d\n", __LINE__); }
    | declaration_specifier unary_modifier paren_typedef_declarator member_pure_opt { printf("| declaration_specifier unary_modifier paren_typedef_declarator member_pure_opt %d\n", __LINE__); }
    | member_conflict_paren_postfix_declaring_item { printf("| member_conflict_paren_postfix_declaring_item  %d\n", __LINE__); }
        ;


    /* but we still have the following conflicts with constructors-
   member_conflict_paren_postfix_declaring_item:
      TYPEDEFname             postfix_paren_typedef_declarator member_pure_opt
      | declaration_specifier postfix_paren_typedef_declarator member_pure_opt
      ;
    so we expand paren_postfix_typedef inline and get...*/

member_conflict_paren_postfix_declaring_item:
    TYPEDEFname     '(' paren_typedef_declarator ')' member_pure_opt { printf("TYPEDEFname     '(' paren_typedef_declarator ')' member_pure_opt %d\n", __LINE__); }
| TYPEDEFname   '(' simple_paren_typedef_declarator postfixing_abstract_declarator ')' member_pure_opt { printf("| TYPEDEFname   '(' simple_paren_typedef_declarator postfixing_abstract_declarator ')' member_pure_opt %d\n", __LINE__); }
| TYPEDEFname   '(' TYPEDEFname postfixing_abstract_declarator ')' member_pure_opt { printf("| TYPEDEFname   '(' TYPEDEFname postfixing_abstract_declarator ')' member_pure_opt %d\n", __LINE__); }
| TYPEDEFname   '(' paren_typedef_declarator ')' postfixing_abstract_declarator     member_pure_opt { printf("| TYPEDEFname   '(' paren_typedef_declarator ')' postfixing_abstract_declarator     member_pure_opt %d\n", __LINE__); }
| declaration_specifier '(' paren_typedef_declarator ')' member_pure_opt { printf("| declaration_specifier '(' paren_typedef_declarator ')' member_pure_opt %d\n", __LINE__); }
| declaration_specifier '(' simple_paren_typedef_declarator postfixing_abstract_declarator ')' member_pure_opt { printf("| declaration_specifier '(' simple_paren_typedef_declarator postfixing_abstract_declarator ')' member_pure_opt %d\n", __LINE__); }
| declaration_specifier '(' TYPEDEFname postfixing_abstract_declarator ')' member_pure_opt { printf("| declaration_specifier '(' TYPEDEFname postfixing_abstract_declarator ')' member_pure_opt %d\n", __LINE__); }
| declaration_specifier '(' paren_typedef_declarator ')' postfixing_abstract_declarator     member_pure_opt { printf("| declaration_specifier '(' paren_typedef_declarator ')' postfixing_abstract_declarator     member_pure_opt %d\n", __LINE__); }
        ;
    /* ...and we are done.  Now all  the  conflicts  appear  on  ';', 
    which can be semantically evaluated/disambiguated */


member_pure_opt:
/* nothing */ { printf("/* nothing */ %d\n", __LINE__); }
| '=' OCTALconstant { printf("| '=' OCTALconstant %d\n", __LINE__); }
        ;


    /*  Note  that  bit  field  names, where redefining TYPEDEFnames, 
    cannot be parenthesized in C++ (due to  ambiguities),  and  hence 
    this  part of the grammar is simpler than ANSI C. :-) The problem 
    occurs because:

         TYPEDEFname ( TYPEDEFname) : .....

    doesn't look like a bit field, rather it looks like a constructor 
    definition! */

bit_field_declarator:
bit_field_identifier_declarator { printf("bit_field_identifier_declarator %d\n", __LINE__); }
| TYPEDEFname {} ':' constant_expression { printf("| TYPEDEFname {} ':' constant_expression %d\n", __LINE__); }
        ;


    /* The actions taken in the "{}" above and below are intended  to 
    allow  the  symbol  table  to  be  updated when the declarator is 
    complete.  It is critical for code like:

            foo : sizeof(foo + 1);
    */

bit_field_identifier_declarator:
                                 ':' constant_expression { printf("':' constant_expression %d\n", __LINE__); }
      | identifier_declarator {} ':' constant_expression { printf("| identifier_declarator {} ':' constant_expression %d\n", __LINE__); }
        ;

enum_name_elaboration:
      global_opt_scope_opt_enum_key '{' enumerator_list '}' { printf("global_opt_scope_opt_enum_key '{' enumerator_list '}' %d\n", __LINE__); }
| enum_name                   '{' enumerator_list '}' { printf("| enum_name                   '{' enumerator_list '}' %d\n", __LINE__); }
        ;


    /* As with structures, the distinction between "elaborating"  and 
    "non-elaborating"  enum  types  is  maintained.  In actuality, it 
    probably does not cause much in the way of conflicts, since a ':' 
    is not allowed.  For symmetry, we maintain the distinction.   The 
    {}  actions are intended to allow the symbol table to be updated.  
    These updates are significant to code such as:

        enum A { first=sizeof(A)};
    */

enum_name:
global_opt_scope_opt_enum_key tag_name { printf("global_opt_scope_opt_enum_key tag_name %d\n", __LINE__); }
        ;

global_opt_scope_opt_enum_key:
ENUM { printf("ENUM %d\n", __LINE__); }
| global_or_scope ENUM { printf("| global_or_scope ENUM %d\n", __LINE__); }
        ;

enumerator_list:
enumerator_list_no_trailing_comma { printf("enumerator_list_no_trailing_comma %d\n", __LINE__); }
| enumerator_list_no_trailing_comma ',' /* C++, not ANSI C */ { printf("| enumerator_list_no_trailing_comma ',' /* C++, not ANSI C */ %d\n", __LINE__); }
        ;


    /* Note that we do not need to rush to add an enumerator  to  the 
    symbol  table  until  *AFTER* the enumerator_value_opt is parsed. 
    The enumerated value is only in scope  AFTER  its  definition  is 
    complete.   Hence the following is legal: "enum {a, b=a+10};" but 
    the following is (assuming no external matching of names) is  not 
    legal:  "enum {c, d=sizeof(d)};" ("d" not defined when sizeof was 
    applied.) This is  notably  contrasted  with  declarators,  which 
    enter scope as soon as the declarator is complete. */

enumerator_list_no_trailing_comma:
enumerator_name enumerator_value_opt { printf("enumerator_name enumerator_value_opt %d\n", __LINE__); }
| enumerator_list_no_trailing_comma ',' enumerator_name enumerator_value_opt { printf("| enumerator_list_no_trailing_comma ',' enumerator_name enumerator_value_opt %d\n", __LINE__); }
        ;

enumerator_name:
IDENTIFIER { printf("IDENTIFIER %d\n", __LINE__); }
| TYPEDEFname { printf("| TYPEDEFname %d\n", __LINE__); }
        ;

enumerator_value_opt:
/* Nothing */ { printf("/* Nothing */ %d\n", __LINE__); }
| '=' constant_expression { printf("| '=' constant_expression %d\n", __LINE__); }
        ;


    /*  We special case the lone type_name which has no storage class 
    (even though it should be an example of  a  parameter_type_list). 
    This helped to disambiguate type-names in parenthetical casts.*/

parameter_type_list:
'(' ')'                             Const_Volatile_list_opt { printf("'(' ')'                             Const_Volatile_list_opt %d\n", __LINE__); }
| '(' type_name ')'                 Const_Volatile_list_opt { printf("| '(' type_name ')'                 Const_Volatile_list_opt %d\n", __LINE__); }
| '(' type_name initializer ')'     Const_Volatile_list_opt { printf("| '(' type_name initializer ')'     Const_Volatile_list_opt %d\n", __LINE__); }
| '(' named_parameter_type_list ')' Const_Volatile_list_opt { printf("| '(' named_parameter_type_list ')' Const_Volatile_list_opt %d\n", __LINE__); }
        ;


    /* The following are used in old style function definitions, when 
    a complex return type includes the "function returning" modifier. 
    Note  the  subtle  distinction  from  parameter_type_list.  These 
    parameters are NOT the parameters for the function being defined, 
    but are simply part of the type definition.  An example would be:

        int(*f(   a  ))(float) long a; {...}

    which is equivalent to the full new style definition:

        int(*f(long a))(float) {...}

    The   type   list    `(float)'    is    an    example    of    an 
    old_parameter_type_list.   The  bizarre point here is that an old 
    function definition declarator can be followed by  a  type  list, 
    which  can  start  with a qualifier `const'.  This conflicts with 
    the new syntactic construct for const member  functions!?!  As  a 
    result,  an  old  style function definition cannot be used in all 
    cases for a member function.  */

old_parameter_type_list:
'(' ')' { printf("'(' ')' %d\n", __LINE__); }
| '(' type_name ')' { printf("| '(' type_name ')' %d\n", __LINE__); }
| '(' type_name initializer ')' { printf("| '(' type_name initializer ')' %d\n", __LINE__); }
| '(' named_parameter_type_list ')' { printf("| '(' named_parameter_type_list ')' %d\n", __LINE__); }
        ;

named_parameter_type_list:  /* WARNING: excludes lone type_name*/
parameter_list { printf("parameter_list %d\n", __LINE__); }
| parameter_list comma_opt_ellipsis { printf("| parameter_list comma_opt_ellipsis %d\n", __LINE__); }
| type_name comma_opt_ellipsis { printf("| type_name comma_opt_ellipsis %d\n", __LINE__); }
| type_name initializer comma_opt_ellipsis  /* C++, not ANSI C */ { printf("| type_name initializer comma_opt_ellipsis  /* C++, not ANSI C */ %d\n", __LINE__); }
| ELLIPSIS /* C++, not ANSI C */ { printf("| ELLIPSIS /* C++, not ANSI C */ %d\n", __LINE__); }
        ;

comma_opt_ellipsis:
ELLIPSIS       /* C
                  ++, not ANSI C */ { printf("ELLIPSIS       /* C++, not ANSI C */ %d\n", __LINE__); }
| ',' ELLIPSIS { printf("| ',' ELLIPSIS %d\n", __LINE__); }
        ;

parameter_list:
non_casting_parameter_declaration { printf("non_casting_parameter_declaration %d\n", __LINE__); }
| non_casting_parameter_declaration initializer /* C++, not ANSI C */ { printf("| non_casting_parameter_declaration initializer /* C++, not ANSI C */ %d\n", __LINE__); }
| type_name             ',' parameter_declaration { printf("| type_name             ',' parameter_declaration %d\n", __LINE__); }
| type_name initializer ',' parameter_declaration  /* C++, not ANSI C */ { printf("| type_name initializer ',' parameter_declaration  /* C++, not ANSI C */ %d\n", __LINE__); }
| parameter_list        ',' parameter_declaration { printf("| parameter_list        ',' parameter_declaration %d\n", __LINE__); }
        ;


    /* There is some very subtle disambiguation going  on  here.   Do 
    not be tempted to make further use of the following production in 
    parameter_list,  or else the conflict count will grow noticeably. 
    Specifically, the next set  of  rules  has  already  been  inline 
    expanded for the first parameter in a parameter_list to support a 
    deferred disambiguation. The subtle disambiguation has to do with 
    contexts where parameter type lists look like old-style-casts. */

parameter_declaration:
type_name { printf("type_name %d\n", __LINE__); }
| type_name initializer  /* C++, not ANSI C */ { printf("| type_name initializer  /* C++, not ANSI C */ %d\n", __LINE__); }
| non_casting_parameter_declaration { printf("| non_casting_parameter_declaration %d\n", __LINE__); }
| non_casting_parameter_declaration initializer /* C++, not ANSI C */ { printf("| non_casting_parameter_declaration initializer /* C++, not ANSI C */ %d\n", __LINE__); }
        ;


    /* There is an LR ambiguity between old-style parenthesized casts 
    and parameter-type-lists.  This tends to happen in contexts where 
    either  an  expression or a parameter-type-list is possible.  For 
    example, assume that T is an  externally  declared  type  in  the 
    code:

           int (T ((int

    it might continue:

           int (T ((int)0));

    which would make it:

           (int) (T) (int)0 ;

    which  is  an  expression,  consisting  of  a  series  of  casts.  
    Alternatively, it could be:

           int (T ((int a)));

    which would make it the redeclaration of T, equivalent to:

           int T (dummy_name (int a));

    if we see a type that either has a named variable (in  the  above 
    case "a"), or a storage class like:

           int (T ((int register

    then  we  know  it  can't  be  a cast, and it is "forced" to be a 
    parameter_list.

    It is not yet clear that the ANSI C++ committee would  decide  to 
    place this disambiguation into the syntax, rather than leaving it 
    as  a  constraint check (i.e., a valid parser would have to parse 
    everything as though it were  a  parameter  list  (in  these  odd 
    contexts),  and  then  give an error if is to a following context 
    (like "0" above) that invalidated this syntax evaluation. */

    /* One big thing implemented here is that a TYPEDEFname CANNOT be 
    redeclared when we don't have declaration_specifiers! Notice that 
    when we do use a TYPEDEFname based declarator, only the "special" 
    (non-ambiguous  in  this  context)  typedef_declarator  is  used. 
    Everything else that is "missing" shows up as a type_name. */

non_casting_parameter_declaration: /*have names or storage classes */
declaration_specifier { printf("declaration_specifier %d\n", __LINE__); }
| declaration_specifier abstract_declarator { printf("| declaration_specifier abstract_declarator %d\n", __LINE__); }
| declaration_specifier identifier_declarator { printf("| declaration_specifier identifier_declarator %d\n", __LINE__); }
| declaration_specifier parameter_typedef_declarator { printf("| declaration_specifier parameter_typedef_declarator %d\n", __LINE__); }
| declaration_qualifier_list { printf("| declaration_qualifier_list %d\n", __LINE__); }
| declaration_qualifier_list abstract_declarator { printf("| declaration_qualifier_list abstract_declarator %d\n", __LINE__); }
| declaration_qualifier_list identifier_declarator { printf("| declaration_qualifier_list identifier_declarator %d\n", __LINE__); }
| type_specifier identifier_declarator { printf("| type_specifier identifier_declarator %d\n", __LINE__); }
| type_specifier parameter_typedef_declarator { printf("| type_specifier parameter_typedef_declarator %d\n", __LINE__); }
| basic_type_name identifier_declarator { printf("| basic_type_name identifier_declarator %d\n", __LINE__); }
| basic_type_name parameter_typedef_declarator { printf("| basic_type_name parameter_typedef_declarator %d\n", __LINE__); }
| TYPEDEFname                   identifier_declarator { printf("| TYPEDEFname                   identifier_declarator %d\n", __LINE__); }
| TYPEDEFname                   parameter_typedef_declarator { printf("| TYPEDEFname                   parameter_typedef_declarator %d\n", __LINE__); }
| global_or_scoped_typedefname  identifier_declarator { printf("| global_or_scoped_typedefname  identifier_declarator %d\n", __LINE__); }
| global_or_scoped_typedefname  parameter_typedef_declarator { printf("| global_or_scoped_typedefname  parameter_typedef_declarator %d\n", __LINE__); }
| Const_Volatile_list identifier_declarator { printf("| Const_Volatile_list identifier_declarator %d\n", __LINE__); }
        ;

type_name:
type_specifier { printf("type_specifier %d\n", __LINE__); }
| basic_type_name { printf("| basic_type_name %d\n", __LINE__); }
| TYPEDEFname { printf("| TYPEDEFname %d\n", __LINE__); }
| global_or_scoped_typedefname { printf("| global_or_scoped_typedefname %d\n", __LINE__); }
| Const_Volatile_list { printf("| Const_Volatile_list %d\n", __LINE__); }
| IDENTIFIER { printf("| IDENTIFIER %d\n", __LINE__); /* by gamestar */}

| type_specifier               abstract_declarator { printf("| type_specifier               abstract_declarator %d\n", __LINE__); }
| basic_type_name              abstract_declarator { printf("| basic_type_name              abstract_declarator %d\n", __LINE__); }
| TYPEDEFname                  abstract_declarator { printf("| TYPEDEFname                  abstract_declarator %d\n", __LINE__); }
| global_or_scoped_typedefname abstract_declarator { printf("| global_or_scoped_typedefname abstract_declarator %d\n", __LINE__); }
| Const_Volatile_list          abstract_declarator { printf("| Const_Volatile_list          abstract_declarator %d\n", __LINE__); }
| IDENTIFIER abstract_declarator { printf("| IDENTIFIER abstract_declarator %d\n", __LINE__); /* by gamestar */}
        ;

initializer_opt:
/* nothing */ { printf("/* nothing */ %d\n", __LINE__); }
| initializer { printf("| initializer %d\n", __LINE__); }
        ;

initializer:
'=' initializer_group { printf("'=' initializer_group %d\n", __LINE__); }
        ;

initializer_group:
'{' initializer_list '}' { printf("'{' initializer_list '}' %d\n", __LINE__); }
| '{' initializer_list ',' '}' { printf("| '{' initializer_list ',' '}' %d\n", __LINE__); }
| assignment_expression { printf("| assignment_expression %d\n", __LINE__); }
        ;

initializer_list:
initializer_group { printf("initializer_group %d\n", __LINE__); }
| initializer_list ',' initializer_group { printf("| initializer_list ',' initializer_group %d\n", __LINE__); }
        ;


/*************************** STATEMENTS *******************************/

statement:
labeled_statement { printf("labeled_statement %d\n", __LINE__); }
| compound_statement { printf("| compound_statement %d\n", __LINE__); }
| expression_statement { printf("| expression_statement %d\n", __LINE__); }
| selection_statement { printf("| selection_statement %d\n", __LINE__); }
| iteration_statement { printf("| iteration_statement %d\n", __LINE__); }
| jump_statement { printf("| jump_statement %d\n", __LINE__); }
| declaration /* C++, not ANSI C */ { printf("| declaration /* C++, not ANSI C */ %d\n", __LINE__); }
        ;

labeled_statement:
label                      ':' statement { printf("label                      ':' statement %d\n", __LINE__); }
| CASE constant_expression ':' statement { printf("| CASE constant_expression ':' statement %d\n", __LINE__); }
| DEFAULT                  ':' statement { printf("| DEFAULT                  ':' statement %d\n", __LINE__); }
        ;


    /*  I sneak declarations into statement_list to support C++.  The 
    grammar is a little clumsy this  way,  but  the  violation  of  C 
    syntax is heavily localized */

compound_statement:
'{' statement_list_opt '}' { printf("'{' statement_list_opt '}' %d\n", __LINE__); }
        ;

declaration_list:
declaration { printf("declaration %d\n", __LINE__); }
| declaration_list declaration { printf("| declaration_list declaration %d\n", __LINE__); }
        ;

statement_list_opt:
/* nothing */ { printf("/* nothing */ %d\n", __LINE__); }
| statement_list_opt statement { printf("| statement_list_opt statement %d\n", __LINE__); }
        ;

expression_statement:
comma_expression_opt ';' { printf("comma_expression_opt ';' %d\n", __LINE__); }
        ;

selection_statement:
IF '(' comma_expression ')' statement { printf("IF '(' comma_expression ')' statement %d\n", __LINE__); }
| IF '(' comma_expression ')' statement ELSE statement { printf("| IF '(' comma_expression ')' statement ELSE statement %d\n", __LINE__); }
| SWITCH '(' comma_expression ')' statement { printf("| SWITCH '(' comma_expression ')' statement %d\n", __LINE__); }
        ;

iteration_statement:
        WHILE '(' comma_expression_opt ')' statement
        | DO statement WHILE '(' comma_expression ')' ';'
        | FOR '(' comma_expression_opt ';' comma_expression_opt ';'
                comma_expression_opt ')' statement
        | FOR '(' declaration        comma_expression_opt ';'
                comma_expression_opt ')' statement  /* C++, not ANSI C */
        ;

jump_statement:
        GOTO label                     ';'
        | CONTINUE                     ';'
        | BREAK                        ';'
        | RETURN comma_expression_opt  ';'
        ;


    /*  The  following  actions should update the symbol table in the 
    "label" name space */

label:
        IDENTIFIER
        | TYPEDEFname
        ;


/***************************** EXTERNAL DEFINITIONS *****************************/

translation_unit:
        /* nothing */ { printf("/* nothing */ %d\n", __LINE__); }
| translation_unit external_definition { printf("| translation_unit external_definition %d\n", __LINE__); }
        ;

external_definition:
function_declaration                         /* C++, not ANSI C*/ { printf("function_declaration                         /* C++, not ANSI C*/ %d\n", __LINE__); }
| function_definition { printf("| function_definition %d\n", __LINE__); }
| declaration { printf("| declaration %d\n", __LINE__); }
| linkage_specifier function_declaration     /* C++, not ANSI C*/ { printf("| linkage_specifier function_declaration     /* C++, not ANSI C*/ %d\n", __LINE__); }
| linkage_specifier function_definition      /* C++, not ANSI C*/ { printf("| linkage_specifier function_definition      /* C++, not ANSI C*/ %d\n", __LINE__); }
| linkage_specifier declaration              /* C++, not ANSI C*/ { printf("| linkage_specifier declaration              /* C++, not ANSI C*/ %d\n", __LINE__); }
| linkage_specifier '{' translation_unit '}' /* C++, not ANSI C*/ { printf("| linkage_specifier '{' translation_unit '}' /* C++, not ANSI C*/ %d\n", __LINE__); }
        ;

linkage_specifier:
EXTERN STRINGliteral { printf("EXTERN STRINGliteral %d\n", __LINE__); }
        ;


    /* Note that declaration_specifiers are left out of the following 
    function declarations.  Such omission is illegal in ANSI C. It is 
    sometimes necessary in C++, in instances  where  no  return  type 
    should be specified (e_g., a conversion operator).*/

function_declaration:
identifier_declarator ';' { printf("identifier_declarator ';' %d\n", __LINE__); }
                                    /*  semantically  verify  it is a 
                                    function, and (if ANSI says  it's 
                                    the  law for C++ also...) that it 
                                    is something that  can't  have  a 
                                    return  type  (like  a conversion 
                                    function, or a destructor */

| constructor_function_declaration ';' { printf("| constructor_function_declaration ';' %d\n", __LINE__); }
        ;

function_definition:
new_function_definition { printf("new_function_definition %d\n", __LINE__); }
| old_function_definition { printf("| old_function_definition %d\n", __LINE__); }
| constructor_function_definition { printf("| constructor_function_definition %d\n", __LINE__); }
        ;


    /* Note that in ANSI C, function definitions *ONLY* are presented 
    at file scope.  Hence, if there is a typedefname  active,  it  is 
    illegal  to  redeclare  it  (there  is no enclosing scope at file 
    scope).

    In  contrast,  C++  allows   function   definitions   at   struct 
    elaboration scope, and allows tags that are defined at file scope 
    (and  hence  look like typedefnames) to be redeclared to function 
    calls.  Hence several of the rules are "partially C++  only".   I 
    could  actually  build separate rules for typedef_declarators and 
    identifier_declarators, and mention that  the  typedef_declarator 
    rules represent the C++ only features.

    In  some  sense,  this  is  haggling, as I could/should have left 
    these as constraints in the ANSI C grammar, rather than as syntax 
    requirements.  */

new_function_definition:
identifier_declarator compound_statement { printf("identifier_declarator compound_statement %d\n", __LINE__); }
| declaration_specifier                   declarator compound_statement { printf("| declaration_specifier                   declarator compound_statement  %d\n", __LINE__); }
| type_specifier                          declarator compound_statement { printf("| type_specifier                          declarator compound_statement  %d\n", __LINE__); }
| basic_type_name                         declarator compound_statement { printf("| basic_type_name                         declarator compound_statement  %d\n", __LINE__); }
| TYPEDEFname                             declarator compound_statement { printf("| TYPEDEFname                             declarator compound_statement  %d\n", __LINE__); }
| global_or_scoped_typedefname            declarator compound_statement { printf("| global_or_scoped_typedefname            declarator compound_statement  %d\n", __LINE__); }
| declaration_qualifier_list   identifier_declarator compound_statement { printf("| declaration_qualifier_list   identifier_declarator compound_statement %d\n", __LINE__); }
| Const_Volatile_list          identifier_declarator compound_statement { printf("| Const_Volatile_list          identifier_declarator compound_statement %d\n", __LINE__); }
        ;


    /* Note that I do not support redeclaration of TYPEDEFnames  into 
    function  names  as I did in new_function_definitions (see note). 
    Perhaps I should do it, but for now, ignore the issue. Note  that 
    this  is  a  non-problem  with  ANSI  C,  as  tag  names  are not 
    considered TYPEDEFnames. */

old_function_definition:
old_function_declarator {} old_function_body { printf("old_function_declarator {} old_function_body %d\n", __LINE__); }
| declaration_specifier        old_function_declarator {} old_function_body { printf("| declaration_specifier        old_function_declarator {} old_function_body %d\n", __LINE__); }
| type_specifier               old_function_declarator {} old_function_body { printf("| type_specifier               old_function_declarator {} old_function_body %d\n", __LINE__); }
| basic_type_name              old_function_declarator {} old_function_body { printf("| basic_type_name              old_function_declarator {} old_function_body %d\n", __LINE__); }
| TYPEDEFname                  old_function_declarator {} old_function_body { printf("| TYPEDEFname                  old_function_declarator {} old_function_body %d\n", __LINE__); }
| global_or_scoped_typedefname old_function_declarator {} old_function_body { printf("| global_or_scoped_typedefname old_function_declarator {} old_function_body %d\n", __LINE__); }
| declaration_qualifier_list   old_function_declarator {} old_function_body { printf("| declaration_qualifier_list   old_function_declarator {} old_function_body %d\n", __LINE__); }
| Const_Volatile_list          old_function_declarator {} old_function_body { printf("| Const_Volatile_list          old_function_declarator {} old_function_body %d\n", __LINE__); }
        ;

old_function_body:
declaration_list compound_statement { printf("declaration_list compound_statement %d\n", __LINE__); }
| compound_statement { printf("| compound_statement %d\n", __LINE__); }
        ;


    /*    Verify    via    constraints     that     the     following 
        declaration_specifier           is          really          a 
        typedef_declaration_specifier, consisting of:

        ... TYPEDEFname :: TYPEDEFname

    optionally *preceded* by a "inline" keyword.   Use  care  not  to 
    support "inline" as a postfix!

    Similarly, the global_or_scoped_typedefname must be:

        ... TYPEDEFname :: TYPEDEFname

    with matching names at the end of the list.

    We  use the more general form to prevent a syntax conflict with a 
    typical    function    definition    (which    won't    have    a 
    constructor_init_list) */

constructor_function_definition:
global_or_scoped_typedefname parameter_type_list constructor_init_list_opt compound_statement { printf("global_or_scoped_typedefname parameter_type_list constructor_init_list_opt compound_statement %d\n", __LINE__); }
| declaration_specifier parameter_type_list constructor_init_list_opt compound_statement { printf("| declaration_specifier parameter_type_list constructor_init_list_opt compound_statement %d\n", __LINE__); }
        ;


    /*  Same  comments  as  seen  for constructor_function_definition 
    apply here */

constructor_function_declaration:
global_or_scoped_typedefname parameter_type_list { printf("global_or_scoped_typedefname parameter_type_list %d\n", __LINE__); }
| declaration_specifier      parameter_type_list  /* request to inline, no definition */ { printf("| declaration_specifier      parameter_type_list  /* request to inline, no definition */ %d\n", __LINE__); }
        ;


    /* The following use of declaration_specifiers are made to  allow 
    for  a TYPEDEFname preceded by an INLINE modifier. This fact must 
    be verified semantically.  It should also be  verified  that  the 
    TYPEDEFname  is  ACTUALLY  the  class name being elaborated. Note 
    that we could break out typedef_declaration_specifier from within 
    declaration_specifier, and we  might  narrow  down  the  conflict 
    region a bit. A second alternative (to what is done) for cleaning 
    up  this  stuff  is  to  let the tokenizer specially identify the 
    current class being elaborated as a special token, and not just a 
    typedefname. Unfortunately, things would get very  confusing  for 
    the  lexer,  as  we may pop into enclosed tag elaboration scopes; 
    into function definitions; or into both recursively! */

    /* I should make the following  rules  easier  to  annotate  with 
    scope  entry  and exit actions.  Note how hard it is to establish 
    the scope when you don't even know what the decl_spec is!! It can 
    be done with $-1 hacking, but I should not encourage users to  do 
    this directly. */

constructor_function_in_class:
declaration_specifier   constructor_parameter_list_and_body { printf("declaration_specifier   constructor_parameter_list_and_body %d\n", __LINE__); }
| TYPEDEFname           constructor_parameter_list_and_body { printf("| TYPEDEFname           constructor_parameter_list_and_body %d\n", __LINE__); }
        ;


    /* The following conflicts with member declarations-
    constructor_parameter_list_and_body:
          parameter_type_list ';'
          | parameter_type_list constructor_init_list_opt compound_statement
          ;
    so parameter_type_list was expanded inline to get */

    /* C++, not ANSI C */
constructor_parameter_list_and_body:
'('                           ')' Const_Volatile_list_opt ';' { printf("'('                           ')' Const_Volatile_list_opt ';' %d\n", __LINE__); }
| '(' type_name initializer     ')' Const_Volatile_list_opt ';' { printf("| '(' type_name initializer     ')' Const_Volatile_list_opt ';'  %d\n", __LINE__); }
| '(' named_parameter_type_list ')' Const_Volatile_list_opt ';' { printf("| '(' named_parameter_type_list ')' Const_Volatile_list_opt ';' %d\n", __LINE__); }
| '('                           ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement { printf("| '('                           ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement %d\n", __LINE__); }
| '(' type_name initializer     ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement { printf("| '(' type_name initializer     ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement %d\n", __LINE__); }
| '(' named_parameter_type_list ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement { printf("| '(' named_parameter_type_list ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement %d\n", __LINE__); }
| constructor_conflicting_parameter_list_and_body { printf("| constructor_conflicting_parameter_list_and_body %d\n", __LINE__); }
        ;


    /* The following conflicted with member declaration-
    constructor_conflicting_parameter_list_and_body:
        '('   type_name ')'                 Const_Volatile_list_opt ';'
        | '(' type_name ')'                 Const_Volatile_list_opt
                constructor_init_list_opt compound_statement
        ;
    so type_name was inline expanded to get the following... */


    /*  Note  that by inline expanding Const_Volatile_opt in a few of 
    the following rules I can transform 3  RR  conflicts  into  3  SR 
    conflicts.  Since  all the conflicts have a look ahead of ';', it 
    doesn't  really  matter  (also,  there  are  no   bad   LALR-only 
    components in the conflicts) */

constructor_conflicting_parameter_list_and_body:
'(' type_specifier                 ')' Const_Volatile_list_opt ';' { printf("'(' type_specifier                 ')' Const_Volatile_list_opt ';' %d\n", __LINE__); }
| '(' basic_type_name              ')' Const_Volatile_list_opt ';' { printf("| '(' basic_type_name              ')' Const_Volatile_list_opt ';' %d\n", __LINE__); }
| '(' TYPEDEFname                  ')' Const_Volatile_list_opt ';' { printf("| '(' TYPEDEFname                  ')' Const_Volatile_list_opt ';' %d\n", __LINE__); }
| '(' global_or_scoped_typedefname ')' Const_Volatile_list_opt ';' { printf("| '(' global_or_scoped_typedefname ')' Const_Volatile_list_opt ';' %d\n", __LINE__); }
| '(' Const_Volatile_list          ')' Const_Volatile_list_opt ';' { printf("| '(' Const_Volatile_list          ')' Const_Volatile_list_opt ';' %d\n", __LINE__); }
| '(' type_specifier               abstract_declarator ')' Const_Volatile_list_opt ';' { printf("| '(' type_specifier               abstract_declarator ')' Const_Volatile_list_opt ';' %d\n", __LINE__); }
| '(' basic_type_name              abstract_declarator ')' Const_Volatile_list_opt ';' { printf("| '(' basic_type_name              abstract_declarator ')' Const_Volatile_list_opt ';' %d\n", __LINE__); }

        /* missing entry posted below */

| '(' global_or_scoped_typedefname abstract_declarator ')' Const_Volatile_list_opt ';' { printf("| '(' global_or_scoped_typedefname abstract_declarator ')' Const_Volatile_list_opt ';' %d\n", __LINE__); }
| '(' Const_Volatile_list          abstract_declarator ')' Const_Volatile_list_opt ';' { printf("| '(' Const_Volatile_list          abstract_declarator ')' Const_Volatile_list_opt ';' %d\n", __LINE__); }
| '(' type_specifier               ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement { printf("| '(' type_specifier               ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement %d\n", __LINE__); }
| '(' basic_type_name              ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement { printf("| '(' basic_type_name              ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement %d\n", __LINE__); }
| '(' TYPEDEFname                  ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement { printf("| '(' TYPEDEFname                  ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement %d\n", __LINE__); }
| '(' global_or_scoped_typedefname ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement { printf("| '(' global_or_scoped_typedefname ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement %d\n", __LINE__); }
| '(' Const_Volatile_list           ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement { printf("| '(' Const_Volatile_list           ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement %d\n", __LINE__); }
| '(' type_specifier  abstract_declarator ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement { printf("| '(' type_specifier  abstract_declarator ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement %d\n", __LINE__); }
| '(' basic_type_name abstract_declarator ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement { printf("| '(' basic_type_name abstract_declarator ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement %d\n", __LINE__); }
        /* missing entry posted below */

| '(' global_or_scoped_typedefname abstract_declarator ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement { printf("| '(' global_or_scoped_typedefname abstract_declarator ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement %d\n", __LINE__); }
| '(' Const_Volatile_list          abstract_declarator ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement { printf("| '(' Const_Volatile_list          abstract_declarator ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement %d\n", __LINE__); }
| constructor_conflicting_typedef_declarator  { printf("| constructor_conflicting_typedef_declarator ; %d\n", __LINE__); }
;


    /* The following have ambiguities with member declarations-
    constructor_conflicting_typedef_declarator:
      '(' TYPEDEFname abstract_declarator ')' Const_Volatile_list_opt
                ';'
      |  '(' TYPEDEFname abstract_declarator ')' Const_Volatile_list_opt
                constructor_init_list_opt compound_statement
      ;
    which can be deferred by expanding abstract_declarator, and in two
    cases parameter_qualifier_list, resulting in ...*/

constructor_conflicting_typedef_declarator:
'(' TYPEDEFname unary_abstract_declarator          ')' Const_Volatile_list_opt ';' { printf("'(' TYPEDEFname unary_abstract_declarator          ')' Const_Volatile_list_opt ';' %d\n", __LINE__); }
| '(' TYPEDEFname unary_abstract_declarator       ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement { printf("| '(' TYPEDEFname unary_abstract_declarator       ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement %d\n", __LINE__); }
| '(' TYPEDEFname postfix_abstract_declarator     ')' Const_Volatile_list_opt ';' { printf("| '(' TYPEDEFname postfix_abstract_declarator     ')' Const_Volatile_list_opt ';' %d\n", __LINE__); }
| '(' TYPEDEFname postfix_abstract_declarator     ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement { printf("| '(' TYPEDEFname postfix_abstract_declarator     ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement %d\n", __LINE__); }
| '(' TYPEDEFname postfixing_abstract_declarator  ')' Const_Volatile_list_opt ';' { printf("| '(' TYPEDEFname postfixing_abstract_declarator  ')' Const_Volatile_list_opt ';' %d\n", __LINE__); }
| '(' TYPEDEFname postfixing_abstract_declarator  ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement { printf("| '(' TYPEDEFname postfixing_abstract_declarator  ')' Const_Volatile_list_opt constructor_init_list_opt compound_statement %d\n", __LINE__); }
        ;


constructor_init_list_opt:
/* nothing */ { printf("/* nothing */ %d\n", __LINE__); }
| constructor_init_list { printf("| constructor_init_list %d\n", __LINE__); }
        ;

constructor_init_list:
':' constructor_init { printf("':' constructor_init %d\n", __LINE__); }
| constructor_init_list ',' constructor_init { printf("| constructor_init_list ',' constructor_init %d\n", __LINE__); }
        ;

constructor_init:
IDENTIFIER   '(' argument_expression_list ')' { printf("IDENTIFIER   '(' argument_expression_list ')' %d\n", __LINE__); }
| IDENTIFIER '('                          ')' { printf("| IDENTIFIER '('                          ')' %d\n", __LINE__); }
| TYPEDEFname '(' argument_expression_list ')' { printf("| TYPEDEFname '(' argument_expression_list ')' %d\n", __LINE__); }
| TYPEDEFname '('                          ')' { printf("| TYPEDEFname '('                          ')' %d\n", __LINE__); }
| global_or_scoped_typedefname '(' argument_expression_list ')' { printf("| global_or_scoped_typedefname '(' argument_expression_list ')' %d\n", __LINE__); }
| global_or_scoped_typedefname '('                          ')' { printf("| global_or_scoped_typedefname '('                          ')' %d\n", __LINE__); }
| '(' argument_expression_list ')' /* Single inheritance ONLY*/ { printf("| '(' argument_expression_list ')' /* Single inheritance ONLY*/ %d\n", __LINE__); }
| '(' ')' /* Is this legal? It might be default! */ { printf("| '(' ')' /* Is this legal? It might be default! */ %d\n", __LINE__); }
        ;

declarator:
identifier_declarator { printf("identifier_declarator %d\n", __LINE__); }
| typedef_declarator { printf("| typedef_declarator %d\n", __LINE__); }
        ;

typedef_declarator:
paren_typedef_declarator          /* would be ambiguous as parameter*/ { printf("paren_typedef_declarator          /* would be ambiguous as parameter*/ %d\n", __LINE__); }
| simple_paren_typedef_declarator /* also ambiguous */ { printf("| simple_paren_typedef_declarator /* also ambiguous */ %d\n", __LINE__); }
| parameter_typedef_declarator    /* not ambiguous as parameter*/ { printf("| parameter_typedef_declarator    /* not ambiguous as parameter*/ %d\n", __LINE__); }
        ;

parameter_typedef_declarator:
TYPEDEFname { printf("TYPEDEFname %d\n", __LINE__); }
| TYPEDEFname postfixing_abstract_declarator { printf("| TYPEDEFname postfixing_abstract_declarator %d\n", __LINE__); }
| clean_typedef_declarator { printf("| clean_typedef_declarator %d\n", __LINE__); }
        ;


    /*  The  following  have  at  least  one  '*'or '&'.  There is no 
    (redundant) '(' between the '*'/'&'  and  the  TYPEDEFname.  This 
    definition  is  critical  in  that  a redundant paren that it too 
    close to the TYPEDEFname (i.e.,  nothing  between  them  at  all) 
    would  make  the TYPEDEFname into a parameter list, rather than a 
    declarator.*/

clean_typedef_declarator:
clean_postfix_typedef_declarator { printf("clean_postfix_typedef_declarator %d\n", __LINE__); }
| asterisk_or_ampersand parameter_typedef_declarator { printf("| asterisk_or_ampersand parameter_typedef_declarator %d\n", __LINE__); }
| unary_modifier        parameter_typedef_declarator { printf("| unary_modifier        parameter_typedef_declarator %d\n", __LINE__); }
        ;

clean_postfix_typedef_declarator:
'('   clean_typedef_declarator ')' { printf("'('   clean_typedef_declarator ')' %d\n", __LINE__); }
| '(' clean_typedef_declarator ')' postfixing_abstract_declarator { printf("| '(' clean_typedef_declarator ')' postfixing_abstract_declarator %d\n", __LINE__); }
        ;


    /* The following have a redundant '(' placed immediately  to  the 
    left  of the TYPEDEFname.  This opens up the possibility that the 
    TYPEDEFname is really the start of a parameter list, and *not*  a 
    declarator*/

paren_typedef_declarator:
postfix_paren_typedef_declarator { printf("postfix_paren_typedef_declarator %d\n", __LINE__); }
| asterisk_or_ampersand '(' simple_paren_typedef_declarator ')' { printf("| asterisk_or_ampersand '(' simple_paren_typedef_declarator ')' %d\n", __LINE__); }
| unary_modifier        '(' simple_paren_typedef_declarator ')' { printf("| unary_modifier        '(' simple_paren_typedef_declarator ')' %d\n", __LINE__); }
| asterisk_or_ampersand '(' TYPEDEFname ')' /* redundant paren */ { printf("| asterisk_or_ampersand '(' TYPEDEFname ')' /* redundant paren */ %d\n", __LINE__); }
| unary_modifier        '(' TYPEDEFname ')' /* redundant paren */ { printf("| unary_modifier        '(' TYPEDEFname ')' /* redundant paren */ %d\n", __LINE__); }
| asterisk_or_ampersand paren_typedef_declarator { printf("| asterisk_or_ampersand paren_typedef_declarator %d\n", __LINE__); }
| unary_modifier        paren_typedef_declarator { printf("| unary_modifier        paren_typedef_declarator %d\n", __LINE__); }
        ;

postfix_paren_typedef_declarator:
'(' paren_typedef_declarator ')' { printf("'(' paren_typedef_declarator ')' %d\n", __LINE__); }
| '(' simple_paren_typedef_declarator postfixing_abstract_declarator ')' { printf("| '(' simple_paren_typedef_declarator postfixing_abstract_declarator ')' %d\n", __LINE__); }
| '(' TYPEDEFname postfixing_abstract_declarator ')'              /* redundant paren */ { printf("| '(' TYPEDEFname postfixing_abstract_declarator ')'              /* redundant paren */ %d\n", __LINE__); }
| '(' paren_typedef_declarator ')' postfixing_abstract_declarator { printf("| '(' paren_typedef_declarator ')' postfixing_abstract_declarator %d\n", __LINE__); }
        ;


    /*  The following excludes lone TYPEDEFname to help in a conflict 
    resolution.  We have special cased lone  TYPEDEFname  along  side 
    all uses of simple_paren_typedef_declarator */

simple_paren_typedef_declarator:
'(' TYPEDEFname ')' { printf("'(' TYPEDEFname ')' %d\n", __LINE__); }
| '(' simple_paren_typedef_declarator ')' { printf("| '(' simple_paren_typedef_declarator ')' %d\n", __LINE__); }
        ;

identifier_declarator:
unary_identifier_declarator { printf("unary_identifier_declarator %d\n", __LINE__); }
| paren_identifier_declarator { printf("| paren_identifier_declarator %d\n", __LINE__); }
        ;


    /*  The  following  allows  "function return array of" as well as 
    "array of function returning".  It COULD be cleaned  up  the  way 
    abstract  declarators  have been.  This change might make it hard 
    to recover from user's syntax errors, whereas now they appear  as 
    simple constraint errors. */

unary_identifier_declarator:
postfix_identifier_declarator { printf("postfix_identifier_declarator %d\n", __LINE__); }
| asterisk_or_ampersand identifier_declarator { printf("| asterisk_or_ampersand identifier_declarator %d\n", __LINE__); }
| unary_modifier        identifier_declarator { printf("| unary_modifier        identifier_declarator %d\n", __LINE__); }
        ;

postfix_identifier_declarator:
paren_identifier_declarator           postfixing_abstract_declarator { printf("paren_identifier_declarator           postfixing_abstract_declarator %d\n", __LINE__); }
| '(' unary_identifier_declarator ')' { printf("| '(' unary_identifier_declarator ')' %d\n", __LINE__); }
| '(' unary_identifier_declarator ')' postfixing_abstract_declarator { printf("| '(' unary_identifier_declarator ')' postfixing_abstract_declarator %d\n", __LINE__); }
        ;

old_function_declarator:
postfix_old_function_declarator { printf("postfix_old_function_declarator %d\n", __LINE__); }
| asterisk_or_ampersand old_function_declarator { printf("| asterisk_or_ampersand old_function_declarator %d\n", __LINE__); }
| unary_modifier      old_function_declarator { printf("| unary_modifier      old_function_declarator %d\n", __LINE__); }
        ;


    /*  ANSI  C  section  3.7.1  states  "An identifier declared as a 
    typedef name shall not be redeclared as a parameter".  Hence  the 
    following is based only on IDENTIFIERs.

    Instead  of identifier_lists, an argument_expression_list is used 
    in  old  style  function   definitions.    The   ambiguity   with 
    constructors   required   the  use  of  argument  lists,  with  a 
    constraint verification of the list (e_g.: check to see that  the 
    "expressions" consisted of lone identifiers).

    An interesting ambiguity appeared:
        const constant=5;
        int foo(constant) ...

    Is  this an old function definition or constructor?  The decision 
    is made later by THIS grammar based on trailing context :-). This 
    ambiguity is probably what caused many parsers to give up on  old 
    style function definitions. */

postfix_old_function_declarator:
paren_identifier_declarator '(' argument_expression_list ')' { printf("paren_identifier_declarator '(' argument_expression_list ')' %d\n", __LINE__); }
| '(' old_function_declarator ')' { printf("| '(' old_function_declarator ')' %d\n", __LINE__); }
| '(' old_function_declarator ')' old_postfixing_abstract_declarator { printf("| '(' old_function_declarator ')' old_postfixing_abstract_declarator %d\n", __LINE__); }
        ;

old_postfixing_abstract_declarator:
array_abstract_declarator /* array modifiers */ { printf("array_abstract_declarator /* array modifiers */ %d\n", __LINE__); }
| old_parameter_type_list  /* function returning modifiers */ { printf("| old_parameter_type_list  /* function returning modifiers */ %d\n", __LINE__); }
        ;

abstract_declarator:
unary_abstract_declarator { printf("unary_abstract_declarator %d\n", __LINE__); }
| postfix_abstract_declarator { printf("| postfix_abstract_declarator %d\n", __LINE__); }
| postfixing_abstract_declarator { printf("| postfixing_abstract_declarator %d\n", __LINE__); }
        ;

postfixing_abstract_declarator:
array_abstract_declarator { printf("array_abstract_declarator %d\n", __LINE__); }
| parameter_type_list { printf("| parameter_type_list %d\n", __LINE__); }
        ;

array_abstract_declarator:
'[' ']' { printf("'[' ']' %d\n", __LINE__); }
| '[' constant_expression ']' { printf("| '[' constant_expression ']' %d\n", __LINE__); }
| array_abstract_declarator '[' constant_expression ']' { printf("| array_abstract_declarator '[' constant_expression ']' %d\n", __LINE__); }
        ;

unary_abstract_declarator:
asterisk_or_ampersand { printf("asterisk_or_ampersand %d\n", __LINE__); }
| unary_modifier { printf("| unary_modifier %d\n", __LINE__); }
| asterisk_or_ampersand abstract_declarator { printf("| asterisk_or_ampersand abstract_declarator %d\n", __LINE__); }
| unary_modifier        abstract_declarator { printf("| unary_modifier        abstract_declarator %d\n", __LINE__); }
        ;

postfix_abstract_declarator:
'(' unary_abstract_declarator ')' { printf("'(' unary_abstract_declarator ')' %d\n", __LINE__); }
| '(' postfix_abstract_declarator ')' { printf("| '(' postfix_abstract_declarator ')' %d\n", __LINE__); }
| '(' postfixing_abstract_declarator ')' { printf("| '(' postfixing_abstract_declarator ')' %d\n", __LINE__); }
| '(' unary_abstract_declarator ')' postfixing_abstract_declarator { printf("| '(' unary_abstract_declarator ')' postfixing_abstract_declarator %d\n", __LINE__); }
        ;

asterisk_or_ampersand:
'*' { printf("'*' %d\n", __LINE__); }
| '&' { printf("| '&' %d\n", __LINE__); }
        ;

unary_modifier:
scope '*' Const_Volatile_list_opt { printf("scope '*' Const_Volatile_list_opt %d\n", __LINE__); }
| asterisk_or_ampersand Const_Volatile_list { printf("| asterisk_or_ampersand Const_Volatile_list %d\n", __LINE__); }
        ;



/************************* NESTED SCOPE SUPPORT ******************************/


    /*  The  actions taken in the rules that follow involve notifying 
    the lexer that it should use the scope specified to determine  if 
    the  next  IDENTIFIER  token is really a TYPEDEFname token.  Note 
    that the actions must be taken before the parse has a  chance  to 
    "look-ahead" at the token that follows the "::", and hence should 
    be  done  during  a  reduction to "scoping_name" (which is always 
    followed by CLCL).  Since we are defining an  LR(1)  grammar,  we 
    are  assured  that  an action specified *before* the :: will take 
    place before the :: is shifted, and hence before the  token  that 
    follows the CLCL is scanned/lexed. */

    /*  Note that at the end of each of the following rules we should 
    be sure that the tag name is  in,  or  placed  in  the  indicated 
    scope.   If  no  scope  is  specified, then we must add it to our 
    current scope IFF it cannot  be  found  in  an  external  lexical 
    scope. */

scoping_name:
tag_name { printf("tag_name %d\n", __LINE__); }
| aggregate_key tag_name { printf("| aggregate_key tag_name %d\n", __LINE__); }
                          /* also update symbol table here by notifying it about a (possibly) new tag*/
        ;

scope:
scoping_name CLCL { printf("scoping_name CLCL %d\n", __LINE__); }
| scope scoping_name  CLCL { printf("| scope scoping_name  CLCL %d\n", __LINE__); }
        ;


    /*  Don't try to simplify the count of non-terminals by using one 
    of the other definitions of  "IDENTIFIER  or  TYPEDEFname"  (like 
    "label").   If you reuse such a non-terminal, 2 RR conflicts will 
    appear. The conflicts are LALR-only. The underlying cause of  the 
    LALR-only   conflict   is  that  labels,  are  followed  by  ':'.  
    Similarly, structure elaborations which provide a derivation have 
    have ':' just  after  tag_name  This  reuse,  with  common  right 
    context, is too much for an LALR parser. */

tag_name:
IDENTIFIER { printf("IDENTIFIER %d\n", __LINE__); }
| TYPEDEFname { printf("| TYPEDEFname %d\n", __LINE__); }
        ;

global_scope:
{ /*scan for upcoming name in file scope */ } CLCL { printf("{ /*scan for upcoming name in file scope */ } CLCL %d\n", __LINE__); }
        ;

global_or_scope:
global_scope { printf("global_scope %d\n", __LINE__); }
| scope { printf("| scope %d\n", __LINE__); }
| global_scope scope { printf("| global_scope scope %d\n", __LINE__); }
        ;


    /*  The  following can be used in an identifier based declarator. 
    (Declarators  that  redefine  an  existing  TYPEDEFname   require 
    special  handling,  and are not included here).  In addition, the 
    following are valid "identifiers" in  an  expression,  whereas  a 
    TYPEDEFname is NOT.*/

scope_opt_identifier:
IDENTIFIER { printf("IDENTIFIER %d\n", __LINE__); }
| scope IDENTIFIER  /* C++ not ANSI C */ { printf("| scope IDENTIFIER  /* C++ not ANSI C */ %d\n", __LINE__); }
        ;

scope_opt_complex_name:
complex_name { printf("complex_name %d\n", __LINE__); }
| scope complex_name { printf("| scope complex_name %d\n", __LINE__); }
        ;

complex_name:
'~' TYPEDEFname { printf("'~' TYPEDEFname %d\n", __LINE__); }
| operator_function_name { printf("| operator_function_name %d\n", __LINE__); }
        ;


    /*  Note that the derivations for global_opt_scope_opt_identifier 
    and global_opt_scope_opt_complex_name must be  placed  after  the 
    derivation:

       paren_identifier_declarator : scope_opt_identifier

    There  are several states with RR conflicts on "(", ")", and "[".  
    In these states we give up and assume a declaration, which  means 
    resolving   in  favor  of  paren_identifier_declarator.  This  is 
    basically the "If it can be  a  declaration  rule...",  with  our 
    finite cut off. */

global_opt_scope_opt_identifier:
global_scope scope_opt_identifier { printf("global_scope scope_opt_identifier %d\n", __LINE__); }
|            scope_opt_identifier { printf("|            scope_opt_identifier %d\n", __LINE__); }
        ;

global_opt_scope_opt_complex_name:
global_scope scope_opt_complex_name { printf("global_scope scope_opt_complex_name %d\n", __LINE__); }
|            scope_opt_complex_name { printf("|            scope_opt_complex_name %d\n", __LINE__); }
        ;


    /*  Note  that we exclude a lone TYPEDEFname.  When all alone, it 
    gets involved in a lot of ambiguities (re: function like cast  vs 
    declaration),   and  hence  must  be  special  cased  in  several 
    contexts. Note that generally every use of scoped_typedefname  is 
    accompanied by a parallel production using lone TYPEDEFname */

scoped_typedefname:
scope TYPEDEFname { printf("scope TYPEDEFname %d\n", __LINE__); }
        ;

global_or_scoped_typedefname:
scoped_typedefname { printf("scoped_typedefname %d\n", __LINE__); }
| global_scope scoped_typedefname { printf("| global_scope scoped_typedefname %d\n", __LINE__); }
| global_scope TYPEDEFname { printf("| global_scope TYPEDEFname %d\n", __LINE__); }
        ;

global_opt_scope_opt_typedefname:
TYPEDEFname { printf("TYPEDEFname %d\n", __LINE__); }
| global_or_scoped_typedefname { printf("| global_or_scoped_typedefname %d\n", __LINE__); }
        ;

%%
        
int yyerror(char * string)
{
    printf("parser error: %s\n", string);

    return 0;
}

SInt accsMgr::doParse()
{
    return (yyparse() == 0 ? IDE_SUCCESS : IDE_FAILURE);
}
    

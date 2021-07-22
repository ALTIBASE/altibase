
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     EM_OPTION_INC = 258,
     EM_INCLUDE = 259,
     EM_SEMI = 260,
     EM_EQUAL = 261,
     EM_COMMA = 262,
     EM_LPAREN = 263,
     EM_RPAREN = 264,
     EM_LBRAC = 265,
     EM_RBRAC = 266,
     EM_DQUOTE = 267,
     EM_PATHNAME = 268,
     EM_FILENAME = 269,
     M_INCLUDE = 270,
     M_DEFINE = 271,
     M_IFDEF = 272,
     M_IFNDEF = 273,
     M_UNDEF = 274,
     M_ENDIF = 275,
     M_IF = 276,
     M_ELIF = 277,
     M_ELSE = 278,
     M_SKIP_MACRO = 279,
     M_LBRAC = 280,
     M_RBRAC = 281,
     M_DQUOTE = 282,
     M_FILENAME = 283,
     M_DEFINED = 284,
     M_NUMBER = 285,
     M_CHARACTER = 286,
     M_BACKSLASHNEWLINE = 287,
     M_IDENTIFIER = 288,
     M_FUNCTION = 289,
     M_STRING_LITERAL = 290,
     M_RIGHT_OP = 291,
     M_LEFT_OP = 292,
     M_OR_OP = 293,
     M_LE_OP = 294,
     M_GE_OP = 295,
     M_LPAREN = 296,
     M_RPAREN = 297,
     M_AND_OP = 298,
     M_EQ_OP = 299,
     M_NE_OP = 300,
     M_NEWLINE = 301,
     M_CONSTANT = 302
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 21 "ulpPreprocy.y"

    char *strval;



/* Line 1676 of yacc.c  */
#line 105 "ulpPreprocy.hpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif





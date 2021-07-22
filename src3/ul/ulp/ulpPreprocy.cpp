
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         PPparse
#define yylex           PPlex
#define yyerror         PPerror
#define yylval          PPlval
#define yychar          PPchar
#define yydebug         PPdebug
#define yynerrs         PPnerrs


/* Copy the first part of user declarations.  */


/* Line 189 of yacc.c  */
#line 81 "ulpPreprocy.cpp"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


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

/* Line 214 of yacc.c  */
#line 21 "ulpPreprocy.y"

    char *strval;



/* Line 214 of yacc.c  */
#line 170 "ulpPreprocy.cpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */

/* Line 264 of yacc.c  */
#line 43 "ulpPreprocy.y"


#include <ulpPreproc.h>


#undef YY_READ_BUF_SIZE
#undef YY_BUF_SIZE
#define YY_READ_BUF_SIZE (16384)
#define YY_BUF_SIZE (YY_READ_BUF_SIZE * 2) /* size of default input buffer */

//============== global variables for PPparse ================//

// parser의 시작 상태를 지정하는 변수.
SInt            gUlpPPStartCond = PP_ST_NONE;
// parser가 이전 상태로 복귀하기위한 변수.
SInt            gUlpPPPrevCond  = PP_ST_NONE;

// PPIF parser에서 사용되는 버퍼 시작/끝 pointer
SChar *gUlpPPIFbufptr = NULL;
SChar *gUlpPPIFbuflim = NULL;

// 아래의 macro if 처리 class를 ulpPPLexer 클래스 안으로 넣고 싶지만,
// sun장비에서 initial 되지 않는 알수없는 문제가있어 밖으로 다뺌.
ulpPPifstackMgr *gUlpPPifstackMgr[MAX_HEADER_FILE_NUM];
// gUlpPPifstackMgr index
SInt             gUlpPPifstackInd = -1;

/* externs of PPLexer */
// include header 파싱중인지 여부.
extern idBool        gUlpPPisCInc;
extern idBool        gUlpPPisSQLInc;

/* externs of ulpMain.h */
extern ulpProgOption gUlpProgOption;
extern ulpCodeGen    gUlpCodeGen;
extern iduMemory    *gUlpMem;
// Macro table
extern ulpMacroTable gUlpMacroT;
// preprocessor parsing 중 발생한 에러에대한 코드정보.
extern SChar        *gUlpPPErrCode;

//============================================================//


//============ Function declarations for PPparse =============//

// Macro if 구문 처리를 위한 parse 함수
extern SInt PPIFparse ( void *aBuf, SInt *aRes );
extern int  PPlex  ( YYSTYPE *lvalp );
extern void PPerror( const SChar* aMsg );

extern void ulpFinalizeError(void);

//============================================================//



/* Line 264 of yacc.c  */
#line 240 "ulpPreprocy.cpp"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   38

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  48
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  16
/* YYNRULES -- Number of rules.  */
#define YYNRULES  32
/* YYNRULES -- Number of states.  */
#define YYNSTATES  55

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   302

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     4,     7,    10,    12,    14,    20,    26,
      30,    36,    38,    42,    44,    46,    48,    50,    52,    54,
      56,    58,    60,    65,    70,    73,    76,    79,    81,    83,
      86,    89,    91
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      49,     0,    -1,    -1,    49,    50,    -1,    49,    54,    -1,
      51,    -1,    52,    -1,     4,    10,    14,    11,     5,    -1,
       4,    12,    14,    12,     5,    -1,     4,    14,     5,    -1,
       3,     6,    53,     9,     5,    -1,    14,    -1,    53,     7,
      14,    -1,    55,    -1,    56,    -1,    57,    -1,    60,    -1,
      61,    -1,    58,    -1,    59,    -1,    62,    -1,    63,    -1,
      15,    25,    28,    26,    -1,    15,    27,    28,    27,    -1,
      16,    33,    -1,    16,    34,    -1,    19,    33,    -1,    21,
      -1,    22,    -1,    17,    33,    -1,    18,    33,    -1,    23,
      -1,    20,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   150,   150,   152,   157,   165,   192,   200,   216,   231,
     249,   253,   277,   324,   328,   332,   336,   341,   346,   351,
     356,   361,   365,   394,   424,   459,   498,   506,   572,   649,
     705,   746,   783
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "EM_OPTION_INC", "EM_INCLUDE", "EM_SEMI",
  "EM_EQUAL", "EM_COMMA", "EM_LPAREN", "EM_RPAREN", "EM_LBRAC", "EM_RBRAC",
  "EM_DQUOTE", "EM_PATHNAME", "EM_FILENAME", "M_INCLUDE", "M_DEFINE",
  "M_IFDEF", "M_IFNDEF", "M_UNDEF", "M_ENDIF", "M_IF", "M_ELIF", "M_ELSE",
  "M_SKIP_MACRO", "M_LBRAC", "M_RBRAC", "M_DQUOTE", "M_FILENAME",
  "M_DEFINED", "M_NUMBER", "M_CHARACTER", "M_BACKSLASHNEWLINE",
  "M_IDENTIFIER", "M_FUNCTION", "M_STRING_LITERAL", "M_RIGHT_OP",
  "M_LEFT_OP", "M_OR_OP", "M_LE_OP", "M_GE_OP", "M_LPAREN", "M_RPAREN",
  "M_AND_OP", "M_EQ_OP", "M_NE_OP", "M_NEWLINE", "M_CONSTANT", "$accept",
  "preprocessor", "Emsql_grammar", "Emsql_include", "Emsql_include_option",
  "Emsql_include_path_list", "Macro_grammar", "Macro_include",
  "Macro_define", "Macro_undef", "Macro_if", "Macro_elif", "Macro_ifdef",
  "Macro_ifndef", "Macro_else", "Macro_endif", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    48,    49,    49,    49,    50,    50,    51,    51,    51,
      52,    53,    53,    54,    54,    54,    54,    54,    54,    54,
      54,    54,    55,    55,    56,    56,    57,    58,    59,    60,
      61,    62,    63
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     2,     1,     1,     5,     5,     3,
       5,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     4,     4,     2,     2,     2,     1,     1,     2,
       2,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,     0,     0,     0,     0,     0,     0,     0,
      32,    27,    28,    31,     3,     5,     6,     4,    13,    14,
      15,    18,    19,    16,    17,    20,    21,     0,     0,     0,
       0,     0,     0,    24,    25,    29,    30,    26,    11,     0,
       0,     0,     9,     0,     0,     0,     0,     0,     0,    22,
      23,    12,    10,     7,     8
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     1,    14,    15,    16,    39,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -33
static const yytype_int8 yypact[] =
{
     -33,     0,   -33,     5,    -5,   -19,   -32,   -20,    -9,    -8,
     -33,   -33,   -33,   -33,   -33,   -33,   -33,   -33,   -33,   -33,
     -33,   -33,   -33,   -33,   -33,   -33,   -33,    12,    13,    14,
       9,     1,     2,   -33,   -33,   -33,   -33,   -33,   -33,     3,
      20,    21,   -33,     6,     7,    22,    30,    32,    33,   -33,
     -33,   -33,   -33,   -33,   -33
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -33,   -33,   -33,   -33,   -33,   -33,   -33,   -33,   -33,   -33,
     -33,   -33,   -33,   -33,   -33,   -33
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
       2,    33,    34,     3,     4,    28,    31,    29,    32,    30,
      45,    27,    46,    35,    42,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    36,    37,    38,    40,    41,    43,
      44,    47,    49,    48,    50,    52,    51,    53,    54
};

static const yytype_uint8 yycheck[] =
{
       0,    33,    34,     3,     4,    10,    25,    12,    27,    14,
       7,     6,     9,    33,     5,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    33,    33,    14,    14,    14,    28,
      28,    11,    26,    12,    27,     5,    14,     5,     5
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    49,     0,     3,     4,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    50,    51,    52,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,     6,    10,    12,
      14,    25,    27,    33,    34,    33,    33,    33,    14,    53,
      14,    14,     5,    28,    28,     7,     9,    11,    12,    26,
      27,    14,     5,     5,     5
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */





/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

/* User initialization code.  */

/* Line 1242 of yacc.c  */
#line 26 "ulpPreprocy.y"
{
    // parse option에 따라 초기 상태를 지정해줌.
    switch ( gUlpProgOption.mOptParseInfo )
    {
        case PARSE_NONE :
            gUlpPPStartCond = PP_ST_INIT_SKIP;
            break;
        case PARSE_PARTIAL :
        case PARSE_FULL :
            gUlpPPStartCond = PP_ST_MACRO;
            break;
        default :
            break;
    }
}

/* Line 1242 of yacc.c  */
#line 1322 "ulpPreprocy.cpp"

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:

/* Line 1455 of yacc.c  */
#line 153 "ulpPreprocy.y"
    {
        /* 이전 상태로 복귀 SKIP_NONE 상태 혹은 MACRO상태 에서 emsql구문을 파싱할 수 있기때문 */
        gUlpPPStartCond = gUlpPPPrevCond;
    ;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 158 "ulpPreprocy.y"
    {
        /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
         * 전이 된다. 나머지 경우는 MACRO상태로 전이됨. */
    ;}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 166 "ulpPreprocy.y"
    {
        /* EXEC SQL INCLUDE ... */
        SChar sStrtmp[MAX_FILE_PATH_LEN];

        // 내장될 헤더파일의 시작을 알린다. my girlfriend
        idlOS::snprintf( sStrtmp, MAX_FILE_PATH_LEN, "@$LOVELY.K.J.H$ (%s)\n",
                         gUlpProgOption.ulpGetIncList() );
        WRITESTR2BUFPP( sStrtmp );

        /* BUG-27683 : iostream 사용 제거 */
        // 0. flex 버퍼 상태 저장.
        ulpPPSaveBufferState();

        gUlpPPisSQLInc = ID_TRUE;

        // 1. doPPparse()를 재호출한다.
        doPPparse( gUlpProgOption.ulpGetIncList() );

        gUlpPPisSQLInc = ID_FALSE;

        // 내장될 헤더파일의 끝을 알린다.
        WRITESTR2BUFPP((SChar *)"#$LOVELY.K.J.H$\n");

        // 2. precompiler를 실행한 directory를 current path로 재setting
        idlOS::strcpy( gUlpProgOption.mCurrentPath, gUlpProgOption.mStartPath );
    ;}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 193 "ulpPreprocy.y"
    {
        /* EXEC SQL OPTION( INCLUDE = ... ) */
        // 하위 노드에서 처리함.
    ;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 201 "ulpPreprocy.y"
    {
        // Emsql_include 문법은 제일 뒤에 반드시 ';' 가 와야함.
        // check exist header file in include paths
        if ( gUlpProgOption.ulpLookupHeader( (yyvsp[(3) - (5)].strval), ID_FALSE )
             == IDE_FAILURE )
        {
            ulpErrorMgr sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_FILE_NOT_FOUND,
                             (yyvsp[(3) - (5)].strval) );
            gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            PPerror( ulpGetErrorMSG(&sErrorMgr) );
        }

    ;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 217 "ulpPreprocy.y"
    {
        // check exist header file in include paths
        if ( gUlpProgOption.ulpLookupHeader( (yyvsp[(3) - (5)].strval), ID_FALSE )
             == IDE_FAILURE )
        {
            ulpErrorMgr sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_FILE_NOT_FOUND,
                             (yyvsp[(3) - (5)].strval) );
            gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            PPerror( ulpGetErrorMSG(&sErrorMgr) );
        }

    ;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 232 "ulpPreprocy.y"
    {
        // check exist header file in include paths
        if ( gUlpProgOption.ulpLookupHeader( (yyvsp[(2) - (3)].strval), ID_FALSE )
             == IDE_FAILURE )
        {
            ulpErrorMgr sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_FILE_NOT_FOUND,
                             (yyvsp[(2) - (3)].strval) );
            gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            PPerror( ulpGetErrorMSG(&sErrorMgr) );
        }

    ;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 254 "ulpPreprocy.y"
    {

        SChar sPath[MAX_FILE_PATH_LEN];

        // path name 길이 에러 체크 필요.
        idlOS::strncpy( sPath, (yyvsp[(1) - (1)].strval), MAX_FILE_PATH_LEN-1 );

        // include path가 추가되면 g_ProgOption.path에 저장한다.
        if ( sPath[0] == IDL_FILE_SEPARATOR )
        {
            // 절대경로인 경우
            idlOS::sprintf( gUlpProgOption.mIncludePathList[gUlpProgOption.mIncludePathCnt],
                            sPath );
        }
        else
        {
            // 상대경로인 경우
              idlOS::sprintf( gUlpProgOption.mIncludePathList[gUlpProgOption.mIncludePathCnt],
                              "%s%c%s", gUlpProgOption.mCurrentPath, IDL_FILE_SEPARATOR, sPath);
        }
        gUlpProgOption.mIncludePathCnt++;

    ;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 278 "ulpPreprocy.y"
    {

        SChar sPath[MAX_FILE_PATH_LEN];

        idlOS::strncpy( sPath, (yyvsp[(3) - (3)].strval), MAX_FILE_PATH_LEN-1 );

        // include path가 추가되면 g_ProgOption.path에 저장한다.
        if ( sPath[0] == IDL_FILE_SEPARATOR )
        {
            // 절대경로인 경우
            idlOS::sprintf( gUlpProgOption.mIncludePathList[gUlpProgOption.mIncludePathCnt],
                            sPath );
        }
        else
        {
            // 상대경로인 경우
              idlOS::sprintf( gUlpProgOption.mIncludePathList[gUlpProgOption.mIncludePathCnt],
                              "%s%c%s", gUlpProgOption.mCurrentPath, IDL_FILE_SEPARATOR, sPath);
        }
        gUlpProgOption.mIncludePathCnt++;

    ;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 325 "ulpPreprocy.y"
    {
            gUlpPPStartCond = PP_ST_MACRO;
        ;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 329 "ulpPreprocy.y"
    {
            gUlpPPStartCond = PP_ST_MACRO;
        ;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 333 "ulpPreprocy.y"
    {
            gUlpPPStartCond = PP_ST_MACRO;
        ;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 337 "ulpPreprocy.y"
    {
            /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
             * 전이 된다. */
        ;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 342 "ulpPreprocy.y"
    {
            /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
             * 전이 된다. */
        ;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 347 "ulpPreprocy.y"
    {
            /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
             * 전이 된다. */
        ;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 352 "ulpPreprocy.y"
    {
            /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
             * 전이 된다. */
        ;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 357 "ulpPreprocy.y"
    {
            /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
             * 전이 된다. */
        ;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 366 "ulpPreprocy.y"
    {
            /* #include <...> */

            // 1. check exist header file in include paths
            if ( gUlpProgOption.ulpLookupHeader( (yyvsp[(3) - (4)].strval), ID_TRUE )
                 == IDE_FAILURE )
            {
                //do nothing
            }
            else
            {

                // 현재 #include 처리다.
                gUlpPPisCInc = ID_TRUE;

                /* BUG-27683 : iostream 사용 제거 */
                // 2. flex 버퍼 상태 저장.
                ulpPPSaveBufferState();
                // 3. doPPparse()를 재호출한다.
                doPPparse( gUlpProgOption.ulpGetIncList() );
                // 전에 #inlcude 처리중이었나? 확인함
                gUlpPPisCInc = gUlpProgOption.ulpIsHeaderCInclude();

                // 4. precompiler를 실행한 directory를 current path로 재setting
                idlOS::strcpy( gUlpProgOption.mCurrentPath, gUlpProgOption.mStartPath );
            }

        ;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 395 "ulpPreprocy.y"
    {

            // 1. check exist header file in include paths
            if ( gUlpProgOption.ulpLookupHeader( (yyvsp[(3) - (4)].strval), ID_TRUE )
                 == IDE_FAILURE )
            {
                // do nothing
            }
            else
            {

                // 현재 #include 처리다.
                gUlpPPisCInc = ID_TRUE;
                /* BUG-27683 : iostream 사용 제거 */
                // 2. flex 버퍼 상태 저장.
                ulpPPSaveBufferState();
                // 3. doPPparse()를 재호출한다.
                doPPparse( gUlpProgOption.ulpGetIncList() );
                // 전에 #inlcude 처리중이었나? 확인함
                gUlpPPisCInc = gUlpProgOption.ulpIsHeaderCInclude();

                // 4. precompiler를 실행한 directory를 current path로 재setting
                idlOS::strcpy( gUlpProgOption.mCurrentPath, gUlpProgOption.mStartPath );
            }

        ;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 425 "ulpPreprocy.y"
    {
            SChar sTmpDEFtext[ MAX_MACRO_DEFINE_CONTENT_LEN ];

            ulpPPEraseBN4MacroText( sTmpDEFtext, ID_FALSE );

            //printf("ID=[%s], TEXT=[%s]\n",$<strval>2,sTmpDEFtext);
            if ( sTmpDEFtext[0] == '\0' )
            {
                // macro symbol table에 추가함.
                if( gUlpMacroT.ulpMDefine( (yyvsp[(2) - (2)].strval), NULL, ID_FALSE ) == IDE_FAILURE )
                {
                    ulpErrorMgr sErrorMgr;

                    ulpSetErrorCode( &sErrorMgr,
                                     ulpERR_ABORT_Memory_Alloc_Error );
                    gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                    PPerror( ulpGetErrorMSG(&sErrorMgr) );
                }
            }
            else
            {
                // macro symbol table에 추가함.
                if( gUlpMacroT.ulpMDefine( (yyvsp[(2) - (2)].strval), sTmpDEFtext, ID_FALSE ) == IDE_FAILURE )
                {
                    ulpErrorMgr sErrorMgr;

                    ulpSetErrorCode( &sErrorMgr,
                                     ulpERR_ABORT_Memory_Alloc_Error );
                    gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                    PPerror( ulpGetErrorMSG(&sErrorMgr) );
                }
            }

        ;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 460 "ulpPreprocy.y"
    {
            // function macro의경우 인자 정보는 따로 저장되지 않는다.
            SChar sTmpDEFtext[ MAX_MACRO_DEFINE_CONTENT_LEN ];

            ulpPPEraseBN4MacroText( sTmpDEFtext, ID_FALSE );

            // #define A() {...} 이면, macro id는 A이다.
            if ( sTmpDEFtext[0] == '\0' )
            {
                // macro symbol table에 추가함.
                if ( gUlpMacroT.ulpMDefine( (yyvsp[(2) - (2)].strval), NULL, ID_TRUE ) == IDE_FAILURE )
                {
                    ulpErrorMgr sErrorMgr;

                    ulpSetErrorCode( &sErrorMgr,
                                     ulpERR_ABORT_Memory_Alloc_Error );
                    gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                    PPerror( ulpGetErrorMSG(&sErrorMgr) );
                }
            }
            else
            {
                // macro symbol table에 추가함.
                if ( gUlpMacroT.ulpMDefine( (yyvsp[(2) - (2)].strval), sTmpDEFtext, ID_TRUE ) == IDE_FAILURE )
                {
                    ulpErrorMgr sErrorMgr;

                    ulpSetErrorCode( &sErrorMgr,
                                     ulpERR_ABORT_Memory_Alloc_Error );
                    gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                    PPerror( ulpGetErrorMSG(&sErrorMgr) );
                }
            }

        ;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 499 "ulpPreprocy.y"
    {
            // $<strval>2 를 macro symbol table에서 삭제 한다.
            gUlpMacroT.ulpMUndef( (yyvsp[(2) - (2)].strval) );
        ;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 507 "ulpPreprocy.y"
    {
            SInt  sVal;
            SChar sTmpExpBuf[MAX_MACRO_IF_EXPR_LEN];

            switch( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpPrevIfStatus() )
            {
                case PP_IF_IGNORE :
                    // 단순히 token만 소모하는 역할이다. PPIFparse 호출하지 않는다.
                    ulpPPEraseBN4MacroText( sTmpExpBuf, ID_TRUE );
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IF, PP_IF_IGNORE );
                    break;

                case PP_IF_TRUE :
                    // #if expression 을 복사해온다.
                    ulpPPEraseBN4MacroText( sTmpExpBuf, ID_TRUE );

                    gUlpPPIFbufptr = sTmpExpBuf;
                    gUlpPPIFbuflim = sTmpExpBuf + idlOS::strlen(sTmpExpBuf);
                    if ( PPIFparse( sTmpExpBuf, &sVal ) != 0 )
                    {
                        //error 처리
                        ulpErrorMgr sErrorMgr;

                        ulpSetErrorCode( &sErrorMgr,
                                         ulpERR_ABORT_COMP_IF_Macro_Syntax_Error );
                        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                        PPerror( ulpGetErrorMSG(&sErrorMgr) );
                    }

                    //idlOS::printf("## PPIF result value=%d\n",sVal);
                    /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
                    * 전이 된다. */
                    if ( sVal != 0 )
                    {
                        // true
                        gUlpPPStartCond = PP_ST_MACRO;
                        // if stack manager 에 결과 정보 push
                        gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IF, PP_IF_TRUE );
                    }
                    else
                    {
                        // false
                        gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                        // if stack manager 에 결과 정보 push
                        gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IF, PP_IF_FALSE );
                    }
                    break;

                case PP_IF_FALSE :
                    // 단순히 token만 소모하는 역할이다. PPIFparse 호출하지 않는다.
                    ulpPPEraseBN4MacroText( sTmpExpBuf, ID_TRUE );
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IF, PP_IF_IGNORE );
                    break;

                default:
                    IDE_ASSERT(0);
            }
        ;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 573 "ulpPreprocy.y"
    {
            SInt  sVal;
            SChar sTmpExpBuf[MAX_MACRO_IF_EXPR_LEN];

            // #elif 순서 문법 검사.
            if ( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfCheckGrammar( PP_ELIF )
                 == ID_FALSE )
            {
                //error 처리
                ulpErrorMgr sErrorMgr;

                ulpSetErrorCode( &sErrorMgr,
                                 ulpERR_ABORT_COMP_ELIF_Macro_Sequence_Error );
                gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                PPerror( ulpGetErrorMSG(&sErrorMgr) );
            }

            switch( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpPrevIfStatus() )
            {
                case PP_IF_IGNORE :
                    // 단순히 token만 소모하는 역할이다. PPIFparse 호출하지 않는다.
                    ulpPPEraseBN4MacroText( sTmpExpBuf, ID_TRUE );
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_ELIF, PP_IF_IGNORE );
                    break;

                case PP_IF_TRUE :
                    // 단순히 token만 소모하는 역할이다. PPIFparse 호출하지 않는다.
                    ulpPPEraseBN4MacroText( sTmpExpBuf, ID_TRUE );
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_ELIF, PP_IF_IGNORE );
                    break;

                case PP_IF_FALSE :
                    ulpPPEraseBN4MacroText( sTmpExpBuf, ID_TRUE );

                    gUlpPPIFbufptr = sTmpExpBuf;
                    gUlpPPIFbuflim = sTmpExpBuf + idlOS::strlen(sTmpExpBuf);
                    //idlOS::printf("## start PPELIF parser text:[%s]\n",sTmpExpBuf);
                    if ( PPIFparse( sTmpExpBuf, &sVal ) != 0 )
                    {
                        //error 처리
                        ulpErrorMgr sErrorMgr;

                        ulpSetErrorCode( &sErrorMgr,
                                         ulpERR_ABORT_COMP_ELIF_Macro_Syntax_Error );
                        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                        PPerror( ulpGetErrorMSG(&sErrorMgr) );
                    }
                    //idlOS::printf("## PPELIF result value=%d\n",sVal);

                    /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
                     * 전이 된다. */
                    if ( sVal != 0 )
                    {
                        gUlpPPStartCond = PP_ST_MACRO;
                        // if stack manager 에 결과 정보 push
                        gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_ELIF, PP_IF_TRUE );
                    }
                    else
                    {
                        gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                        // if stack manager 에 결과 정보 push
                        gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_ELIF, PP_IF_FALSE );
                    }
                    break;

                default:
                    IDE_ASSERT(0);
            }
        ;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 650 "ulpPreprocy.y"
    {

            switch( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpPrevIfStatus() )
            {
                case PP_IF_IGNORE :
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFDEF, PP_IF_IGNORE );
                    break;

                case PP_IF_TRUE :
                    /* BUG-28162 : SESC_DECLARE 부활  */
                    // pare가 full 이아니고, SESC_DECLARE가 오고, #include처리가 아닐때
                    // #ifdef SESC_DECLARE 예전처럼 처리됨.
                    if( (gUlpProgOption.mOptParseInfo != PARSE_FULL) &&
                        (idlOS::strcmp( (yyvsp[(2) - (2)].strval), "SESC_DECLARE" ) == 0 ) &&
                        (gUlpPPisCInc != ID_TRUE) )
                    {
                        gUlpPPStartCond = PP_ST_MACRO;
                        gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFDEF, PP_IF_SESC_DEC );
                        WRITESTR2BUFPP((SChar *)"EXEC SQL BEGIN DECLARE SECTION;");
                    }
                    else
                    {
                        // $<strval>2 를 macro symbol table에 존재하는지 확인한다.
                        if ( gUlpMacroT.ulpMLookup((yyvsp[(2) - (2)].strval)) != NULL )
                        {
                            // 존재한다
                            gUlpPPStartCond = PP_ST_MACRO;
                            // if stack manager 에 결과 정보 push
                            gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFDEF, PP_IF_TRUE );
                        }
                        else
                        {
                            // 존재안한다
                            gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                            // if stack manager 에 결과 정보 push
                            gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFDEF, PP_IF_FALSE );
                        }
                    }
                    break;

                case PP_IF_FALSE :
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFDEF, PP_IF_IGNORE );
                    break;

                default:
                    IDE_ASSERT(0);
            }
        ;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 706 "ulpPreprocy.y"
    {
            switch( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpPrevIfStatus() )
            {
                case PP_IF_IGNORE :
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFNDEF, PP_IF_IGNORE );
                    break;

                case PP_IF_TRUE :
                    // $<strval>2 를 macro symbol table에 존재하는지 확인한다.
                    if ( gUlpMacroT.ulpMLookup((yyvsp[(2) - (2)].strval)) != NULL )
                    {
                        // 존재한다
                        gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                        // if stack manager 에 결과 정보 push
                        gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFNDEF, PP_IF_FALSE );
                    }
                    else
                    {
                        // 존재안한다
                        gUlpPPStartCond = PP_ST_MACRO;
                        // if stack manager 에 결과 정보 push
                        gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFNDEF, PP_IF_TRUE );
                    }
                    break;

                case PP_IF_FALSE :
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFNDEF, PP_IF_IGNORE );
                    break;

                default:
                    IDE_ASSERT(0);
            }
        ;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 747 "ulpPreprocy.y"
    {
            // #else 순서 문법 검사.
            if ( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfCheckGrammar( PP_ELSE )
                 == ID_FALSE )
            {
                // error 처리
                ulpErrorMgr sErrorMgr;

                ulpSetErrorCode( &sErrorMgr,
                                 ulpERR_ABORT_COMP_ELSE_Macro_Sequence_Error );
                gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                PPerror( ulpGetErrorMSG(&sErrorMgr) );
            }

            switch( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpPrevIfStatus() )
            {
                case PP_IF_IGNORE :
                case PP_IF_TRUE :
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_ELSE, PP_IF_IGNORE );
                    break;

                case PP_IF_FALSE :
                    gUlpPPStartCond = PP_ST_MACRO;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_ELSE, PP_IF_TRUE );
                    break;

                default:
                    IDE_ASSERT(0);
            }
        ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 784 "ulpPreprocy.y"
    {
            idBool sSescDEC;

            // #endif 순서 문법 검사.
            if ( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfCheckGrammar( PP_ENDIF )
                 == ID_FALSE )
            {
                // error 처리
                ulpErrorMgr sErrorMgr;

                ulpSetErrorCode( &sErrorMgr,
                                 ulpERR_ABORT_COMP_ENDIF_Macro_Sequence_Error );
                gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                PPerror( ulpGetErrorMSG(&sErrorMgr) );
            }

            /* BUG-28162 : SESC_DECLARE 부활  */
            // if stack 을 이전 조건문 까지 pop 한다.
            sSescDEC = gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpop4endif();

            if( sSescDEC == ID_TRUE )
            {
                //#endif 를 아래 string으로 바꿔준다.
                WRITESTR2BUFPP((SChar *)"EXEC SQL END DECLARE SECTION;");
            }

            /* BUG-27961 : preprocessor의 중첩 #if처리시 #endif 다음소스 무조건 출력하는 버그  */
            if( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfneedSkip4Endif() == ID_TRUE )
            {
                // 출력 하지마라.
                gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
            }
            else
            {
                // 출력 해라.
                gUlpPPStartCond = PP_ST_MACRO;
            }
        ;}
    break;



/* Line 1455 of yacc.c  */
#line 2248 "ulpPreprocy.cpp"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 825 "ulpPreprocy.y"



int doPPparse( SChar *aFilename )
{
/***********************************************************************
 *
 * Description :
 *      The initial function of PPparser.
 *
 ***********************************************************************/
    int sRes;

    ulpPPInitialize( aFilename );

    gUlpPPifstackMgr[++gUlpPPifstackInd] = new ulpPPifstackMgr();

    sRes = PPparse();

    gUlpCodeGen.ulpGenWriteFile();

    delete gUlpPPifstackMgr[gUlpPPifstackInd];

    gUlpPPifstackInd--;

    ulpPPFinalize();

    return sRes;
}



/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

%pure_parser

%{
#include <ulpComp.h>
#include <sqlcli.h>
%}
%union
{
    int   intval;
    char *strval;
}
%initial-action
{
    idlOS::memset(&yyval, 0, sizeof(yyval));

    /* BUG-28061 : preprocessing을마치면 marco table을 초기화하고, *
     *             ulpComp 에서 재구축한다.                       */
    switch ( gUlpProgOption.mOptParseInfo )
    {
        // 옵션 -parse none 에 해당하는 상태.
        case PARSE_NONE :
            gUlpCOMPStartCond = CP_ST_NONE;
            break;
        // 옵션 -parse partial 에 해당하는 상태.
        case PARSE_PARTIAL :
            gUlpCOMPStartCond = CP_ST_PARTIAL;
            break;
        // 옵션 -parse full 에 해당하는 상태.
        case PARSE_FULL :
            gUlpCOMPStartCond = CP_ST_C;
            break;
        default :
            break;
    }
};


%{

#undef YY_READ_BUF_SIZE
#undef YY_BUF_SIZE
#define YY_READ_BUF_SIZE (16384)
#define YY_BUF_SIZE (YY_READ_BUF_SIZE * 2) /* size of default input buffer */

//============== global variables for COMPparse ================//

/* externs of ulpMain.h */
extern ulpProgOption gUlpProgOption;
extern ulpCodeGen    gUlpCodeGen;
extern iduMemory    *gUlpMem;

// Macro table
extern ulpMacroTable  gUlpMacroT;
// Scope table
extern ulpScopeTable  gUlpScopeT;
// Struct tabletable
extern ulpStructTable gUlpStructT;

/* externs of COMPLexer */
extern idBool         gDontPrint2file;
extern SInt           gUlpCOMPMacroExpIndex;
/* BUG-31831 : An additional error message is needed to notify 
the unacceptability of using varchar type in #include file.
include file 파싱중인지를 알려줌 */
extern SInt           gUlpCOMPIncludeIndex;

/* extern of PPIF parser */
extern SChar         *gUlpPPIFbufptr;
extern SChar         *gUlpPPIFbuflim;

// lexer의 시작상태를 지정함.
SInt                 gUlpCOMPStartCond = CP_ST_NONE;
/* 이전 상태로 복귀하기 위한 변수 */
SInt                 gUlpCOMPPrevCond  = CP_ST_NONE;

/* BUG-35518 Shared pointer should be supported in APRE */
SInt                 gUlpSharedPtrPrevCond  = CP_ST_NONE;

// parsing중에 상태 정보 & C 변수에 대한 정보 저장.
ulpParseInfo         gUlpParseInfo;

// 현제 scope depth
SInt                 gUlpCurDepth = 0;

// 현재 처리중인 stmt type
ulpStmtType          gUlpStmttype    = S_UNKNOWN;
// sql query string 을 저장해야하는지 여부. 
idBool               gUlpIsPrintStmt = ID_TRUE;

// 현재 처리중인 host변수의 indicator 정보
ulpSymTElement      *gUlpIndNode = NULL;
SChar                gUlpIndName[MAX_HOSTVAR_NAME_SIZE * 2];
// 현재 처리중인 host변수의 file option 변수 정보
SChar                gUlpFileOptName[MAX_HOSTVAR_NAME_SIZE * 2];

/* macro if 조건문처리를 위한 변수들. */
ulpPPifstackMgr     *gUlpCOMPifstackMgr[MAX_HEADER_FILE_NUM];
SInt                 gUlpCOMPifstackInd = -1;

/* BUG-46824 anonymous block 에서 object_name이 해석된 횟수 */
UInt                 gUlpProcObjCount = 0;
/* BUG-446824 anonymous block에서 첫번째 object_name을 저장하는 변수 */
SChar               *gUlpPSMObjName;

extern SChar        *gUlpCOMPErrCode;

//============================================================//


//=========== Function declarations for COMPparse ============//

// Macro if 구문 처리를 위한 parse 함수
extern SInt PPIFparse ( void *aBuf, SInt *aRes );
extern int  COMPlex   ( YYSTYPE *lvalp );
extern void COMPerror ( const SChar* aMsg );

extern void ulpFinalizeError(void);

//============================================================//

/* BUG-42357 */
extern int COMPlineno;

%}

/*** EOF token ***/
%token END_OF_FILE

/*** C tokens ***/
%token C_AUTO
%token C_BREAK
%token C_CASE
%token C_CHAR
%token C_VARCHAR
%token C_CONST
%token C_CONTINUE
%token C_DEFAULT
%token C_DO
%token C_DOUBLE
%token C_ENUM
%token C_EXTERN
%token C_FLOAT
%token C_FOR
%token C_GOTO
%token C_INT
%token C_LONG
%token C_REGISTER
%token C_RETURN
%token C_SHORT
%token C_SIGNED
%token C_SIZEOF
%token C_STATIC
%token C_STRUCT
%token C_SWITCH
%token C_TYPEDEF
%token C_UNION
%token C_UNSIGNED
%token C_VOID
%token C_VOLATILE
%token C_WHILE
%token C_ELIPSIS
%token C_ELSE
%token C_IF
%token C_CONSTANT
%token C_IDENTIFIER
%token C_TYPE_NAME
%token C_STRING_LITERAL
%token C_RIGHT_ASSIGN
%token C_LEFT_ASSIGN
%token C_ADD_ASSIGN
%token C_SUB_ASSIGN
%token C_MUL_ASSIGN
%token C_DIV_ASSIGN
%token C_MOD_ASSIGN
%token C_AND_ASSIGN
%token C_XOR_ASSIGN
%token C_OR_ASSIGN
%token C_INC_OP
%token C_DEC_OP
%token C_PTR_OP
%token C_AND_OP
%token C_EQ_OP
%token C_NE_OP
%token C_RIGHT_OP
%token C_LEFT_OP
%token C_OR_OP
%token C_LE_OP
%token C_GE_OP
%token C_APRE_BINARY
%token C_APRE_BINARY2
%token C_APRE_BIT
%token C_APRE_BYTES
%token C_APRE_VARBYTES
%token C_APRE_NIBBLE
%token C_APRE_INTEGER
%token C_APRE_NUMERIC
%token C_APRE_BLOB_LOCATOR
%token C_APRE_CLOB_LOCATOR
%token C_APRE_BLOB
%token C_APRE_CLOB
%token C_SQLLEN
%token C_SQL_TIMESTAMP_STRUCT
%token C_SQL_TIME_STRUCT
%token C_SQL_DATE_STRUCT
%token C_SQL_NUMERIC_STRUCT /* BUG-45933 */
%token C_SQL_DA_STRUCT /* BUG-41010  */
%token C_FAILOVERCB
%token C_NCHAR_CS
%token C_ATTRIBUTE  /* BUG-34691 */

/*** Macro tokens ***/
%token M_INCLUDE
%token M_DEFINE
%token M_UNDEF
%token M_FUNCTION
%token M_LBRAC
%token M_RBRAC
%token M_DQUOTE
%token M_FILENAME
%token M_IF
%token M_ELIF
%token M_ELSE
%token M_ENDIF
%token M_IFDEF
%token M_IFNDEF
%token M_CONSTANT
%token M_IDENTIFIER

/*** EmSQL tokens ***/
%token EM_SQLSTART
%token EM_ERROR

%token TR_ADD
%token TR_AFTER
%token TR_AGER
%token TR_ALL
%token TR_ALTER
%token TR_AND
%token TR_ANY
%token TR_ARCHIVE
%token TR_ARCHIVELOG
%token TR_AS
%token TR_ASC
%token TR_AT
%token TR_BACKUP
%token TR_BEFORE
%token TR_BEGIN
%token TR_BY
/* BUG-41010 Add dynamic binding feature on Apre */
%token TR_BIND
%token TR_CASCADE
%token TR_CASE
%token TR_CAST
%token TR_CHECK_OPENING_PARENTHESIS
%token TR_CLOSE
%token TR_COALESCE
%token TR_COLUMN
%token TR_COMMENT
%token TR_COMMIT
%token TR_COMPILE
%token TR_CONNECT
%token TR_CONSTRAINT
%token TR_CONSTRAINTS
%token TR_CONTINUE
%token TR_CREATE
%token TR_VOLATILE
%token TR_CURSOR
%token TR_CYCLE
%token TR_DATABASE
%token TR_DECLARE
%token TR_DEFAULT
%token TR_DELETE
%token TR_DEQUEUE
%token TR_DESC
%token TR_DIRECTORY
%token TR_DISABLE
%token TR_DISCONNECT
%token TR_DISTINCT
%token TR_DROP
/* BUG-41010 Add dynamic binding feature on Apre */
%token TR_DESCRIBE
%token TR_DESCRIPTOR 
%token TR_EACH
%token TR_ELSE
%token TR_ELSEIF
%token TR_ENABLE
%token TR_END
%token TR_ENQUEUE
%token TR_ESCAPE
%token TR_EXCEPTION
%token TR_EXEC
%token TR_EXECUTE
%token TR_EXIT
%token TR_FAILOVERCB
%token TR_FALSE
%token TR_FETCH
%token TR_FIFO
%token TR_FLUSH
%token TR_FOR
%token TR_FOREIGN
%token TR_FROM
%token TR_FULL
%token TR_FUNCTION
%token TR_GOTO
%token TR_GRANT
%token TR_GROUP
%token TR_HAVING
%token TR_IF
%token TR_IN
%token TR_IN_BF_LPAREN
%token TR_INNER
%token TR_INSERT
%token TR_INTERSECT
%token TR_INTO
%token TR_IS
%token TR_ISOLATION
%token TR_JOIN
%token TR_KEY
%token TR_LEFT
%token TR_LESS
%token TR_LEVEL
%token TR_LIFO
%token TR_LIKE
%token TR_LIMIT
%token TR_LOCAL
%token TR_LOGANCHOR
%token TR_LOOP
/* BUG-41010 Add dynamic binding feature on Apre */
%token TR_MERGE
%token TR_MOVE
%token TR_MOVEMENT
%token TR_NEW
%token TR_NOARCHIVELOG
%token TR_NOCYCLE
%token TR_NOT
%token TR_NULL
%token TR_OF
%token TR_OFF
%token TR_OLD
%token TR_ON
%token TR_OPEN
%token TR_OR
%token TR_ORDER
%token TR_OUT
%token TR_OUTER
%token TR_OVER
%token TR_PARTITION
%token TR_PARTITIONS
%token TR_POINTER
%token TR_PRIMARY
%token TR_PRIOR
%token TR_PRIVILEGES
%token TR_PROCEDURE
%token TR_PUBLIC
%token TR_QUEUE
%token TR_READ
%token TR_REBUILD
%token TR_RECOVER
%token TR_REFERENCES
%token TR_REFERENCING
%token TR_REGISTER
%token TR_RESTRICT
%token TR_RETURN
%token TR_REVOKE
%token TR_RIGHT
%token TR_ROLLBACK
%token TR_ROW
%token TR_SAVEPOINT
%token TR_SELECT
%token TR_SEQUENCE
%token TR_SESSION
%token TR_SET
%token TR_SOME
%token TR_SPLIT
%token TR_START
%token TR_STATEMENT
%token TR_SYNONYM
%token TR_TABLE
%token TR_TEMPORARY
%token TR_THAN
%token TR_THEN
%token TR_TO
%token TR_TRIGGER
%token TR_TRUE
%token TR_TYPE
%token TR_TYPESET
%token TR_UNION
%token TR_UNIQUE
%token TR_UNREGISTER
%token TR_UNTIL
%token TR_UPDATE
%token TR_USER
%token TR_USING
%token TR_VALUES
%token TR_VARIABLE
%token TR_VARIABLE_LARGE        /* BUG-43840 */
/* BUG-41010 Add dynamic binding feature on Apre */
%token TR_VARIABLES
%token TR_VIEW
%token TR_WHEN
%token TR_WHERE
%token TR_WHILE
%token TR_WITH
%token TR_WORK
%token TR_WRITE
%token TR_PARALLEL
%token TR_NOPARALLEL
%token TR_LOB
%token TR_STORE
%token TR_ENDEXEC
%token TR_PRECEDING
%token TR_FOLLOWING
%token TR_CURRENT_ROW
%token TR_LINK                  /* BUG-37100 */
%token TR_ROLE                  /* PROJ-1812 ROLE */
%token TR_WITHIN                // PROJ-2527 WITHIN GROUP AGGR
%token TR_LOGGING               /* BUG-46157 */

%token TK_BETWEEN
%token TK_EXISTS

%token TO_ACCESS
%token TO_CONSTANT
%token TO_IDENTIFIED
%token TO_INDEX
%token TO_MINUS
%token TO_MODE
%token TO_OTHERS
%token TO_RAISE
%token TO_RENAME
%token TO_REPLACE
%token TO_ROWTYPE
%token TO_SEGMENT
%token TO_WAIT
%token TO_PIVOT
%token TO_UNPIVOT
%token TO_MATERIALIZED
%token TO_CONNECT_BY_NOCYCLE
%token TO_CONNECT_BY_ROOT
%token TO_NULLS
%token TO_PURGE                 /* PROJ-2441 flashback */
%token TO_FLASHBACK             /* PROJ-2441 flashback */
%token TO_VC2COLL
%token TO_KEEP                  /* PROJ-2528 Keep Aggregation */

%token TA_ELSIF
%token TA_EXTENTSIZE
%token TA_FIXED
%token TA_LOCK
%token TA_MAXROWS
%token TA_ONLINE
%token TA_OFFLINE
%token TA_REPLICATION
%token TA_REVERSE
%token TA_ROWCOUNT
%token TA_STEP
%token TA_TABLESPACE
%token TA_TRUNCATE
%token TA_SQLCODE
%token TA_SQLERRM
%token TA_LINKER                    /* BUG-37100 */
%token TA_REMOTE_TABLE              /* BUG-37100 */
%token TA_SHARD                     /* PROJ-2638 */
%token TA_DISJOIN                   /* PROJ-1810 Partition Exchange */   
%token TA_CONJOIN                   /* BUG-42468 JOIN TO CONJOIN */
/* BUG-45502 */
%token TA_SEC
%token TA_MSEC
%token TA_USEC
%token TA_SECOND
%token TA_MILLISECOND
%token TA_MICROSECOND
%token TA_ANALYSIS_PROPAGATION       /* BUG-46157 */

%token TI_NONQUOTED_IDENTIFIER
%token TI_QUOTED_IDENTIFIER
%token TI_HOSTVARIABLE
%token TL_TYPED_LITERAL
%token TL_LITERAL
%token TL_NCHAR_LITERAL
%token TL_UNICODE_LITERAL
%token TL_INTEGER
%token TL_NUMERIC
%token TS_AT_SIGN
%token TS_CONCATENATION_SIGN
%token TS_DOUBLE_PERIOD
%token TS_EXCLAMATION_POINT
%token TS_PERCENT_SIGN
%token TS_OPENING_PARENTHESIS
%token TS_CLOSING_PARENTHESIS
%token TS_OPENING_BRACKET
%token TS_CLOSING_BRACKET
%token TS_ASTERISK
%token TS_PLUS_SIGN
%token TS_COMMA
%token TS_MINUS_SIGN
%token TS_PERIOD
%token TS_SLASH
%token TS_COLON
%token TS_SEMICOLON
%token TS_LESS_THAN_SIGN
%token TS_EQUAL_SIGN
%token TS_GREATER_THAN_SIGN
%token TS_QUESTION_MARK
%token TS_OUTER_JOIN_OPERATOR  /* PROJ-1653 Outer Join Operator (+) */
%token TX_HINTS

%token SES_V_NUMERIC
%token SES_V_INTEGER
%token SES_V_HOSTVARIABLE
%token SES_V_LITERAL
%token SES_V_TYPED_LITERAL
%token SES_V_DQUOTE_LITERAL
%token SES_V_IDENTIFIER
%token SES_V_ABSOLUTE
%token SES_V_ALLOCATE
%token SES_V_ASENSITIVE
%token SES_V_AUTOCOMMIT
%token SES_V_BATCH
%token SES_V_BLOB_FILE
%token SES_V_BREAK
%token SES_V_CLOB_FILE
%token SES_V_CUBE
%token SES_V_DEALLOCATE
%token SES_V_DESCRIPTOR
%token SES_V_DO
%token SES_V_FIRST
%token SES_V_FOUND
%token SES_V_FREE
%token SES_V_HOLD
%token SES_V_IMMEDIATE
%token SES_V_INDICATOR
%token SES_V_INSENSITIVE
%token SES_V_LAST
%token SES_V_NEXT
%token SES_V_ONERR
%token SES_V_ONLY
%token APRE_V_OPTION
%token SES_V_PREPARE
%token SES_V_RELATIVE
%token SES_V_RELEASE
%token SES_V_ROLLUP
%token SES_V_SCROLL
%token SES_V_SENSITIVE
%token SES_V_SQLERROR
%token SES_V_THREADS
%token SES_V_WHENEVER
%token SES_V_CURRENT
%token SES_V_GROUPING_SETS
%token SES_V_WITH_ROLLUP

%token SES_V_GET
%token SES_V_DIAGNOSTICS
%token SES_V_CONDITION
%token SES_V_NUMBER
%token SES_V_ROW_COUNT
%token SES_V_RETURNED_SQLCODE
%token SES_V_RETURNED_SQLSTATE
%token SES_V_MESSAGE_TEXT
%token SES_V_ROW_NUMBER
%token SES_V_COLUMN_NUMBER

%%

program
    : combined_grammar
    | program combined_grammar
    ;

combined_grammar
    : C_grammar
    | Emsql_grammar
    | Macro_grammar
    | END_OF_FILE
    {
        YYACCEPT;
    }
    ;


//=====================================================//
//                                                     //
//               ansi-C language grammar               //
//                                                     //
//=====================================================//

C_grammar
    : function_definition
    | declaration
    ;

primary_expr
    : identifier
    | C_CONSTANT
    | string_literal
    | '(' expr ')'
    ;

postfix_expr
    : primary_expr
    | postfix_expr '[' expr ']'
    | postfix_expr '(' ')'
    | postfix_expr '(' argument_expr_list ')'
    | postfix_expr '.' identifier
    | postfix_expr C_PTR_OP identifier
    | postfix_expr C_INC_OP
    | postfix_expr C_DEC_OP
    ;

argument_expr_list
    : assignment_expr
    | argument_expr_list ',' assignment_expr
    ;

unary_expr
    : postfix_expr
    | C_INC_OP unary_expr
    | C_DEC_OP unary_expr
    | unary_operator cast_expr
    | C_SIZEOF unary_expr
    | C_SIZEOF '(' type_name ')'
    ;

unary_operator
    : '&'
    | '*'
    | '+'
    | '-'
    | '~'
    | '!'
    ;

cast_expr
    : unary_expr
    | '(' type_name ')' cast_expr
    ;

multiplicative_expr
    : cast_expr
    | multiplicative_expr '*' cast_expr
    | multiplicative_expr '/' cast_expr
    | multiplicative_expr '%' cast_expr
    ;

additive_expr
    : multiplicative_expr
    | additive_expr '+' multiplicative_expr
    | additive_expr '-' multiplicative_expr
    ;

shift_expr
    : additive_expr
    | shift_expr C_LEFT_OP additive_expr
    | shift_expr C_RIGHT_OP additive_expr
    ;

relational_expr
    : shift_expr
    | relational_expr '<' shift_expr
    | relational_expr '>' shift_expr
    | relational_expr C_LE_OP shift_expr
    | relational_expr C_GE_OP shift_expr
    ;

equality_expr
    : relational_expr
    | equality_expr C_EQ_OP relational_expr
    | equality_expr C_NE_OP relational_expr
    ;

and_expr
    : equality_expr
    | and_expr '&' equality_expr
    ;

exclusive_or_expr
    : and_expr
    | exclusive_or_expr '^' and_expr
    ;

inclusive_or_expr
    : exclusive_or_expr
    | inclusive_or_expr '|' exclusive_or_expr
    ;

logical_and_expr
    : inclusive_or_expr
    | logical_and_expr C_AND_OP inclusive_or_expr
    ;

logical_or_expr
    : logical_and_expr
    | logical_or_expr C_OR_OP logical_and_expr
    ;

conditional_expr
    : logical_or_expr
    | logical_or_expr '?' logical_or_expr ':' conditional_expr
    ;

assignment_expr
    : conditional_expr
    | unary_expr assignment_operator assignment_expr
    ;

assignment_operator
    : '='
    | C_MUL_ASSIGN
    | C_DIV_ASSIGN
    | C_MOD_ASSIGN
    | C_ADD_ASSIGN
    | C_SUB_ASSIGN
    | C_LEFT_ASSIGN
    | C_RIGHT_ASSIGN
    | C_AND_ASSIGN
    | C_XOR_ASSIGN
    | C_OR_ASSIGN
    ;

expr
    : assignment_expr
    | expr ',' assignment_expr
    ;

constant_expr
    : conditional_expr
    ;

declaration
    : declaration_specifiers ';'
    {
        /* BUG-28118 : system 헤더파일들도 파싱돼야함.            *
         * 2th. problem : 빈구조체 선언이 허용안됨. ex) struct A; */
        // <type> ; 이 올수 있다. ex> int; char; struct A; ...
        gUlpParseInfo.ulpInitHostInfo();
    }
    | declaration_specifiers init_declarator_list ';'
    {
        iduListNode *sIterator = NULL;
        iduListNode *sNextNode = NULL;
        ulpSymTElement *sSymNode;

        if( gUlpParseInfo.mFuncDecl == ID_TRUE )
        {
            gUlpScopeT.ulpSDelScope( gUlpCurDepth + 1 );
        }

        /* BUG-35518 Shared pointer should be supported in APRE */
        /* convert the sentence for shared pointer */
        if ( gUlpParseInfo.mIsSharedPtr == ID_TRUE)
        {
            WRITESTR2BUFCOMP ( (SChar *)" */\n" );
            IDU_LIST_ITERATE_SAFE(&(gUlpParseInfo.mSharedPtrVarList),
                                  sIterator, sNextNode )
            {
                sSymNode = (ulpSymTElement *)sIterator->mObj;
                if ( gDontPrint2file != ID_TRUE )
                {
                    gUlpCodeGen.ulpGenSharedPtr( sSymNode );
                }
                IDU_LIST_REMOVE(sIterator);
                idlOS::free(sIterator);
            }
            IDU_LIST_INIT( &(gUlpParseInfo.mSharedPtrVarList) );
        }

        // varchar 선언의 경우 해당 code를 주석처리 한후,
        // struct { char arr[...]; SQLLEN len; } 으로의 변환이 필요함.
        if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_VARCHAR) ||
             (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_NVARCHAR) )
        {
            if ( gUlpParseInfo.mFuncDecl != ID_TRUE )
            {
                WRITESTR2BUFCOMP ( (SChar *)" */\n" );

                /* BUG-26375 valgrind bug */
                IDU_LIST_ITERATE_SAFE(&(gUlpParseInfo.mVarcharVarList),
                                      sIterator, sNextNode )
                {
                    sSymNode = (ulpSymTElement *)sIterator->mObj;
                    if ( gDontPrint2file != ID_TRUE )
                    {
                        gUlpCodeGen.ulpGenVarchar( sSymNode );
                    }
                    IDU_LIST_REMOVE(sIterator);
                    idlOS::free(sIterator);
                }
                IDU_LIST_INIT( &(gUlpParseInfo.mVarcharVarList) );
                gUlpParseInfo.mVarcharDecl = ID_FALSE;
            }
        }
   


        gUlpParseInfo.mFuncDecl = ID_FALSE;
        gUlpParseInfo.mHostValInfo4Typedef = NULL;
        // 하나의 선언구문이 처리되면 따로 저장하고 있던 호스트변수정보를 초기화함.
        gUlpParseInfo.ulpInitHostInfo();
    }
    ;

declaration_specifiers
    : storage_class_specifier
    | storage_class_specifier declaration_specifiers
    | type_specifier
    | type_specifier declaration_specifiers
    ;

init_declarator_list
    : init_declarator
    {
        SChar *sVarName;
        ulpSymTNode *sSymNode;
        iduListNode *sIterator = NULL;
        iduListNode *sVarcharListNode = NULL;
        /* BUG-35518 Shared pointer should be supported in APRE */
        iduListNode *sSharedPtrVarListNode = NULL;

        if( gUlpParseInfo.mFuncDecl != ID_TRUE )
        {

            if( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsTypedef != ID_TRUE )
            {   // typedef 정의가 아닐경우
                /* BUG-28118 : system 헤더파일들도 파싱돼야함.            *
                 * 5th. problem : 정의되지 않은 구조체 포인터 변수 선언안됨. *
                 * 8th. problem : can't resolve extern variable type at declaring section. */
                if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsstruct  == ID_TRUE) &&
                     (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName[0] != '\0') &&
                     (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink == NULL) )
                {   // struct 변수 선언의 경우.
                    // structure 변수 선언의경우 extern or pointer가 아니라면 struct table에서
                    // 해당 struct tag가 존재하는지 검사하며, extern or pointer일 경우에는 검사하지 않고
                    // 나중에 해당 변수를 사용할때 검사한다.
                    if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mPointer  == 0) &&
                         (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsExtern == ID_FALSE) )
                    {   // it's not a pointer of struct and extern.
                        gUlpParseInfo.mStructPtr = gUlpStructT.ulpStructLookupAll(
                                            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName,
                                            gUlpCurDepth );
                        if ( gUlpParseInfo.mStructPtr == NULL )
                        {
                            // error 처리

                            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                             ulpERR_ABORT_COMP_C_Unknown_Structname_Error,
                                             gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName );
                            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
                        }
                        else
                        {
                            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink
                                    = gUlpParseInfo.mStructPtr;
                        }
                    }
                    else
                    {   // it's a point or extern of struct
                        // do nothing
                    }
                }
            }
            else
            {
                if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsstruct   == ID_TRUE) &&
                     (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink == NULL) )
                {   // structure 를 typedef 정의할 경우.
                    if (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName[0] == '\0')
                    {   // no tag structure 를 typedef 정의할 경우.
                        // ex) typedef struct { ... } A;
                        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink
                                = gUlpParseInfo.mStructPtr;
                    }
                }
            }

            // char, varchar 변수의경우 -nchar_var 커맨드option에 포함된 변수인지 확인함.
            if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_CHAR) ||
                 (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_VARCHAR) )
            {
                IDU_LIST_ITERATE(&gUlpProgOption.mNcharVarNameList, sIterator)
                {
                    sVarName = (SChar* )sIterator->mObj;
                    if ( idlOS::strcmp( sVarName,
                         gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mName )
                         == 0 )
                    {
                        if ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_CHAR )
                        {
                            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_NCHAR;
                        }
                        else
                        {
                            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_NVARCHAR;
                        }
                    }
                }
            }

            // scope table에 해당 symbol node를 추가한다.
            if( (sSymNode = gUlpScopeT.ulpSAdd ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth],
                                                 gUlpCurDepth ))
                == NULL )
            {
                // error 처리

                ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                ulpERR_ABORT_COMP_C_Add_Symbol_Error,
                                gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mName );
                gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
            }

            //varchar type의 경우, 나중 코드 변환을 위해 list에 따로 저장한다.
            if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_VARCHAR) ||
                 (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_NVARCHAR)
               )
            {
                sVarcharListNode =
                    (iduListNode*)idlOS::malloc(ID_SIZEOF(iduListNode));
                if (sVarcharListNode == NULL)
                {
                    ulpSetErrorCode(&gUlpParseInfo.mErrorMgr, ulpERR_ABORT_Memory_Alloc_Error);
                    gUlpCOMPErrCode = ulpGetErrorSTATE(&gUlpParseInfo.mErrorMgr);
                    COMPerror(ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr));
                }
                else
                {
                    IDU_LIST_INIT_OBJ(sVarcharListNode, &(sSymNode->mElement));
                    IDU_LIST_ADD_LAST(&(gUlpParseInfo.mVarcharVarList), sVarcharListNode);
                }
            }

            /* BUG-35518 Shared pointer should be supported in APRE
             * Store tokens for shared pointer to convert */

            if ( gUlpParseInfo.mIsSharedPtr == ID_TRUE )
            {
                sSharedPtrVarListNode = (iduListNode*)idlOS::malloc(ID_SIZEOF(iduListNode));
                if (sSharedPtrVarListNode == NULL)
                {
                    ulpSetErrorCode(&gUlpParseInfo.mErrorMgr, ulpERR_ABORT_Memory_Alloc_Error);
                    gUlpCOMPErrCode = ulpGetErrorSTATE(&gUlpParseInfo.mErrorMgr);
                    COMPerror(ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr));
                }
                else
                {
                    IDU_LIST_INIT_OBJ(sSharedPtrVarListNode, &(sSymNode->mElement));
                    IDU_LIST_ADD_LAST(&(gUlpParseInfo.mSharedPtrVarList), sSharedPtrVarListNode);
                }

            }

        }
    }
    | init_declarator_list var_decl_list_begin init_declarator
    {
        SChar *sVarName;
        ulpSymTNode *sSymNode;
        iduListNode *sIterator = NULL;
        iduListNode *sVarcharListNode = NULL;

        if( gUlpParseInfo.mFuncDecl != ID_TRUE )
        {

            if( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsTypedef != ID_TRUE )
            {   // typedef 정의가 아닐경우

                /* BUG-28118 : system 헤더파일들도 파싱돼야함.            *
                 * 5th. problem : 정의되지 않은 구조체 포인터 변수 선언안됨. *
                 * 8th. problem : can't resolve extern variable type at declaring section. */
                if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsstruct  == ID_TRUE) &&
                     (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName[0] != '\0') &&
                     (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink == NULL) )
                {   // struct 변수 선언의 경우.
                    // structure 변수 선언의경우 pointer가 아니라면 struct table에서
                    // 해당 struct tag가 존재하는지 검사하며, pointer일 경우에는 검사하지 않고
                    // 나중에 해당 변수를 사용할때 검사한다.
                    if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mPointer  == 0) &&
                         (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsExtern == ID_FALSE) )
                    {   // it's not a pointer of struct and extern.

                        gUlpParseInfo.mStructPtr = gUlpStructT.ulpStructLookupAll(
                                            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName,
                                            gUlpCurDepth );
                        if ( gUlpParseInfo.mStructPtr == NULL )
                        {
                            // error 처리

                            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                             ulpERR_ABORT_COMP_C_Unknown_Structname_Error,
                                             gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName );
                            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
                        }
                        else
                        {
                            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink
                                    = gUlpParseInfo.mStructPtr;
                        }
                    }
                    else
                    {   // it's a point of struct
                        // do nothing
                    }
                }
            }
            else
            {
                // no tag structure 를 typedef 할경우.
                if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsstruct   == ID_TRUE) &&
                     (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink == NULL) )
                {   // structure 를 typedef 정의할 경우.
                    if (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName[0] == '\0')
                    {   // no tag structure 를 typedef 정의할 경우.
                        // ex) typedef struct { ... } A;
                        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink = gUlpParseInfo.mStructPtr;
                    }
                }
            }

            if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_CHAR) ||
                 (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_VARCHAR) )
            {
                IDU_LIST_ITERATE(&gUlpProgOption.mNcharVarNameList, sIterator)
                {
                    sVarName = (SChar* )sIterator->mObj;
                    if ( idlOS::strcmp( sVarName, gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mName )
                         == 0 )
                    {
                        if ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_CHAR )
                        {
                            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_NCHAR;
                        }
                        else
                        {
                            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_NVARCHAR;
                        }
                    }
                }
            }

            if( (sSymNode = gUlpScopeT.ulpSAdd ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth],
                                                 gUlpCurDepth ))
                == NULL )
            {
                // error 처리

                ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                ulpERR_ABORT_COMP_C_Add_Symbol_Error,
                                gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mName );
                gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
            }

            if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_VARCHAR) ||
                 (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_NVARCHAR) )
            {
                sVarcharListNode =
                    (iduListNode*)idlOS::malloc(ID_SIZEOF(iduListNode));
                if (sVarcharListNode == NULL)
                {
                    ulpSetErrorCode(&gUlpParseInfo.mErrorMgr, ulpERR_ABORT_Memory_Alloc_Error);
                    gUlpCOMPErrCode = ulpGetErrorSTATE(&gUlpParseInfo.mErrorMgr);
                    COMPerror(ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr));
                }
                else
                {
                    IDU_LIST_INIT_OBJ(sVarcharListNode, &(sSymNode->mElement));
                    IDU_LIST_ADD_LAST(&(gUlpParseInfo.mVarcharVarList), sVarcharListNode);
                }
            }
        }
    }
    ;

var_decl_list_begin
    : ','
    {
        // , 를 사용한 동일 type을 여러개 선언할 경우 필요한 초기화.
        gUlpParseInfo.mSaveId = ID_TRUE;
        if ( gUlpParseInfo.mHostValInfo4Typedef != NULL )
        {
            if ( gUlpParseInfo.mHostValInfo4Typedef->mArraySize[0] != '\0' )
            {
                idlOS::strncpy( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize,
                                gUlpParseInfo.mHostValInfo4Typedef->mArraySize,
                                MAX_NUMBER_LEN - 1 );
            }
            else
            {
                gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize[0]   = '\0';
            }

            if ( gUlpParseInfo.mHostValInfo4Typedef->mArraySize2[0] != '\0' )
            {
                idlOS::strncpy( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize2,
                                gUlpParseInfo.mHostValInfo4Typedef->mArraySize2,
                                MAX_NUMBER_LEN - 1 );
            }
            else
            {
                gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize2[0]  = '\0';
            }

            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mPointer
                    = gUlpParseInfo.mHostValInfo4Typedef->mPointer;
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsarray
                    = gUlpParseInfo.mHostValInfo4Typedef->mIsarray;
        }
        else
        {
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize[0]   = '\0';
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize2[0]  = '\0';
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mPointer        = 0;
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsarray        = ID_FALSE;
        }

    }
    ;

init_declarator
    : declarator
    | declarator '=' initializer
    ;

storage_class_specifier
    : C_TYPEDEF
    {
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsTypedef = ID_TRUE;
    }
    | C_EXTERN
    {
        /* BUG-28118 : system 헤더파일들도 파싱돼야함.                                 *
         * 8th. problem : can't resolve extern variable type at declaring section. */
        // extern 변수이고 standard type이 아니라면, 변수 선언시 type resolving을 하지않고,
        // 사용시 resolving을 하기위해 필요한 field.
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsExtern = ID_TRUE;
    }
    | C_STATIC
    | C_AUTO
    | C_REGISTER
    ;

type_specifier
    : C_CHAR
    {
        gUlpParseInfo.mSaveId = ID_TRUE;
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_CHAR;
    }
    | C_VARCHAR
    {
        /* BUG-31831 : An additional error message is needed to notify 
            the unacceptability of using varchar type in #include file. */
        if( gUlpCOMPIncludeIndex > 0 )
        {
            // error 처리
            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Varchar_In_IncludeFile_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
        gUlpParseInfo.mSaveId = ID_TRUE;
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_VARCHAR;
    }
    | C_CHAR C_NCHAR_CS
    {
        gUlpParseInfo.mSaveId = ID_TRUE;
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_NCHAR;
    }
    | C_VARCHAR C_NCHAR_CS
    {
        /* BUG-31831 : An additional error message is needed to notify 
            the unacceptability of using varchar type in #include file. */
        if( gUlpCOMPIncludeIndex > 0 )
        {
            // error 처리
            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Varchar_In_IncludeFile_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
        gUlpParseInfo.mSaveId = ID_TRUE;
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_NVARCHAR;
    }
    | C_SHORT
    {
        gUlpParseInfo.mSaveId = ID_TRUE;
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_SHORT;
    }
    | C_INT
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        switch ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType )
        {
            case H_SHORT:
            case H_LONG:
                break;
            default:
                gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_INT;
                break;
        }
    }
    | C_LONG
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        if ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_LONG )
        {
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_LONGLONG;
        }
        else
        {
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_LONG;
        }
    }
    | C_SIGNED
    {
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIssign = ID_TRUE;
    }
    | C_UNSIGNED
    {
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIssign = ID_FALSE;
    }
    | C_FLOAT
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_FLOAT;
    }
    | C_DOUBLE
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_DOUBLE;
    }
    | C_CONST
    | C_VOLATILE
    | C_VOID
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_UNKNOWN;
    }
    | struct_or_union_specifier
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_USER_DEFINED;
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsstruct = ID_TRUE;
    }
    | enum_specifier
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_UNKNOWN;
    }
    | C_TYPE_NAME
    {
        // In case of struct type or typedef type
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.ulpCopyHostInfo4Typedef( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth],
                                               gUlpParseInfo.mHostValInfo4Typedef );
    }
    | C_APRE_BINARY
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_BINARY;
    }
    | C_APRE_BINARY2  /* BUG-46418 */
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_BINARY2;
    }
    | C_APRE_BIT
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_BIT;
    }
    | C_APRE_BYTES
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_BYTES;
    }
    | C_APRE_VARBYTES
    {
        gUlpParseInfo.mSaveId = ID_TRUE;
        
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_VARBYTE;
    }
    | C_APRE_NIBBLE
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_NIBBLE;
    }
    | C_APRE_INTEGER
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_INTEGER;
    }
    | C_APRE_NUMERIC
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_NUMERIC;
    }
    | C_APRE_BLOB_LOCATOR
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_BLOBLOCATOR;
    }
    | C_APRE_CLOB_LOCATOR
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_CLOBLOCATOR;
    }
    | C_APRE_BLOB
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_BLOB;
    }
    | C_APRE_CLOB
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_CLOB;
    }
    | C_SQLLEN
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        if( ID_SIZEOF(SQLLEN) == 4 )
        {
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_INT;
        }
        else
        {
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_LONG;
        }
        // SQLLEN 은 무조건 signed
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIssign = ID_TRUE;
    }
    | C_SQL_TIMESTAMP_STRUCT
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_TIMESTAMP;
    }
    | C_SQL_TIME_STRUCT
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_TIME;
    }
    | C_SQL_DATE_STRUCT
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_DATE;
    }
    /* BUG-45933 */
    | C_SQL_NUMERIC_STRUCT
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_NUMERIC_STRUCT;
    }
    /* BUG-41010 Add dynamic binding feature on Apre */
    | C_SQL_DA_STRUCT
    {
        gUlpParseInfo.mSaveId = ID_TRUE;
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_SQLDA;
    }
    | C_FAILOVERCB
    {
        gUlpParseInfo.mSaveId = ID_TRUE;

        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_FAILOVERCB;
    }
    ;

attribute_specifier
    :
    | C_ATTRIBUTE
    ;

struct_or_union_specifier
   /* BUG-28118 : system 헤더파일들도 파싱돼야함.                                 *
    * 13th. problem : 구조체 안에 이름없는 구조체정의가 오면 처리못함.                */
    // struct <tag> {}
    : struct_or_union struct_decl_begin '}' attribute_specifier
    {
        if ( gUlpParseInfo.mStructDeclDepth > 0 )
        {
            idlOS::free( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth] );
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth] = NULL;
        }
        gUlpParseInfo.mStructDeclDepth--;
        if( gUlpParseInfo.mStructPtr != NULL )
        {
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink
                = gUlpParseInfo.mStructPtr;
        }
    }
    | struct_or_union struct_decl_begin struct_declaration_or_macro_list '}' attribute_specifier
    {
        /* BUG-28118 : system 헤더파일들도 파싱돼야함.                       *
         * 6th. problem : Nested structure 정의중 scope를 잘못 계산하는 문제 */
        if ( gUlpParseInfo.mStructDeclDepth > 0 )
        {
            idlOS::free( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth] );
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth] = NULL;
        }

        gUlpParseInfo.mStructDeclDepth--;

        // typedef struct 의 경우 mStructLink가 설정되지 않는다.
        // 이 경우 mStructLink가가 설정되는 시점은 해당 type을 이용해 변수를 선언하는 시점이다.
        if( gUlpParseInfo.mStructPtr != NULL )
        {
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink
                = gUlpParseInfo.mStructPtr;
        }
    }
   /* BUG-28118 : system 헤더파일들도 파싱돼야함.                                 *
    * 13th. problem : 구조체 안에 이름없는 구조체정의가 오면 처리못함.                */
    // struct {}
    | struct_or_union no_tag_struct_decl_begin '}' attribute_specifier
    {
        if ( gUlpParseInfo.mStructDeclDepth > 0 )
        {
            idlOS::free( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth] );
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth] = NULL;
        }
        gUlpParseInfo.mStructDeclDepth--;
        if( gUlpParseInfo.mStructPtr != NULL )
        {
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink
                    = gUlpParseInfo.mStructPtr;
        }
    }
    | struct_or_union no_tag_struct_decl_begin struct_declaration_or_macro_list '}' attribute_specifier
    {
        /* BUG-28118 : system 헤더파일들도 파싱돼야함.                       *
         * 6th. problem : Nested structure 정의중 scope를 잘못 계산하는 문제 */
        if ( gUlpParseInfo.mStructDeclDepth > 0 )
        {
            idlOS::free( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth] );
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth] = NULL;
        }
        gUlpParseInfo.mStructDeclDepth--;
        if( gUlpParseInfo.mStructPtr != NULL )
        {
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink
                    = gUlpParseInfo.mStructPtr;
        }
    }
    | struct_or_union identifier
    {
        /* BUG-28118 : system 헤더파일들도 파싱돼야함.                       *
         * 6th. problem : Nested structure 정의중 scope를 잘못 계산하는 문제 */
        if ( gUlpParseInfo.mStructDeclDepth > 0 )
        {
            idlOS::free( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth] );
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth] = NULL;
        }

        gUlpParseInfo.mStructDeclDepth--;

        /* BUG-27875 : 구조체안의 typedef type인식못함. */
        gUlpParseInfo.mSkipTypedef = ID_FALSE;

        /* BUG-28118 : system 헤더파일들도 파싱돼야함.            *
         * 2th. problem : 빈구조체 선언이 허용안됨. ex) struct A; *
         * 5th. problem : 정의되지 않은 구조체 포인터 변수 선언안됨. */
        // structure 이름 정보 저장함.
        idlOS::strcpy( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName,
                       $<strval>2 );
    }
    ;

struct_decl_begin
    : identifier '{'
    {
        /* BUG-27875 : 구조체안의 typedef type인식못함. */
        gUlpParseInfo.mSkipTypedef = ID_FALSE;
        // id가 struct table에 있는지 확인한다.
        if ( gUlpStructT.ulpStructLookup( $<strval>1, gUlpCurDepth - 1 )
             != NULL )
        {
            // error 처리
            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_C_Duplicate_Structname_Error,
                             $<strval>1 );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
        else
        {

            idlOS::strcpy( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth - 1]->mStructName,
                           $<strval>1 );
            // struct table에 저장한다.
            /* BUG-28118 : system 헤더파일들도 파싱돼야함.                       *
             * 6th. problem : Nested structure 정의중 scope를 잘못 계산하는 문제 */
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth - 1]->mStructLink
                    = gUlpStructT.ulpStructAdd ( $<strval>1, gUlpCurDepth );

            if ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth - 1]->mStructLink
                 == NULL )
            {
                // error 처리
                ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                 ulpERR_ABORT_COMP_C_Duplicate_Structname_Error,
                                 $<strval>1 );
                gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
            }
        }
    }
    ;

no_tag_struct_decl_begin
    : '{'
    {
        /* BUG-27875 : 구조체안의 typedef type인식못함. */
        gUlpParseInfo.mSkipTypedef = ID_FALSE;
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth - 1]->mStructName[0] = '\0';
        // struct table에 저장한다.
        // no tag struct node는 hash table 마지막 bucket에 추가된다.
        /* BUG-28118 : system 헤더파일들도 파싱돼야함.                       *
         * 6th. problem : Nested structure 정의중 scope를 잘못 계산하는 문제 */
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth - 1]->mStructLink
                = gUlpStructT.ulpNoTagStructAdd ();
    }
    ;

struct_or_union
    : C_STRUCT
    {
        /* BUG-27875 : 구조체안의 typedef type인식못함. */
        // 아래 구문을 처리하기위해 mSkipTypedef 변수 추가됨.
        // typedef struct Struct1 Struct1;
        // struct Struct1       <- mSkipTypedef = ID_TRUE  :
        //                          Struct1은 비록 이전에 typedef되어 있지만 렉서에서 C_TYPE_NAME이아닌
        // {                        C_IDENTIFIER로 인식되어야 한다.
        //    ...               <- mSkipTypedef = ID_FALSE :
        //    ...                   필드에 typedef 이름이 오면 C_TYPE_NAME으로 인식돼야한다.
        // };
        gUlpParseInfo.mSkipTypedef = ID_TRUE;
        gUlpParseInfo.mStructDeclDepth++;
        /* BUG-28118 : system 헤더파일들도 파싱돼야함.                       *
         * 6th. problem : Nested structure 정의중 scope를 잘못 계산하는 문제 */
        if( gUlpParseInfo.mStructDeclDepth >= MAX_NESTED_STRUCT_DEPTH )
        {
            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Exceed_Max_Nested_Struct_Depth_Error );
            ulpPrintfErrorCode( stderr,
                                &gUlpParseInfo.mErrorMgr);
            IDE_ASSERT(0);
        }
        else
        {
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]
                    = (ulpSymTElement *)idlOS::malloc(ID_SIZEOF( ulpSymTElement));
            if ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth] == NULL )
            {
                ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                 ulpERR_ABORT_Memory_Alloc_Error );
                ulpPrintfErrorCode( stderr,
                                    &gUlpParseInfo.mErrorMgr);
                IDE_ASSERT(0);
            }
            else
            {
                (void) gUlpParseInfo.ulpInitHostInfo();
            }
        }
        gUlpParseInfo.mSaveId      = ID_TRUE;
    }
    | C_UNION
    {
        /* BUG-27875 : 구조체안의 typedef type인식못함. */
        gUlpParseInfo.mSkipTypedef = ID_TRUE;
        gUlpParseInfo.mStructDeclDepth++;
        /* BUG-28118 : system 헤더파일들도 파싱돼야함.                       *
         * 6th. problem : Nested structure 정의중 scope를 잘못 계산하는 문제 */
        if( gUlpParseInfo.mStructDeclDepth >= MAX_NESTED_STRUCT_DEPTH )
        {
            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Exceed_Max_Nested_Struct_Depth_Error );
            ulpPrintfErrorCode( stderr,
                                &gUlpParseInfo.mErrorMgr);
            IDE_ASSERT(0);
        }
        else
        {
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]
                    = (ulpSymTElement *)idlOS::malloc(ID_SIZEOF( ulpSymTElement));
            if ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth] == NULL )
            {
                ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                 ulpERR_ABORT_Memory_Alloc_Error );
                ulpPrintfErrorCode( stderr,
                                    &gUlpParseInfo.mErrorMgr);
                IDE_ASSERT(0);
            }
            else
            {
                (void) gUlpParseInfo.ulpInitHostInfo();
            }
        }
        gUlpParseInfo.mSaveId      = ID_TRUE;
    }
    ;

/* BUG-28118 : system 헤더파일들도 파싱돼야함. */
// 4th. problem: C 구조체 선언 문법안에 MACRO 문법이 포함될수 있도록 매크로 문법 포함함.
struct_declaration_or_macro_list
    : struct_declaration_or_macro
    | struct_declaration_or_macro_list struct_declaration_or_macro
    ;

struct_declaration_or_macro
    : struct_declaration
    | Macro_grammar
    ;

struct_declaration
    : type_specifier_list struct_declarator_list ';'
    {
        iduListNode *sIterator = NULL;
        iduListNode *sNextNode = NULL;
        ulpSymTElement *sSymNode;

        if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_VARCHAR) ||
             (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_NVARCHAR) )
        {
            if ( gUlpParseInfo.mFuncDecl != ID_TRUE )
            {
                WRITESTR2BUFCOMP ( (SChar *)" */\n" );

                /* BUG-26375 valgrind bug */
                IDU_LIST_ITERATE_SAFE(&(gUlpParseInfo.mVarcharVarList),
                                        sIterator, sNextNode)
                {
                    sSymNode = (ulpSymTElement *)sIterator->mObj;
                    if ( gDontPrint2file != ID_TRUE )
                    {
                        gUlpCodeGen.ulpGenVarchar( sSymNode );
                    }
                    IDU_LIST_REMOVE(sIterator);
                    idlOS::free(sIterator);
                }
                IDU_LIST_INIT( &(gUlpParseInfo.mVarcharVarList) );
                gUlpParseInfo.mVarcharDecl = ID_FALSE;
            }
        }

        gUlpParseInfo.mHostValInfo4Typedef = NULL;
        gUlpParseInfo.ulpInitHostInfo();
    }
    ;

/* struct field 선언 문법 */
struct_declarator_list
   /* BUG-28118 : system 헤더파일들도 파싱돼야함.                                 *
    * 13th. problem : 구조체 안에 이름없는 구조체정의가 오면 처리못함.                */
    :
    | struct_declarator
    {
        SChar       *sVarName;
        ulpSymTNode *sSymNode;
        iduListNode *sIterator = NULL;
        iduListNode *sVarcharListNode = NULL;

        // field 이름 중복 검사함.
        if ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth - 1]->mStructLink->mChild->ulpSymLookup
             ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mName ) != NULL )
        {
            // error 처리
            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_C_Duplicate_Structname_Error,
                             gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mName );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
        else
        {
            /* BUG-28118 : system 헤더파일들도 파싱돼야함.            *
             * 5th. problem : 정의되지 않은 구조체 포인터 변수 선언안됨. *
             * 8th. problem : can't resolve extern variable type at declaring section. */
            if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsstruct  == ID_TRUE) &&
                 (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName[0] != '\0') &&
                 (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink == NULL) )
            {   // struct 변수 선언의 경우.
                // structure 변수 선언의경우 pointer가 아니라면 struct table에서
                // 해당 struct tag가 존재하는지 검사하며, pointer일 경우에는 검사하지 않고
                // 나중에 해당 변수를 사용할때 검사한다.
                if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mPointer  == 0) &&
                     (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsExtern == ID_FALSE) )
                {   // it's not a pointer of struct and extern.

                    gUlpParseInfo.mStructPtr = gUlpStructT.ulpStructLookupAll(
                                        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName,
                                        gUlpCurDepth );
                    if ( gUlpParseInfo.mStructPtr == NULL )
                    {
                        // error 처리

                        ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                        ulpERR_ABORT_COMP_C_Unknown_Structname_Error,
                                        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName );
                        gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                        COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
                    }
                    else
                    {
                        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink
                                = gUlpParseInfo.mStructPtr;
                    }
                }
                else
                {   // it's a point of struct
                    // do nothing
                }
            }

            if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_CHAR) ||
                 (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_VARCHAR) )
            {
                IDU_LIST_ITERATE(&gUlpProgOption.mNcharVarNameList, sIterator)
                {
                    sVarName = (SChar* )sIterator->mObj;
                    if ( idlOS::strcmp( sVarName, gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mName )
                         == 0 )
                    {
                        if ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_CHAR )
                        {
                            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_NCHAR;
                        }
                        else
                        {
                            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_NVARCHAR;
                        }
                    }
                }
            }

            /* BUG-28118 : system 헤더파일들도 파싱돼야함.                       *
             * 6th. problem : Nested structure 정의중 scope를 잘못 계산하는 문제 */
            // struct 필드를 add하려 한다면, mHostValInfo의 이전 index에 저장된 struct node pointer 를 이용해야함.
            sSymNode =
                    gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth - 1]->mStructLink
                    ->mChild->ulpSymAdd(
                                           gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]
                                       );

            if ( sSymNode != NULL )
            {
                if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_VARCHAR) ||
                     (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_NVARCHAR) )
                {
                    sVarcharListNode =
                        (iduListNode*)idlOS::malloc(ID_SIZEOF(iduListNode));
                    if (sVarcharListNode == NULL)
                    {
                        ulpSetErrorCode(&gUlpParseInfo.mErrorMgr, ulpERR_ABORT_Memory_Alloc_Error);
                        gUlpCOMPErrCode = ulpGetErrorSTATE(&gUlpParseInfo.mErrorMgr);
                        COMPerror(ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr));
                    }
                    else
                    {
                        IDU_LIST_INIT_OBJ(sVarcharListNode, &(sSymNode->mElement));
                        IDU_LIST_ADD_LAST(&(gUlpParseInfo.mVarcharVarList), sVarcharListNode);
                    }
                }
            }
        }
    }
    | struct_declarator_list struct_decl_list_begin struct_declarator
    {
        SChar *sVarName;
        ulpSymTNode *sSymNode;
        iduListNode *sIterator = NULL;
        iduListNode *sVarcharListNode = NULL;

        // field 이름 중복 검사함.
        if ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth - 1]->mStructLink->mChild->ulpSymLookup
             ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mName ) != NULL )
        {
            // error 처리
            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_C_Duplicate_Structname_Error,
                             gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mName );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
        else
        {
            /* BUG-28118 : system 헤더파일들도 파싱돼야함.            *
             * 5th. problem : 정의되지 않은 구조체 포인터 변수 선언안됨. *
             * 8th. problem : can't resolve extern variable type at declaring section. */
            if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsstruct  == ID_TRUE) &&
                 (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName[0] != '\0') &&
                 (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink == NULL) )
            {   // struct 변수 선언의 경우.
                // structure 변수 선언의경우 pointer가 아니라면 struct table에서
                // 해당 struct tag가 존재하는지 검사하며, pointer일 경우에는 검사하지 않고
                // 나중에 해당 변수를 사용할때 검사한다.
                if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mPointer  == 0) &&
                     (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsExtern == ID_FALSE) )
                {   // it's not a pointer of struct and extern.

                    gUlpParseInfo.mStructPtr = gUlpStructT.ulpStructLookupAll(
                                        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName,
                                        gUlpCurDepth );
                    if ( gUlpParseInfo.mStructPtr == NULL )
                    {
                        // error 처리

                        ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                        ulpERR_ABORT_COMP_C_Unknown_Structname_Error,
                                        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName );
                        gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                        COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
                    }
                    else
                    {
                        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink
                                = gUlpParseInfo.mStructPtr;
                    }
                }
                else
                {   // it's a point of struct
                    // do nothing
                }
            }

            if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_CHAR) ||
                 (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_VARCHAR) )
            {
                IDU_LIST_ITERATE(&gUlpProgOption.mNcharVarNameList, sIterator)
                {
                    sVarName = (SChar* )sIterator->mObj;
                    if ( idlOS::strcmp( sVarName, gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mName )
                         == 0 )
                    {
                        if ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_CHAR )
                        {
                            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_NCHAR;
                        }
                        else
                        {
                            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_NVARCHAR;
                        }
                    }
                }
            }

            /* BUG-28118 : system 헤더파일들도 파싱돼야함.                       *
             * 6th. problem : Nested structure 정의중 scope를 잘못 계산하는 문제 */
            // struct 필드를 add하려 한다면, mHostValInfo의 이전 index에 저장된 struct node pointer 를 이용해야함.
            sSymNode =
                  gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth - 1]->mStructLink
                  ->mChild->ulpSymAdd (
                                          gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]
                                      );

            if ( sSymNode != NULL )
            {
                if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_VARCHAR) ||
                     (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_NVARCHAR) )
                {
                    sVarcharListNode =
                        (iduListNode*)idlOS::malloc(ID_SIZEOF(iduListNode));
                    if (sVarcharListNode == NULL)
                    {
                        ulpSetErrorCode(&gUlpParseInfo.mErrorMgr, ulpERR_ABORT_Memory_Alloc_Error);
                        gUlpCOMPErrCode = ulpGetErrorSTATE(&gUlpParseInfo.mErrorMgr);
                        COMPerror(ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr));
                    }
                    else
                    {
                        IDU_LIST_INIT_OBJ(sVarcharListNode, &(sSymNode->mElement));
                        IDU_LIST_ADD_LAST(&(gUlpParseInfo.mVarcharVarList), sVarcharListNode);
                    }
                }
            }
        }
    }
    ;

struct_decl_list_begin
    : ','
    {
        // , 를 사용한 동일 type을 여러개 선언할 경우 필요한 초기화.
        gUlpParseInfo.mSaveId = ID_TRUE;
        if ( gUlpParseInfo.mHostValInfo4Typedef != NULL )
        {
            if ( gUlpParseInfo.mHostValInfo4Typedef->mArraySize[0] != '\0' )
            {
                idlOS::strncpy( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize,
                                gUlpParseInfo.mHostValInfo4Typedef->mArraySize,
                                MAX_NUMBER_LEN - 1 );
            }
            else
            {
                gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize[0]   = '\0';
            }

            if ( gUlpParseInfo.mHostValInfo4Typedef->mArraySize2[0] != '\0' )
            {
                idlOS::strncpy( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize2,
                                gUlpParseInfo.mHostValInfo4Typedef->mArraySize2,
                                MAX_NUMBER_LEN - 1 );
            }
            else
            {
                gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize2[0]  = '\0';
            }

            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mPointer
                    = gUlpParseInfo.mHostValInfo4Typedef->mPointer;
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsarray
                    = gUlpParseInfo.mHostValInfo4Typedef->mIsarray;
        }
        else
        {
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize[0]   = '\0';
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize2[0]  = '\0';
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mPointer        = 0;
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsarray        = ID_FALSE;
        }
    }
    ;

struct_declarator
    : declarator
    | ':' constant_expr
    | declarator ':' constant_expr
    ;

enum_specifier
    : C_ENUM '{' enumerator_list '}'
    | C_ENUM identifier '{' enumerator_list '}'
    | C_ENUM identifier
    ;

enumerator_list
    : enumerator
    | enumerator_list ',' enumerator
    ;

enumerator
    : identifier
    | identifier '=' constant_expr
    ;

declarator
    : declarator2
    {
        gUlpParseInfo.mConstantExprStr[0] = '\0';
        gUlpParseInfo.mArrDepth = 0;
    }
    | pointer declarator2
    {
        gUlpParseInfo.mConstantExprStr[0] = '\0';
        gUlpParseInfo.mArrDepth = 0;
    }
    ;

declarator2
    : identifier
    | '(' declarator ')'
    | declarator2 '[' ']'
    {
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mPointer ++;
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsarray = ID_TRUE;
        if( gUlpParseInfo.mArrDepth == 0 )
        {
            if ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize[0] == '\0' )
            {
                // do nothing
            }
            else
            {
                idlOS::strncpy( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize2,
                                gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize,
                                MAX_NUMBER_LEN - 1 );
            }
        }
        else if ( gUlpParseInfo.mArrDepth == 1 )
        {
            // 2차 배열까지만 처리함.
            gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize2[0] = '\0';
        }
        else
        {
            // 2차 배열까지만 처리함.
            // ignore
        }

        gUlpParseInfo.mArrDepth++;
    }
    | declarator2 arr_decl_begin constant_expr ']'
    {
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mPointer ++;
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsarray = ID_TRUE;

        if( gUlpParseInfo.mArrDepth == 0 )
        {
            if ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize[0] == '\0' )
            {
                // 1차 배열의 expr
                idlOS::strncpy( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize,
                                gUlpParseInfo.mConstantExprStr,
                                MAX_NUMBER_LEN - 1 );
            }
            else
            {
                idlOS::strncpy( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize2,
                                gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize,
                                MAX_NUMBER_LEN - 1 );
                idlOS::strncpy( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize,
                                gUlpParseInfo.mConstantExprStr,
                                MAX_NUMBER_LEN - 1 );
            }

        }
        else if ( gUlpParseInfo.mArrDepth == 1 )
        {
            if ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize2[0] == '\0' )
            {
                // 2차 배열의 expr
                idlOS::strncpy( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mArraySize2,
                                gUlpParseInfo.mConstantExprStr,
                                MAX_NUMBER_LEN - 1 );
            }
            else
            {
                // do nothing
            }
        }

        gUlpParseInfo.mArrExpr = ID_FALSE;
        gUlpParseInfo.mArrDepth++;
    }
    | declarator2 '(' ')'
    | declarator2 func_decl_begin parameter_type_list ')'
    {
        gUlpCurDepth--;
    }
    | declarator2 '(' parameter_identifier_list ')'
    ;

arr_decl_begin
    : '['
    {
        // array [ expr ] => expr 의 시작이라는 것을 알림. expr을 저장하기 위함.
        // 물론 expr 문법 체크도 함.
        gUlpParseInfo.mConstantExprStr[0] = '\0';
        gUlpParseInfo.mArrExpr = ID_TRUE;
    }
    ;

func_decl_begin
    : '('
    {
        gUlpCurDepth++;
        gUlpParseInfo.mFuncDecl = ID_TRUE;
    }
    ;

pointer
    : '*'
    {
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mPointer ++;
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mAlloc = ID_TRUE;
    }
    | '*' type_specifier_list
    {
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mPointer ++;
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mAlloc = ID_TRUE;
    }
    | '*' pointer
    {
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mPointer ++;
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mAlloc = ID_TRUE;
    }
    | '*' type_specifier_list pointer
    {
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mPointer ++;
        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mAlloc = ID_TRUE;
    }
    ;

type_specifier_list
    : type_specifier
    | type_specifier_list type_specifier
    ;

parameter_identifier_list
    : identifier_list
    | identifier_list ',' C_ELIPSIS
    ;

identifier_list
    : identifier
    | identifier_list ',' identifier
    ;

parameter_type_list
    : parameter_list
    | parameter_list ',' C_ELIPSIS
    ;

parameter_list
    : parameter_declaration
    | parameter_list ',' parameter_declaration
    ;

/* 함수 인자 선언부 문법 */
parameter_declaration
    : type_specifier_list declarator
    {
        SChar *sVarName;
        iduListNode *sIterator = NULL;

        /* BUG-28118 : system 헤더파일들도 파싱돼야함.            *
         * 5th. problem : 정의되지 않은 구조체 포인터 변수 선언안됨. */
        if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mIsstruct  == ID_TRUE) &&
             (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName[0] != '\0') &&
             (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink == NULL) )
        {   // struct 변수 선언의 경우, type check rigidly.

            gUlpParseInfo.mStructPtr = gUlpStructT.ulpStructLookupAll(
                                gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName,
                                gUlpCurDepth );
            if ( gUlpParseInfo.mStructPtr == NULL )
            {
                // error 처리

                ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                 ulpERR_ABORT_COMP_C_Unknown_Structname_Error,
                                 gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructName );
                gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
            }
            else
            {
                gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mStructLink
                        = gUlpParseInfo.mStructPtr;
            }

        }

        if ( (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_CHAR) ||
             (gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_VARCHAR) )
        {
            IDU_LIST_ITERATE(&gUlpProgOption.mNcharVarNameList, sIterator)
            {
                sVarName = (SChar* )sIterator->mObj;
                if ( idlOS::strcmp( sVarName, gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mName )
                     == 0 )
                {
                    if ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType == H_CHAR )
                    {
                        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_NCHAR;
                    }
                    else
                    {
                        gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mType = H_NVARCHAR;
                    }
                }
            }
        }

        if( gUlpScopeT.ulpSAdd ( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]
                                 , gUlpCurDepth )
            == NULL )
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_C_Add_Symbol_Error,
                             gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mName );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }

        gUlpParseInfo.ulpInitHostInfo();
    }
    | type_name
    ;

type_name
    : type_specifier_list
    | type_specifier_list abstract_declarator
    ;

abstract_declarator
    : pointer
    | abstract_declarator2
    | pointer abstract_declarator2
    ;

abstract_declarator2
    : '(' abstract_declarator ')'
    | '[' ']'
    | '[' constant_expr ']'
    | abstract_declarator2 '[' ']'
    | abstract_declarator2 '[' constant_expr ']'
    | '(' ')'
    | '(' parameter_type_list ')'
    | abstract_declarator2 '(' ')'
    | abstract_declarator2 '(' parameter_type_list ')'
    ;

initializer
    : assignment_expr
    | '{' initializer_list '}'
    | '{' initializer_list ',' '}'
    ;

initializer_list
    : initializer
    | initializer_list ',' initializer
    ;

statement
    : labeled_statement
    | compound_statement
    | expression_statement
    | selection_statement
    | iteration_statement
    | jump_statement
    ;

labeled_statement
    : identifier ':' statement
    /* BUG-30637 : The syntax of labeled statement is wrong */
    | identifier ':' Emsql_grammar
    | C_CASE constant_expr ':' statement
    | C_DEFAULT ':' statement
    | C_CASE constant_expr ':' Emsql_grammar
    | C_DEFAULT ':' Emsql_grammar
    ;

compound_statement
    : compound_statement_begin '}'
    | compound_statement_begin super_compound_stmt_list '}'
    ;

super_compound_stmt
    // declaration 문법에서 마지막에 변수정보 초기화 해준다.
    : declaration
    | statement
    {
        /* BUG-29081 : 변수 선언부가 statement 중간에 들어오면 파싱 에러발생. */
        // statement 를 파싱한뒤 변수 type정보를 저장해두고 있는 자료구조를 초기화해줘야한다.
        // 저장 자체를 안하는게 이상적이나 type처리 문법을 선언부와 함께 공유하므로 어쩔수 없다.
        gUlpParseInfo.ulpInitHostInfo();
    }
    | Emsql_grammar
    ;

super_compound_stmt_list
    : super_compound_stmt
    | super_compound_stmt_list super_compound_stmt
    ;

compound_statement_begin
    : '{'
    {
        if( gUlpParseInfo.mFuncDecl == ID_TRUE )
        {
            gUlpParseInfo.mFuncDecl = ID_FALSE;
        }
    }
    ;

declaration_list
    : declaration
    | declaration_list declaration
    ;

expression_statement
    : ';'
    | expr ';'
    ;

selection_statement
    : C_IF '(' expr ')' statement
    | C_IF '(' expr ')' statement C_ELSE statement
    | C_SWITCH '(' expr ')' statement
    ;

iteration_statement
    : C_WHILE '(' expr ')' statement
    | C_DO statement C_WHILE '(' expr ')' ';'
    | C_FOR '(' ';' ';' ')' statement
    | C_FOR '(' ';' ';' expr ')' statement
    | C_FOR '(' ';' expr ';' ')' statement
    | C_FOR '(' ';' expr ';' expr ')' statement
    | C_FOR '(' expr ';' ';' ')' statement
    | C_FOR '(' expr ';' ';' expr ')' statement
    | C_FOR '(' expr ';' expr ';' ')' statement
    | C_FOR '(' expr ';' expr ';' expr ')' statement
    ;

jump_statement
    : C_GOTO identifier ';'
    | C_CONTINUE ';'
    | C_BREAK ';'
    | C_RETURN ';'
    | C_RETURN expr ';'
    ;

function_definition
    : declarator function_body
    | declaration_specifiers declarator function_body
    ;

function_body
    : compound_statement
    | declaration_list compound_statement
    ;

identifier
    : C_IDENTIFIER
    {
        if( idlOS::strlen($<strval>1) >= MAX_HOSTVAR_NAME_SIZE )
        {
            //error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_C_Exceed_Max_Id_Length_Error,
                             $<strval>1 );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
        else
        {
            if( gUlpParseInfo.mSaveId == ID_TRUE )
            {
                idlOS::strcpy( gUlpParseInfo.mHostValInfo[gUlpParseInfo.mStructDeclDepth]->mName,
                               $<strval>1 );
                gUlpParseInfo.mSaveId = ID_FALSE;
            }
        }
    }
    ;

string_literal
    : C_STRING_LITERAL
    | C_STRING_LITERAL string_literal
    ;

//=====================================================//
//                                                     //
//                    MACRO grammar                    //
//                                                     //
//=====================================================//

Macro_grammar
        : Macro_include
        {
            /* BUG-28061 : preprocessing을마치면 marco table을 초기화하고, *
             *             ulpComp 에서 재구축한다.                       */
            gUlpCOMPStartCond = gUlpCOMPPrevCond;
        }
        | Macro_define
        {
            gUlpCOMPStartCond = gUlpCOMPPrevCond;
        }
        | Macro_undef
        {
            gUlpCOMPStartCond = gUlpCOMPPrevCond;
        }
        | Macro_ifdef
        {
            /* macro 조건문의 경우 참이면 C상태, 거짓이면 MACRO_IFSKIP 상태로
             * 전이 된다. */
        }
        | Macro_ifndef
        {
            /* macro 조건문의 경우 참이면 C상태, 거짓이면 MACRO_IFSKIP 상태로
             * 전이 된다. */
        }
        | Macro_if
        {
            /* macro 조건문의 경우 참이면 C상태, 거짓이면 MACRO_IFSKIP 상태로
             * 전이 된다. */
        }
        | Macro_elif
        {
            /* macro 조건문의 경우 참이면 C상태, 거짓이면 MACRO_IFSKIP 상태로
             * 전이 된다. */
        }
        | Macro_else
        {
            /* macro 조건문의 경우 참이면 C상태, 거짓이면 MACRO_IFSKIP 상태로
             * 전이 된다. */
        }
        | Macro_endif
        ;

Macro_include
        : M_INCLUDE M_LBRAC M_FILENAME M_RBRAC
        {
            /* #include <...> */

            // 1. check exist header file in include paths
            if ( gUlpProgOption.ulpLookupHeader( $<strval>3, ID_TRUE )
                 == IDE_FAILURE )
            {
                // do nothing
            }
            else
            {

                // 현재 #include 처리다.
                gDontPrint2file = ID_TRUE;
                /* BUG-27683 : iostream 사용 제거 */
                // 2. flex 버퍼 상태 저장.
                ulpCOMPSaveBufferState();
                // 3. doCOMPparse()를 재호출한다.
                doCOMPparse( gUlpProgOption.ulpGetIncList() );
                // 전에 #inlcude 처리중이었나? 확인함
                gDontPrint2file = gUlpProgOption.ulpIsHeaderCInclude();

                // 4. precompiler를 실행한 directory를 current path로 재setting
                idlOS::strcpy( gUlpProgOption.mCurrentPath, gUlpProgOption.mStartPath );
            }

        }
        | M_INCLUDE M_DQUOTE M_FILENAME M_DQUOTE
        {

            // 1. check exist header file in include paths
            if ( gUlpProgOption.ulpLookupHeader( $<strval>3, ID_TRUE )
                 == IDE_FAILURE )
            {
                // do nothing
            }
            else
            {

                // 현재 #include 처리다.
                gDontPrint2file = ID_TRUE;
                /* BUG-27683 : iostream 사용 제거 */
                // 2. flex 버퍼 상태 저장.
                ulpCOMPSaveBufferState();
                // 3. doCOMPparse()를 재호출한다.
                doCOMPparse( gUlpProgOption.ulpGetIncList() );
                // 전에 #inlcude 처리중이었나? 확인함
                gDontPrint2file = gUlpProgOption.ulpIsHeaderCInclude();

                // 4. precompiler를 실행한 directory를 current path로 재setting
                idlOS::strcpy( gUlpProgOption.mCurrentPath, gUlpProgOption.mStartPath );
            }

        }
        ;

Macro_define
        /* BUG-28061 : preprocessing을마치면 marco table을 초기화하고, *
         *             ulpComp 에서 재구축한다.                       */
        : M_DEFINE M_IDENTIFIER
        {
            SChar sTmpDEFtext[ MAX_MACRO_DEFINE_CONTENT_LEN ];
            idlOS::memset(sTmpDEFtext,0,MAX_MACRO_DEFINE_CONTENT_LEN);

            ulpCOMPEraseBN4MacroText( sTmpDEFtext , ID_FALSE );

            if ( sTmpDEFtext[0] == '\0' )
            {
                // macro symbol table에 추가함.
                if( gUlpMacroT.ulpMDefine( $<strval>2, NULL, ID_FALSE ) == IDE_FAILURE )
                {

                    ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                     ulpERR_ABORT_Memory_Alloc_Error );
                    gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                    COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
                }
            }
            else
            {
                // macro symbol table에 추가함.
                if( gUlpMacroT.ulpMDefine( $<strval>2, sTmpDEFtext, ID_FALSE ) == IDE_FAILURE )
                {

                    ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                     ulpERR_ABORT_Memory_Alloc_Error );
                    gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                    COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
                }
            }

        }
        | M_DEFINE M_FUNCTION
        {
            // function macro의경우 인자 정보는 따로 저장되지 않는다.
            SChar sTmpDEFtext[ MAX_MACRO_DEFINE_CONTENT_LEN ];

            idlOS::memset(sTmpDEFtext,0,MAX_MACRO_DEFINE_CONTENT_LEN);
            ulpCOMPEraseBN4MacroText( sTmpDEFtext , ID_FALSE );

            // #define A() {...} 이면, macro id는 A이다.
            if ( sTmpDEFtext[0] == '\0' )
            {
                // macro symbol table에 추가함.
                if ( gUlpMacroT.ulpMDefine( $<strval>2, NULL, ID_TRUE ) == IDE_FAILURE )
                {

                    ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                     ulpERR_ABORT_Memory_Alloc_Error );
                    gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                    COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
                }
            }
            else
            {
                // macro symbol table에 추가함.
                if ( gUlpMacroT.ulpMDefine( $<strval>2, sTmpDEFtext, ID_TRUE ) == IDE_FAILURE )
                {

                    ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                     ulpERR_ABORT_Memory_Alloc_Error );
                    gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                    COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
                }
            }

        }
        ;

Macro_undef
        : M_UNDEF M_IDENTIFIER
        {
            // $<strval>2 를 macro symbol table에서 삭제 한다.
            gUlpMacroT.ulpMUndef( $<strval>2 );
        }
        ;

Macro_if
        : M_IF
        {
            SInt  sVal;
            SChar sTmpExpBuf[MAX_MACRO_IF_EXPR_LEN];
            /* BUG-32413 APRE memory allocation failure should be fixed */
            idlOS::memset(sTmpExpBuf, 0, MAX_MACRO_IF_EXPR_LEN);

            switch( gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpPrevIfStatus() )
            {
                // 이전 상태가 PP_IF_IGNORE 이면 계속 무시함.
                case PP_IF_IGNORE :
                    // 단순히 token만 소모하는 역할이다. PPIFparse 호출하지 않는다.
                    ulpCOMPEraseBN4MacroText( sTmpExpBuf , ID_TRUE );
                    gUlpCOMPStartCond = CP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_IF, PP_IF_IGNORE );
                    break;
                // 이전 상태가 PP_IF_TRUE 이면 이번 #if <expr>파싱하여 값을 확인해봐야함.
                case PP_IF_TRUE :
                    // #if expression 을 복사해온다.
                    ulpCOMPEraseBN4MacroText( sTmpExpBuf , ID_TRUE );
                    gUlpPPIFbufptr = sTmpExpBuf;
                    gUlpPPIFbuflim = sTmpExpBuf + idlOS::strlen(sTmpExpBuf);

                    if ( PPIFparse( sTmpExpBuf, &sVal ) != 0 )
                    {
                        //error 처리

                        ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                         ulpERR_ABORT_COMP_IF_Macro_Syntax_Error );
                        gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                        COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
                    }
                    /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
                    * 전이 된다. */
                    if ( sVal != 0 )
                    {
                        // true
                        gUlpCOMPStartCond = gUlpCOMPPrevCond;
                        // if stack manager 에 결과 정보 push
                        gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_IF, PP_IF_TRUE );
                    }
                    else
                    {
                        // false
                        gUlpCOMPStartCond = CP_ST_MACRO_IF_SKIP;
                        // if stack manager 에 결과 정보 push
                        gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_IF, PP_IF_FALSE );
                    }
                    break;
                // 이전 상태가 PP_IF_FALSE 이면 무시함.
                case PP_IF_FALSE :
                    // 단순히 token만 소모하는 역할이다. PPIFparse 호출하지 않는다.
                    ulpCOMPEraseBN4MacroText( sTmpExpBuf , ID_TRUE );
                    gUlpCOMPStartCond = CP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_IF, PP_IF_IGNORE );
                    break;

                default:
                    IDE_ASSERT(0);
            }
        }
        ;

Macro_elif
        : M_ELIF
        {
            SInt  sVal;
            SChar sTmpExpBuf[MAX_MACRO_IF_EXPR_LEN];
            /* BUG-32413 APRE memory allocation failure should be fixed */
            idlOS::memset(sTmpExpBuf, 0, MAX_MACRO_IF_EXPR_LEN);

            // #elif 순서 문법 검사.
            if ( gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfCheckGrammar( PP_ELIF )
                 == ID_FALSE )
            {
                //error 처리

                ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                 ulpERR_ABORT_COMP_ELIF_Macro_Sequence_Error );
                gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
            }

            switch( gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpPrevIfStatus() )
            {
                case PP_IF_IGNORE :
                    // 단순히 token만 소모하는 역할이다. PPIFparse 호출하지 않는다.
                    ulpCOMPEraseBN4MacroText( sTmpExpBuf , ID_TRUE );
                    gUlpCOMPStartCond = CP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_ELIF, PP_IF_IGNORE );
                    break;

                case PP_IF_TRUE :
                    // 단순히 token만 소모하는 역할이다. PPIFparse 호출하지 않는다.
                    ulpCOMPEraseBN4MacroText( sTmpExpBuf , ID_TRUE );
                    gUlpCOMPStartCond = CP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_ELIF, PP_IF_IGNORE );
                    break;

                case PP_IF_FALSE :
                    ulpCOMPEraseBN4MacroText( sTmpExpBuf , ID_TRUE );

                    gUlpPPIFbufptr = sTmpExpBuf;
                    gUlpPPIFbuflim = sTmpExpBuf + idlOS::strlen(sTmpExpBuf);

                    if ( PPIFparse( sTmpExpBuf, &sVal ) != 0 )
                    {
                        //error 처리

                        ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                         ulpERR_ABORT_COMP_ELIF_Macro_Syntax_Error );
                        gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                        COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
                    }

                    /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
                     * 전이 된다. */
                    if ( sVal != 0 )
                    {
                        gUlpCOMPStartCond = gUlpCOMPPrevCond;
                        // if stack manager 에 결과 정보 push
                        gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_ELIF, PP_IF_TRUE );
                    }
                    else
                    {
                        gUlpCOMPStartCond = CP_ST_MACRO_IF_SKIP;
                        // if stack manager 에 결과 정보 push
                        gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_ELIF, PP_IF_FALSE );
                    }
                    break;

                default:
                    IDE_ASSERT(0);
            }
        }
        ;

Macro_ifdef
        : M_IFDEF M_IDENTIFIER
        {
            switch( gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpPrevIfStatus() )
            {
                case PP_IF_IGNORE :
                    gUlpCOMPStartCond = CP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_IFDEF, PP_IF_IGNORE );
                    break;

                case PP_IF_TRUE :
                    // $<strval>2 를 macro symbol table에 존재하는지 확인한다.
                    if ( gUlpMacroT.ulpMLookup($<strval>2) != NULL )
                    {
                        // 존재한다
                        gUlpCOMPStartCond = gUlpCOMPPrevCond;
                        // if stack manager 에 결과 정보 push
                        gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_IFDEF, PP_IF_TRUE );
                    }
                    else
                    {
                        // 존재안한다
                        gUlpCOMPStartCond = CP_ST_MACRO_IF_SKIP;
                        // if stack manager 에 결과 정보 push
                        gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_IFDEF, PP_IF_FALSE );
                    }
                    break;

                case PP_IF_FALSE :
                    gUlpCOMPStartCond = CP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_IFDEF, PP_IF_IGNORE );
                    break;

                default:
                    IDE_ASSERT(0);
            }
        }
        ;

Macro_ifndef
        : M_IFNDEF M_IDENTIFIER
        {
            switch( gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpPrevIfStatus() )
            {
                case PP_IF_IGNORE :
                    gUlpCOMPStartCond = CP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_IFNDEF, PP_IF_IGNORE );
                    break;

                case PP_IF_TRUE :
                    // $<strval>2 를 macro symbol table에 존재하는지 확인한다.
                    if ( gUlpMacroT.ulpMLookup($<strval>2) != NULL )
                    {
                        // 존재한다
                        gUlpCOMPStartCond = CP_ST_MACRO_IF_SKIP;
                        // if stack manager 에 결과 정보 push
                        gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_IFNDEF, PP_IF_FALSE );
                    }
                    else
                    {
                        // 존재안한다
                        gUlpCOMPStartCond = gUlpCOMPPrevCond;
                        // if stack manager 에 결과 정보 push
                        gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_IFNDEF, PP_IF_TRUE );
                    }
                    break;

                case PP_IF_FALSE :
                    gUlpCOMPStartCond = CP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_IFNDEF, PP_IF_IGNORE );
                    break;

                default:
                    IDE_ASSERT(0);
            }
        }
        ;

Macro_else
        : M_ELSE
        {
            // #else 순서 문법 검사.
            if ( gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfCheckGrammar( PP_ELSE )
                 == ID_FALSE )
            {
                // error 처리

                ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                 ulpERR_ABORT_COMP_ELSE_Macro_Sequence_Error );
                gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
            }

            switch( gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpPrevIfStatus() )
            {
                case PP_IF_IGNORE :
                case PP_IF_TRUE :
                    gUlpCOMPStartCond = CP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_ELSE, PP_IF_IGNORE );
                    break;

                case PP_IF_FALSE :
                    gUlpCOMPStartCond = gUlpCOMPPrevCond;
                    // if stack manager 에 결과 정보 push
                    gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpush( PP_ELSE, PP_IF_TRUE );
                    break;

                default:
                    IDE_ASSERT(0);
            }
        }
        ;

Macro_endif
        : M_ENDIF
        {
            // #endif 순서 문법 검사.
            if ( gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfCheckGrammar( PP_ENDIF )
                 == ID_FALSE )
            {
                // error 처리

                ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                 ulpERR_ABORT_COMP_ENDIF_Macro_Sequence_Error );
                gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
            }
            // if stack 을 이전 조건문 까지 pop 한다.
            gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfpop4endif();

            /* BUG-27961 : preprocessor의 중첩 #if처리시 #endif 다음소스 무조건 출력하는 버그  */
            if( gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfneedSkip4Endif() == ID_TRUE )
            {
                // 출력 하지마라.
                gUlpCOMPStartCond = CP_ST_MACRO_IF_SKIP;
            }
            else
            {
                // 출력 해라.
                gUlpCOMPStartCond = gUlpCOMPPrevCond;
            }
        }
        ;


//=====================================================//
//                                                     //
//               Embedded SQL grammar                  //
//                                                     //
//=====================================================//

Emsql_grammar
    : EM_SQLSTART at_clause sql_stmt TS_SEMICOLON
    {
        // 내장구문을 comment형태로 쓴다.
        gUlpCodeGen.ulpGenComment( gUlpCodeGen.ulpGetQueryBuf() );
        // 내장구문 관련 코드생성한다.
        if( gUlpStmttype > S_IGNORE )
        {
            gUlpCodeGen.ulpGenEmSQLFlush( gUlpStmttype, gUlpIsPrintStmt );
        }
        // ulpCodeGen class 의 query buffer 를 초기화한다.
        gUlpCodeGen.ulpGenInitQBuff();
        // ulpCodeGen class 의 mEmSQLInfo 를 초기화한다.
        gUlpCodeGen.ulpClearEmSQLInfo();
        // lexer의 상태를 embedded sql 구문을 파싱하기 이전상태로 되돌린다.
        gUlpCOMPStartCond = gUlpCOMPPrevCond;
        // init variables
        gUlpIsPrintStmt = ID_TRUE;
        gUlpStmttype    = S_UNKNOWN;
    }
    | EM_SQLSTART at_clause esql_stmt TS_SEMICOLON
    {

        gUlpCodeGen.ulpGenComment( gUlpCodeGen.ulpGetQueryBuf() );
        if( gUlpStmttype > S_IGNORE )
        {
            gUlpCodeGen.ulpGenEmSQLFlush( gUlpStmttype, gUlpIsPrintStmt );
        }
        gUlpCodeGen.ulpGenInitQBuff();
        gUlpCodeGen.ulpClearEmSQLInfo();
        gUlpCOMPStartCond = gUlpCOMPPrevCond;
        // init variables
        gUlpIsPrintStmt = ID_TRUE;
        gUlpStmttype    = S_UNKNOWN;
    }
    | EM_SQLSTART at_clause dsql_stmt TS_SEMICOLON
    {

        gUlpCodeGen.ulpGenComment( gUlpCodeGen.ulpGetQueryBuf() );
        if( gUlpStmttype > S_IGNORE )
        {
            gUlpCodeGen.ulpGenEmSQLFlush( gUlpStmttype, gUlpIsPrintStmt );
        }
        gUlpCodeGen.ulpGenInitQBuff();
        gUlpCodeGen.ulpClearEmSQLInfo();
        gUlpCOMPStartCond = gUlpCOMPPrevCond;
        // init variables
        gUlpIsPrintStmt = ID_TRUE;
        gUlpStmttype    = S_UNKNOWN;
    }
    | EM_SQLSTART at_clause proc_stmt TR_ENDEXEC TS_SEMICOLON
    {

        gUlpCodeGen.ulpGenComment( gUlpCodeGen.ulpGetQueryBuf() );
        if( gUlpStmttype > S_IGNORE )
        {
            gUlpCodeGen.ulpGenEmSQLFlush( gUlpStmttype, gUlpIsPrintStmt );
        }
        gUlpCodeGen.ulpGenInitQBuff();
        gUlpCodeGen.ulpClearEmSQLInfo();
        gUlpCOMPStartCond = gUlpCOMPPrevCond;
        // init variables
        gUlpIsPrintStmt = ID_TRUE;
        gUlpStmttype    = S_UNKNOWN;
        /* BUG-46824 추가된 변수 초기화 */
        gUlpProcObjCount = 0;
        gUlpPSMObjName = NULL;
    }
    | EM_SQLSTART at_clause etc_stmt TS_SEMICOLON
    {

        gUlpCodeGen.ulpGenComment( gUlpCodeGen.ulpGetQueryBuf() );
        if( gUlpStmttype > S_IGNORE )
        {
            gUlpCodeGen.ulpGenEmSQLFlush( gUlpStmttype, gUlpIsPrintStmt );
        }
        gUlpCodeGen.ulpGenInitQBuff();
        gUlpCodeGen.ulpClearEmSQLInfo();
        gUlpCOMPStartCond = gUlpCOMPPrevCond;
        gUlpIsPrintStmt = ID_TRUE;
        gUlpStmttype    = S_UNKNOWN;
    }
    | EM_SQLSTART option_stmt TS_SEMICOLON
    {
        gUlpCodeGen.ulpGenInitQBuff();
        gUlpCodeGen.ulpClearEmSQLInfo();
        gUlpCOMPStartCond = gUlpCOMPPrevCond;
    }
    | EM_SQLSTART exception_stmt TS_SEMICOLON
    {
        // whenever 구문을 comment로 쓴다.
        gUlpCodeGen.ulpGenComment( gUlpCodeGen.ulpGetQueryBuf() );

        gUlpCodeGen.ulpGenInitQBuff();
        gUlpCodeGen.ulpClearEmSQLInfo();
        gUlpCOMPStartCond = gUlpCOMPPrevCond;
    }
    | EM_SQLSTART threads_stmt TS_SEMICOLON
    {
        gUlpCodeGen.ulpGenComment( gUlpCodeGen.ulpGetQueryBuf() );
        if( gUlpStmttype > S_IGNORE )
        {
            gUlpCodeGen.ulpGenEmSQLFlush( gUlpStmttype, gUlpIsPrintStmt );
        }
        gUlpCodeGen.ulpGenInitQBuff();
        gUlpCodeGen.ulpClearEmSQLInfo();
        gUlpCOMPStartCond = gUlpCOMPPrevCond;
        gUlpIsPrintStmt = ID_TRUE;
        gUlpStmttype    = S_UNKNOWN;
    }

    | EM_SQLSTART sharedptr_stmt
    {
        gUlpCodeGen.ulpGenComment( gUlpCodeGen.ulpGetQueryBuf() );
        gUlpCodeGen.ulpGenInitQBuff();
        gUlpCodeGen.ulpClearEmSQLInfo();
        gUlpIsPrintStmt = ID_TRUE;
        gUlpStmttype    = S_UNKNOWN;
    }

    | EM_SQLSTART getdiag_stmt TS_SEMICOLON
    {
        /* TASK-7218 Handling Multiple Errors */
        gUlpCodeGen.ulpGenComment( gUlpCodeGen.ulpGetQueryBuf() );
        if( gUlpStmttype > S_IGNORE )
        {
            gUlpCodeGen.ulpGenEmSQLFlush( gUlpStmttype, gUlpIsPrintStmt );
        }
        gUlpCodeGen.ulpGenInitQBuff();
        gUlpCodeGen.ulpClearEmSQLInfo();
        gUlpCOMPStartCond = gUlpCOMPPrevCond;
        gUlpIsPrintStmt = ID_TRUE;
        gUlpStmttype    = S_UNKNOWN;
    }
    ;

at_clause
    : /* empty */
    {
        // do nothing
    }
    | TR_AT SES_V_IDENTIFIER
    {
        if ( idlOS::strlen($<strval>2) > MAX_CONN_NAME_LEN )
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Exceed_Max_Connname_Error,
                             $<strval>2 );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
        else
        {
            gUlpCodeGen.ulpGenEmSQL( GEN_CONNNAME, (void *) $<strval>2 );
            gUlpCodeGen.ulpGenRemoveQueryToken( $<strval>2 );
            gUlpCodeGen.ulpGenRemoveQueryToken( $<strval>1 );
        }

    }
    | TR_AT SES_V_HOSTVARIABLE
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_CONNNAME, (void *) $<strval>2 );
        gUlpCodeGen.ulpGenRemoveQueryToken( $<strval>2 );
        gUlpCodeGen.ulpGenRemoveQueryToken( $<strval>1 );
    }
    ;

    //////////////////////////////////////////////////////////////////////
    ////
    ////  esql_stmt
    ////   : embedded sql 구문 문법.
    ////    ( 몇몇 dynamic method관련 문법들은 dsql_stmt에 정의됨. )
    ////
    //////////////////////////////////////////////////////////////////////

esql_stmt
  /*: alloc_cursor_stmt
  */: declare_cursor_stmt
    {
        gUlpStmttype = S_DeclareCur;
    }
    | declare_statement_stmt
    {
        gUlpStmttype = S_DeclareStmt;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | open_cursor_stmt
    {
        gUlpStmttype = S_Open;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | fetch_cursor_stmt
    {
        gUlpStmttype = S_Fetch;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | close_cursor_stmt
    {
        gUlpIsPrintStmt = ID_FALSE;
    }
    | autocommit_stmt
    {
        gUlpStmttype = S_AutoCommit;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | conn_stmt
    {
        gUlpStmttype = S_Connect;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | disconn_stmt
    {
        gUlpStmttype = S_Disconnect;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | free_lob_loc_stmt
    {
        gUlpStmttype = S_FreeLob;
        gUlpIsPrintStmt = ID_FALSE;
    }
    ;

    /////////////////////////////////////
    ////  esql_stmt : declare_cursor_stmt
    /////////////////////////////////////

/*
alloc_cursor_stmt
  : TR_ALLOCATE SES_V_IDENTIFIER cursor_sensitivity_opt cursor_scroll_opt
    TR_CURSOR cursor_hold_opt cursor_return_opt TR_FOR SES_V_IDENTIFIER
  ;
*/
declare_cursor_stmt
    : begin_declare select_or_with_select_statement_4emsql cursor_method_opt
    {
        /* BUG-40939 */
        SChar* sTmpQueryBuf;
        UInt   sCurNameLength = 0;

        sCurNameLength = idlOS::strlen( gUlpCodeGen.ulpGenGetEmSQLInfo()->mCurName );
        sTmpQueryBuf = idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(), 
                                      gUlpCodeGen.ulpGenGetEmSQLInfo()->mCurName ) + sCurNameLength;
        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR, 
                                 idlOS::strstr(sTmpQueryBuf, $<strval>2) );
        gUlpIsPrintStmt = ID_TRUE;
    }
    | begin_declare SES_V_IDENTIFIER cursor_method_opt
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_STMTNAME, (void *) $<strval>2 );
        gUlpIsPrintStmt = ID_FALSE;
    }
    ;

begin_declare
    : TR_DECLARE SES_V_IDENTIFIER cursor_sensitivity_opt cursor_scroll_opt
    TR_CURSOR cursor_hold_opt TR_FOR
    {
        if ( idlOS::strlen($<strval>2) >= MAX_CUR_NAME_LEN)
        {

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Exceed_Max_Cursorname_Error,
                             $<strval>2 );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }

        gUlpCodeGen.ulpGenEmSQL( GEN_CURNAME, (void *) $<strval>2 );
    }
    ;

/* BUG-41471 Maintain one APRE source code for HDB and HDB-DA */
cursor_sensitivity_opt
  : /* empty */
  {
        UInt sValue = SQL_INSENSITIVE;
        gUlpCodeGen.ulpGenEmSQL( GEN_CURSORSENSITIVITY, (void *)&sValue );
  }
  | SES_V_SENSITIVE
  {
        UInt sValue = SQL_SENSITIVE;
        gUlpCodeGen.ulpGenEmSQL( GEN_CURSORSENSITIVITY, (void *)&sValue );
  }
  | SES_V_INSENSITIVE
  {
        UInt sValue = SQL_INSENSITIVE;
        gUlpCodeGen.ulpGenEmSQL( GEN_CURSORSENSITIVITY, (void *)&sValue );
  }
  | SES_V_ASENSITIVE
  {
        UInt sValue = SQL_INSENSITIVE;
        gUlpCodeGen.ulpGenEmSQL( GEN_CURSORSENSITIVITY, (void *)&sValue );
  }
  ;

/* BUG-41471 Maintain one APRE source code for HDB and HDB-DA */
cursor_scroll_opt
    : /* empty */
    {
        UInt sValue = SQL_NONSCROLLABLE;
        gUlpCodeGen.ulpGenEmSQL( GEN_CURSORSCROLLABLE, (void *) &sValue );
}
    | SES_V_SCROLL
    {
        UInt sValue = SQL_SCROLLABLE;
        gUlpCodeGen.ulpGenEmSQL( GEN_CURSORSCROLLABLE, (void *) &sValue );
        gUlpCodeGen.ulpGenEmSQL( GEN_SCROLLCUR, (void *) "1" );
    }
    ;

/* BUG-41471 Maintain one APRE source code for HDB and HDB-DA */
cursor_hold_opt
  : /* empty */
  {
      UInt sValue = SQL_CURSOR_HOLD_OFF;
      gUlpCodeGen.ulpGenEmSQL( GEN_CURSORWITHHOLD, (void *) &sValue );
  }
  | TR_WITH SES_V_HOLD
  {
      UInt sValue = SQL_CURSOR_HOLD_ON;
      gUlpCodeGen.ulpGenEmSQL( GEN_CURSORWITHHOLD, (void *) &sValue );
  }
  | TR_WITH TR_RETURN
  {
      ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                       ulpERR_ABORT_COMP_Not_Supported_WITH_RETURN_Error );
      gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
      COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
  }
  | TR_WITH SES_V_HOLD TR_WITH TR_RETURN
  {
      ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                       ulpERR_ABORT_COMP_Not_Supported_WITH_RETURN_Error );
      gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
      COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
  }
  ;

cursor_method_opt
  : /* empty */
  | TR_FOR TR_READ SES_V_ONLY
  {

      ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                       ulpERR_ABORT_COMP_Not_Supported_READ_ONLY_Error );
      gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
      COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
  }
  | TR_FOR TR_UPDATE cursor_update_list
  | TR_FOR TR_UPDATE SES_V_IDENTIFIER /*NOWAIT*/
  | TR_FOR TR_UPDATE TO_WAIT SES_V_INTEGER /*wait 0*/
  ;

cursor_update_list
  : /* empty */
  | TR_OF cursor_update_column_list
  ;

cursor_update_column_list
  : object_name
  | cursor_update_column_list TS_COMMA object_name
  ;

    /////////////////////////////////////
    ////  esql_stmt : declare_cursor_stmt
    /////////////////////////////////////

declare_statement_stmt
    : TR_DECLARE SES_V_IDENTIFIER TR_STATEMENT
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_STMTNAME, (void *) $<strval>2 );
    }
    ;

    /////////////////////////////////////
    ////  esql_stmt : open_cursor_stmt
    /////////////////////////////////////

open_cursor_stmt
    : TR_OPEN SES_V_IDENTIFIER
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_CURNAME, (void *) $<strval>2 );
    }
    | TR_OPEN SES_V_IDENTIFIER TR_USING host_var_list
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_CURNAME, (void *) $<strval>2 );
    }
    /* BUG-41010 Add dynamic binding feature on Apre */
    | TR_OPEN SES_V_IDENTIFIER TR_USING using_descriptor
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_CURNAME, (void *) $<strval>2 );
    }
    ;

    /////////////////////////////////////
    ////  esql_stmt : fetch_cursor_stmt
    /////////////////////////////////////

fetch_cursor_stmt
    : TR_FETCH fetch_orientation_from SES_V_IDENTIFIER TR_INTO out_host_var_list
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_CURNAME, (void *) $<strval>3 );
    }
    /* BUG-45448 parecompile시에 FECTH절에서 FOR구문 처리하도록 추가 */
    | for_clause TR_FETCH fetch_orientation_from SES_V_IDENTIFIER TR_INTO out_host_var_list
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_CURNAME, (void *) $<strval>4 );
    }
    /* BUG-41010 Add dynamic binding feature on Apre */
    | TR_FETCH fetch_orientation_from SES_V_IDENTIFIER TR_USING TR_DESCRIPTOR host_variable
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_CURNAME, (void *) $<strval>3 );
    }
    ;

fetch_orientation_from
    : /* empty */
    | TR_FROM
    | fetch_orientation
    | fetch_orientation TR_FROM
    ;

fetch_orientation
    : SES_V_FIRST
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_SCROLLCUR, (void *) "1" /*F_FIRST*/ );
    }
    | TR_PRIOR
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_SCROLLCUR, (void *) "2" /*F_PRIOR*/ );
    }
    | SES_V_NEXT
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_SCROLLCUR, (void *) "3" /*F_NEXT*/ );
    }
    | SES_V_LAST
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_SCROLLCUR, (void *) "4" /*F_LAST*/ );
    }
    | SES_V_CURRENT
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_SCROLLCUR, (void *) "5" /*F_CURRENT*/ );
    }
    | SES_V_RELATIVE fetch_integer
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_SCROLLCUR, (void *) "6" /*F_RELATIVE*/ );
    }
    | SES_V_ABSOLUTE fetch_integer
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_SCROLLCUR, (void *) "7" /*F_ABSOLUTE*/ );
    }
    ;

fetch_integer
    : SES_V_INTEGER
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_SQLINFO, (void *) $<strval>1 );
    }
    | TS_PLUS_SIGN SES_V_INTEGER
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_SQLINFO, (void *) $<strval>2 );
    }
    | TS_MINUS_SIGN SES_V_INTEGER
    {
        SChar sTmpStr[MAX_NUMBER_LEN];
        idlOS::snprintf( sTmpStr, MAX_NUMBER_LEN ,"-%s", $<strval>2 );
        gUlpCodeGen.ulpGenEmSQL( GEN_SQLINFO, (void *) sTmpStr );
    }
    ;


    /////////////////////////////////////
    ////  esql_stmt : close_cursor_stmt
    /////////////////////////////////////

close_cursor_stmt
    : TR_CLOSE SES_V_RELEASE SES_V_IDENTIFIER
    {
        gUlpStmttype = S_CloseRel;
        gUlpCodeGen.ulpGenEmSQL( GEN_CURNAME, (void *) $<strval>3 );
    }
    | TR_CLOSE SES_V_IDENTIFIER
    {
        gUlpStmttype = S_Close;
        gUlpCodeGen.ulpGenEmSQL( GEN_CURNAME, (void *) $<strval>2 );
    }
    ;

    /////////////////////////////////////
    ////  esql_stmt : autocommit_stmt
    /////////////////////////////////////

autocommit_stmt
    : SES_V_AUTOCOMMIT TR_ON
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_SQLINFO, (void *) "1" /*ON*/);
    }
    | SES_V_AUTOCOMMIT TR_OFF
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_SQLINFO, (void *) "0" /*OFF*/);
    }
    ;

    /////////////////////////////////////
    ////  esql_stmt : conn_stmt
    /////////////////////////////////////

conn_stmt
    : TR_CONNECT SES_V_HOSTVARIABLE TO_IDENTIFIED TR_BY SES_V_HOSTVARIABLE
    {
        ulpSymTElement *sSymNode;

        // User name
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>2+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenAddHostVarList( $<strval>2+1, sSymNode ,
                                          gUlpIndName, NULL, NULL, HV_IN_TYPE);

        // Password name
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>5+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenAddHostVarList( $<strval>5+1, sSymNode ,
                                          gUlpIndName, NULL, NULL, HV_IN_TYPE);

        gUlpCodeGen.ulpIncHostVarNum( 2 );
    }
    | TR_CONNECT SES_V_HOSTVARIABLE TO_IDENTIFIED TR_BY SES_V_HOSTVARIABLE TR_OPEN SES_V_HOSTVARIABLE
    {
        ulpSymTElement *sSymNode;

        // User name
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>2+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenAddHostVarList( $<strval>2+1, sSymNode ,
                                          gUlpIndName, NULL, NULL, HV_IN_TYPE);

        // Password name
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>5+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenAddHostVarList( $<strval>5+1, sSymNode ,
                                          gUlpIndName, NULL, NULL, HV_IN_TYPE);

        /* using open :drivername */
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>7+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        // driver name이라고 표시함.
        //sSymNode -> mMoreInfo = 1;
        //gUlpCodeGen.ulpGenAddHostVarList( sSymNode );

        gUlpCodeGen.ulpIncHostVarNum( 2 );
    }
    | TR_CONNECT SES_V_HOSTVARIABLE TO_IDENTIFIED TR_BY SES_V_HOSTVARIABLE TR_USING SES_V_HOSTVARIABLE
    {
        ulpSymTElement *sSymNode;

        // User name
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>2+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenAddHostVarList( $<strval>2+1, sSymNode ,
                                          gUlpIndName, NULL, NULL, HV_IN_TYPE);

        // Password name
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>5+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenAddHostVarList( $<strval>5+1, sSymNode ,
                                          gUlpIndName, NULL, NULL, HV_IN_TYPE);


        /* using :conn_opt1 */
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>7+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        // driver name이라고 표시함.
        gUlpCodeGen.ulpGenAddHostVarList( $<strval>7+1, sSymNode ,
                                          gUlpIndName, NULL, NULL, HV_IN_TYPE);

        gUlpCodeGen.ulpIncHostVarNum( 3 );
    }
    | TR_CONNECT SES_V_HOSTVARIABLE TO_IDENTIFIED TR_BY SES_V_HOSTVARIABLE TR_USING SES_V_HOSTVARIABLE TS_COMMA SES_V_HOSTVARIABLE
    {
        ulpSymTElement *sSymNode;

        // User name
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>2+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenAddHostVarList( $<strval>2+1, sSymNode ,
                                          gUlpIndName, NULL, NULL, HV_IN_TYPE);

        // Password name
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>5+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenAddHostVarList( $<strval>5+1, sSymNode ,
                                          gUlpIndName, NULL, NULL, HV_IN_TYPE);

        /* using :conn_opt1, :conn_opt2 */
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>7+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenAddHostVarList( $<strval>7+1, sSymNode ,
                                          gUlpIndName, NULL, NULL, HV_IN_TYPE);


        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>9+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenAddHostVarList( $<strval>9+1, sSymNode ,
                                          gUlpIndName, NULL, NULL, HV_IN_TYPE);

        gUlpCodeGen.ulpIncHostVarNum( 4 );
    }
    | TR_CONNECT SES_V_HOSTVARIABLE TO_IDENTIFIED TR_BY SES_V_HOSTVARIABLE TR_USING SES_V_HOSTVARIABLE TS_COMMA SES_V_HOSTVARIABLE TR_OPEN SES_V_HOSTVARIABLE
    {
        ulpSymTElement *sSymNode;

        // User name
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>2+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenAddHostVarList( $<strval>2+1, sSymNode ,
                                          gUlpIndName, NULL, NULL, HV_IN_TYPE);

        // Password name
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>5+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenAddHostVarList( $<strval>5+1, sSymNode ,
                                          gUlpIndName, NULL, NULL, HV_IN_TYPE);

        /* using :conn_opt1, :conn_opt2 open :drivername */
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>7+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenAddHostVarList( $<strval>7+1, sSymNode ,
                                          gUlpIndName, NULL, NULL, HV_IN_TYPE);

        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>9+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenAddHostVarList( $<strval>9+1, sSymNode ,
                                          gUlpIndName, NULL, NULL, HV_IN_TYPE);

        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>11+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        // driver name이라고 표시함.
        //sSymNode -> mMoreInfo = 1;
        //gUlpCodeGen.ulpGenAddHostVarList( sSymNode );

        gUlpCodeGen.ulpIncHostVarNum( 4 );
    }
    | TR_CONNECT SES_V_HOSTVARIABLE TO_IDENTIFIED TR_BY SES_V_HOSTVARIABLE TR_USING SES_V_HOSTVARIABLE TR_OPEN SES_V_HOSTVARIABLE
    {
        ulpSymTElement *sSymNode;

        // User name
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>2+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenAddHostVarList( $<strval>2+1, sSymNode , gUlpIndName,
                                          NULL, NULL, HV_IN_TYPE);

        // Password name
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>5+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenAddHostVarList( $<strval>5+1, sSymNode , gUlpIndName, NULL,
                                          NULL, HV_IN_TYPE);

        /* using :conn_opt1 open :drivername */
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>7+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenAddHostVarList( $<strval>7+1, sSymNode , gUlpIndName, NULL,
                                          NULL, HV_IN_TYPE);

        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>9+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        // driver name이라고 표시함.
        //sSymNode -> mMoreInfo = 1;
        //gUlpCodeGen.ulpGenAddHostVarList( sSymNode );

        gUlpCodeGen.ulpIncHostVarNum( 3 );
    }
    ;

    /////////////////////////////////////
    ////  esql_stmt : disconn_stmt
    /////////////////////////////////////

disconn_stmt
    : TR_DISCONNECT
    ;

    /////////////////////////////////////
    ////  esql_stmt : free_lob_loc_stmt
    /////////////////////////////////////

free_lob_loc_stmt
    : for_clause SES_V_FREE TR_LOB free_lob_loc_list
    {

    }
    | SES_V_FREE TR_LOB free_lob_loc_list
    {

    }
    ;

    //////////////////////////////////////////////////////////////////////
    ////
    ////  dsql_stmt
    ////   : dynamic method관련 embedded sql 구문 문법.
    ////
    //////////////////////////////////////////////////////////////////////

dsql_stmt
    : alloc_descriptor_stmt
    {
        gUlpStmttype    = S_IGNORE;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | dealloc_descriptor_stmt
    {
        gUlpStmttype    = S_IGNORE;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | prepare_stmt
    {
        gUlpStmttype = S_Prepare;
        gUlpIsPrintStmt = ID_TRUE;
    }
    | dealloc_prepared_stmt
    {
        gUlpStmttype    = S_IGNORE;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | execute_immediate_stmt
    {
        gUlpStmttype    = S_ExecIm;
        gUlpIsPrintStmt = ID_TRUE;
    }
    | execute_stmt
    {
        gUlpStmttype    = S_ExecDML;
        gUlpIsPrintStmt = ID_FALSE;
    }
    /* BUG-41010 Add dynamic binding feature on Apre */
    | bind_stmt
    {
        gUlpStmttype    = S_BindVariables;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | set_array_stmt
    {
        gUlpStmttype    = S_SetArraySize;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | select_list_stmt
    {
        gUlpStmttype    = S_SelectList;
        gUlpIsPrintStmt = ID_FALSE;
    }
    ;

alloc_descriptor_stmt
    : SES_V_ALLOCATE SES_V_DESCRIPTOR SES_V_HOSTVARIABLE with_max_option
    | SES_V_ALLOCATE SES_V_DESCRIPTOR SES_V_IDENTIFIER with_max_option
    ;

with_max_option
  : /* empty */
  | TR_WITH SES_V_IDENTIFIER SES_V_INTEGER    // bugbug : only natural number
  {
      if(idlOS::strncasecmp("MAX", $<strval>2, 3) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  }
  ;

dealloc_descriptor_stmt
    : SES_V_DEALLOCATE SES_V_DESCRIPTOR SES_V_HOSTVARIABLE
    | SES_V_DEALLOCATE SES_V_DESCRIPTOR SES_V_IDENTIFIER
    ;

prepare_stmt
    : begin_prepare SES_V_HOSTVARIABLE
    {
        SChar sTmpStr[MAX_HOSTVAR_NAME_SIZE];
        ulpSymTElement *sSymNode;

        if ( (sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>2+1, gUlpCurDepth )) == NULL )
        {
            //host 변수 못찾는 error
        }

        if( sSymNode != NULL )
        {
            if ( (sSymNode->mType == H_VARCHAR) ||
                 (sSymNode->mType == H_NVARCHAR) )
            {
                idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE,
                                "%s.arr",
                                $<strval>2+1 );
                gUlpCodeGen.ulpGenEmSQL( GEN_QUERYHV, (void *) sTmpStr );
            }
            else
            {
                gUlpCodeGen.ulpGenEmSQL( GEN_QUERYHV, $<strval>2+1 );
            }
        }
        else
        {
            gUlpCodeGen.ulpGenEmSQL( GEN_QUERYHV, $<strval>2+1 );
        }
    }
    | begin_prepare direct_sql_stmt
    {
        /* BUG-40939 */
        SChar* sTmpQueryBuf;
        UInt   sStmtNameLength = 0;
        sStmtNameLength = idlOS::strlen( gUlpCodeGen.ulpGenGetEmSQLInfo()->mStmtName );
        sTmpQueryBuf = idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                      gUlpCodeGen.ulpGenGetEmSQLInfo()->mStmtName ) + sStmtNameLength;
                                      
        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                 idlOS::strstr( sTmpQueryBuf, $<strval>2) );
    }
    | begin_prepare indirect_sql_stmt
    {
        /* BUG-40939 */
        SChar* sTmpQueryBuf;
        SInt   sStmtNameLength = 0;
        sStmtNameLength = idlOS::strlen( gUlpCodeGen.ulpGenGetEmSQLInfo()->mStmtName );
        sTmpQueryBuf = idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                      gUlpCodeGen.ulpGenGetEmSQLInfo()->mStmtName ) + sStmtNameLength;
        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                 idlOS::strstr( sTmpQueryBuf, $<strval>2) );
    }
    ;

begin_prepare
    : SES_V_PREPARE SES_V_IDENTIFIER TR_FROM
    {
        if ( idlOS::strlen($<strval>2) >= MAX_STMT_NAME_LEN )
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Exceed_Max_Stmtname_Error,
                             $<strval>2 );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
        gUlpCodeGen.ulpGenEmSQL( GEN_STMTNAME, (void *) $<strval>2 );
    }
    ;

dealloc_prepared_stmt
    : SES_V_DEALLOCATE SES_V_PREPARE SES_V_IDENTIFIER
    ;
/*
describe_stmt
  : describe_input_stmt
  | describe_output_stmt
  ;

describe_input_stmt
  : TR_DESCRIBE SES_V_INPUT SES_V_IDENTIFIER TR_USING using_descriptor
  ;

describe_output_stmt
  : TR_DESCRIBE TR_OUTPUT SES_V_IDENTIFIER TR_USING using_descriptor
  ;
*/

using_descriptor
  : TR_DESCRIPTOR host_variable
  ;

execute_immediate_stmt
    : begin_immediate SES_V_HOSTVARIABLE
    {
        SChar sTmpStr[MAX_HOSTVAR_NAME_SIZE];
        ulpSymTElement *sSymNode;

        if ( (sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>2+1, gUlpCurDepth )) == NULL )
        {
            //don't report error
        }

        if ( sSymNode != NULL )
        {
            if ( (sSymNode->mType == H_VARCHAR) ||
                 (sSymNode->mType == H_NVARCHAR) )
            {
                idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE,
                                 "%s.arr",
                                 $<strval>2+1 );
                gUlpCodeGen.ulpGenEmSQL( GEN_QUERYHV, (void *) sTmpStr );
            }
            else
            {
                gUlpCodeGen.ulpGenEmSQL( GEN_QUERYHV, $<strval>2+1 );
            }
        }
        else
        {
            gUlpCodeGen.ulpGenEmSQL( GEN_QUERYHV, $<strval>2+1 );
        }
    }
    | begin_immediate direct_sql_stmt
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                 idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                                $<strval>2)
                               );
    }
    | begin_immediate indirect_sql_stmt
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                 idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                                $<strval>2)
                               );
    }
    ;

begin_immediate
    : TR_EXECUTE SES_V_IMMEDIATE
    ;

execute_stmt
  : TR_EXECUTE SES_V_IDENTIFIER
  {
      gUlpCodeGen.ulpGenEmSQL( GEN_STMTNAME, (void *) $<strval>2 );
  }
  | for_clause TR_EXECUTE SES_V_IDENTIFIER TR_USING in_host_var_list
  {
      gUlpCodeGen.ulpGenEmSQL( GEN_STMTNAME, (void *) $<strval>3 );
  }
  | TR_EXECUTE SES_V_IDENTIFIER TR_USING in_host_var_list
  {
      gUlpCodeGen.ulpGenEmSQL( GEN_STMTNAME, (void *) $<strval>2 );
  }
  | TR_EXECUTE SES_V_IDENTIFIER TR_USING using_descriptor
  {
      gUlpCodeGen.ulpGenEmSQL( GEN_STMTNAME, (void *) $<strval>2 );
  }
  ;

/* BUG-41010 add dynamic binding */
bind_stmt
  : TR_DESCRIBE TR_BIND TR_VARIABLES TR_FOR SES_V_IDENTIFIER TR_INTO host_variable
  {
      gUlpCodeGen.ulpGenEmSQL( GEN_STMTNAME, (void *) $<strval>5 );
  }
  ;

set_array_stmt
  : TR_FOR SES_V_IDENTIFIER host_variable
  {
      gUlpCodeGen.ulpGenEmSQL( GEN_STMTNAME, (void *) $<strval>2 );
  }
  ;

/* BUG-41010 add dynamic binding */
select_list_stmt
  : TR_DESCRIBE TR_SELECT SES_V_IDENTIFIER TR_FOR SES_V_IDENTIFIER TR_INTO host_variable
  {
      if(idlOS::strncasecmp("LIST", $<strval>3, 4) != 0)
      {
          // error 처리
          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
      else
      {
          gUlpCodeGen.ulpGenEmSQL( GEN_STMTNAME, (void *) $<strval>5 );
      }
  }
  ;


    //////////////////////////////////////////////////////////////////////
    ////
    ////  proc_stmt
    ////   : PROCEDURE / FUNCTION 문법.
    ////
    //////////////////////////////////////////////////////////////////////


proc_stmt
    /* PROCEDURE and FUNCTION */
    : SP_create_or_replace_procedure_statement TS_SEMICOLON
    {
        gUlpStmttype    = S_DirectOTH;
        gUlpIsPrintStmt = ID_TRUE;
        gUlpCodeGen.ulpGenCutQueryTail4PSM( ';' );

    }
    | SP_create_or_replace_function_statement TS_SEMICOLON
    {
        gUlpStmttype    = S_DirectOTH;
        gUlpIsPrintStmt = ID_TRUE;
        gUlpCodeGen.ulpGenCutQueryTail4PSM( ';' );
    }
    | SP_create_or_replace_typeset_statement TS_SEMICOLON
    {
        gUlpStmttype    = S_DirectOTH;
        gUlpIsPrintStmt = ID_TRUE;
        gUlpCodeGen.ulpGenCutQueryTail4PSM( ';' );
    }
    | TR_EXECUTE TR_BEGIN exec_func_stmt TS_SEMICOLON TR_END TS_SEMICOLON
    {
        idBool sTrue;
        sTrue = ID_TRUE;
        gUlpStmttype    = S_DirectPSM;
        gUlpIsPrintStmt = ID_TRUE;
        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                 idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                                $<strval>3)
                               );
        gUlpCodeGen.ulpGenCutQueryTail4PSM( ';' );
        gUlpCodeGen.ulpGenCutQueryTail4PSM( ';' );
        gUlpCodeGen.ulpGenEmSQL( GEN_PSMEXEC, (void *) &sTrue );
    }
    | TR_EXECUTE SP_anonymous_block_statement TS_SEMICOLON
    {
        idBool sTrue;
        gUlpIsPrintStmt = ID_TRUE;
        
        if ((gUlpProcObjCount == 1) && (gUlpPSMObjName != NULL))
        {
            /* BUG-46824 procedure 
             * BEGIN END안에 object_name이 한개이므로 procedure로 간주한다. 
             * 예)
             * BEGIN
             *     PROC1;
             * END; 
             */
            sTrue = ID_TRUE;
            gUlpStmttype = S_DirectPSM;
            /* BUG-47868 object_name 이 한개일때만 ulpGenCutStringTail4PSM 호출 */
            gUlpCodeGen.ulpGenCutStringTail4PSM( gUlpPSMObjName, ';' );
            gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR, gUlpPSMObjName);
        }
        else
        {
            /* BUG-46824 anonymous block */
            sTrue = ID_FALSE;
            gUlpStmttype = S_DirectANONPSM;
            gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                     idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                                    $<strval>2)
                                   );
        }
        gUlpCodeGen.ulpGenCutQueryTail4PSM( ';' );
        
        gUlpCodeGen.ulpGenEmSQL( GEN_PSMEXEC, (void *) &sTrue );
    }
    ;

    //////////////////////////////////////////////////////////////////////
    ////
    ////  etc_stmt
    ////   : 나머지 embedded sql 구문 문법.
    //////////////////////////////////////////////////////////////////////

etc_stmt
    : SES_V_FREE
    {
        gUlpStmttype    = S_Free;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | SES_V_BATCH TR_ON
    {
        gUlpStmttype    = S_Batch;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | SES_V_BATCH TR_OFF
    {
        gUlpStmttype    = S_Batch;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | TR_ALTER TR_SESSION TR_SET SES_V_AUTOCOMMIT TS_EQUAL_SIGN TR_TRUE
    /* ALTER SESSION SET AUTOCOMMIT = TRUE */
    {
        gUlpStmttype = S_AutoCommit;
        gUlpIsPrintStmt = ID_FALSE;
        gUlpCodeGen.ulpGenEmSQL( GEN_SQLINFO, (void *) "1" /*ON*/);
    }
    | TR_ALTER TR_SESSION TR_SET SES_V_AUTOCOMMIT TS_EQUAL_SIGN TR_FALSE
    /* ALTER SESSION SET AUTOCOMMIT = FALSE */
    {
        gUlpStmttype = S_AutoCommit;
        gUlpIsPrintStmt = ID_FALSE;
        gUlpCodeGen.ulpGenEmSQL( GEN_SQLINFO, (void *) "0" /*OFF*/);
    }
    | TR_ALTER TR_SESSION TR_SET SES_V_IDENTIFIER TS_EQUAL_SIGN SES_V_LITERAL
    /* ALTER SESSION SET DEFAULT_DATE_FORMAT = 'literal' */
    {
        if(idlOS::strcasecmp("DEFAULT_DATE_FORMAT", $<strval>4) != 0 &&
           idlOS::strcasecmp("DATE_FORMAT", $<strval>4) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }

        gUlpStmttype = S_AlterSession;
        gUlpIsPrintStmt = ID_FALSE;
        gUlpCodeGen.ulpGenEmSQL( GEN_EXTRASTRINFO, $<strval>6 );
    }
    | TR_REGISTER TR_FAILOVERCB SES_V_HOSTVARIABLE
    {
        ulpSymTElement *sSymNode;

        // failover var. name
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>3+1, gUlpCurDepth ) ) == NULL )
        {
            // error 처리.
        }
        else
        {
            gUlpCodeGen.ulpGenAddHostVarList( $<strval>3+1, sSymNode , gUlpIndName, NULL,
                                              NULL, HV_IN_TYPE);
        }

        gUlpCodeGen.ulpIncHostVarNum( 1 );

        gUlpCodeGen.ulpGenEmSQL( GEN_SQLINFO, (void *) "1" /*SET*/);
        gUlpStmttype = S_FailOver;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | TR_UNREGISTER TR_FAILOVERCB
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_SQLINFO, (void *) "0" /*UNSET*/);
        gUlpStmttype = S_FailOver;
        gUlpIsPrintStmt = ID_FALSE;
    }
    ;


    //////////////////////////////////////////////////////////////////////
    ////
    ////  option_stmt
    ////   : mutithred option 관련 embedded sql 구문 문법.
    //////////////////////////////////////////////////////////////////////

option_stmt
  : APRE_V_OPTION TS_OPENING_PARENTHESIS SES_V_THREADS TS_EQUAL_SIGN TR_TRUE TS_CLOSING_PARENTHESIS
    {
        idBool sTrue = ID_TRUE;
        gUlpCodeGen.ulpGenComment( gUlpCodeGen.ulpGetQueryBuf() );
        gUlpCodeGen.ulpGenEmSQL( GEN_MT, (void *)&sTrue );
    }
  | APRE_V_OPTION TS_OPENING_PARENTHESIS SES_V_THREADS TS_EQUAL_SIGN TR_FALSE TS_CLOSING_PARENTHESIS
  {
        idBool sFalse = ID_FALSE;
        gUlpCodeGen.ulpGenComment( gUlpCodeGen.ulpGetQueryBuf() );
        gUlpCodeGen.ulpGenEmSQL( GEN_MT, (void *)&sFalse );
  }
  ;


    //////////////////////////////////////////////////////////////////////
    ////
    ////  exception_stmt
    ////   : whenever exception 처리 관련 embedded sql 구문 문법.
    //////////////////////////////////////////////////////////////////////

exception_stmt
    : SES_V_WHENEVER SES_V_SQLERROR TR_CONTINUE
    {
        gUlpCodeGen.ulpGenSetWhenever( gUlpCurDepth,
                                       GEN_WHEN_SQLERROR,
                                       GEN_WHEN_CONT,
                                       NULL );
    }
    | SES_V_WHENEVER SES_V_SQLERROR TR_GOTO SES_V_IDENTIFIER
    {
        gUlpCodeGen.ulpGenSetWhenever( gUlpCurDepth,
                                       GEN_WHEN_SQLERROR,
                                       GEN_WHEN_GOTO,
                                       $<strval>4 );
    }
    | SES_V_WHENEVER SES_V_SQLERROR SES_V_DO SES_V_BREAK
    {
        gUlpCodeGen.ulpGenSetWhenever( gUlpCurDepth,
                                       GEN_WHEN_SQLERROR,
                                       GEN_WHEN_DO_BREAK,
                                       NULL );
    }
    | SES_V_WHENEVER SES_V_SQLERROR SES_V_DO TR_CONTINUE
    {
        gUlpCodeGen.ulpGenSetWhenever( gUlpCurDepth,
                                       GEN_WHEN_SQLERROR,
                                       GEN_WHEN_DO_CONT,
                                       NULL );
    }
    | SES_V_WHENEVER SES_V_SQLERROR SES_V_DO SES_V_IDENTIFIER TS_OPENING_PARENTHESIS
    {
        SChar  sTmpStr[MAX_EXPR_LEN];

        idlOS::snprintf( sTmpStr, MAX_EXPR_LEN , "%s(", $<strval>4 );
        ulpCOMPWheneverFunc( sTmpStr );
        gUlpCodeGen.ulpGenSetWhenever( gUlpCurDepth,
                                       GEN_WHEN_SQLERROR,
                                       GEN_WHEN_DO_FUNC,
                                       sTmpStr );
    }
    | SES_V_WHENEVER SES_V_SQLERROR SES_V_IDENTIFIER
    {
        gUlpCodeGen.ulpGenSetWhenever( gUlpCurDepth,
                                       GEN_WHEN_SQLERROR,
                                       GEN_WHEN_STOP,
                                       NULL );
    }
    | SES_V_WHENEVER TR_NOT SES_V_FOUND TR_CONTINUE
    {
        gUlpCodeGen.ulpGenSetWhenever( gUlpCurDepth,
                                       GEN_WHEN_NOTFOUND,
                                       GEN_WHEN_CONT,
                                       NULL );
    }
    | SES_V_WHENEVER TR_NOT SES_V_FOUND TR_GOTO SES_V_IDENTIFIER
    {
        gUlpCodeGen.ulpGenSetWhenever( gUlpCurDepth,
                                       GEN_WHEN_NOTFOUND,
                                       GEN_WHEN_GOTO,
                                       $<strval>5 );
    }
    | SES_V_WHENEVER TR_NOT SES_V_FOUND SES_V_DO SES_V_BREAK
    {
        gUlpCodeGen.ulpGenSetWhenever( gUlpCurDepth,
                                       GEN_WHEN_NOTFOUND,
                                       GEN_WHEN_DO_BREAK,
                                       NULL );
    }
    | SES_V_WHENEVER TR_NOT SES_V_FOUND SES_V_DO TR_CONTINUE
    {
        gUlpCodeGen.ulpGenSetWhenever( gUlpCurDepth, 
                                       GEN_WHEN_NOTFOUND,
                                       GEN_WHEN_DO_CONT,
                                       NULL );
    }
    | SES_V_WHENEVER TR_NOT SES_V_FOUND SES_V_DO SES_V_IDENTIFIER TS_OPENING_PARENTHESIS
    {
        SChar  sTmpStr[MAX_EXPR_LEN];

        idlOS::snprintf( sTmpStr, MAX_EXPR_LEN , "%s(", $<strval>5 );
        ulpCOMPWheneverFunc( sTmpStr );
        gUlpCodeGen.ulpGenSetWhenever( gUlpCurDepth,
                                       GEN_WHEN_NOTFOUND,
                                       GEN_WHEN_DO_FUNC,
                                       sTmpStr );
    }
    | SES_V_WHENEVER TR_NOT SES_V_FOUND SES_V_IDENTIFIER
    {
        gUlpCodeGen.ulpGenSetWhenever( gUlpCurDepth,
                                       GEN_WHEN_NOTFOUND,
                                       GEN_WHEN_STOP,
                                       NULL );
    }
    ;


threads_stmt
    : /* empty */
    | SES_V_THREADS
    {
        gUlpStmttype    = S_IGNORE;
        gUlpIsPrintStmt = ID_FALSE;
    }
    ;
sharedptr_stmt
    : TR_BEGIN TR_DECLARE TR_POINTER TR_AT SES_V_IDENTIFIER TS_SEMICOLON
    {
        gUlpSharedPtrPrevCond  = gUlpCOMPPrevCond;
        gUlpCOMPStartCond = CP_ST_C;
        idlOS::strcpy ( gUlpCodeGen.ulpGetSharedPtrName(), $<strval>5 );
        gUlpParseInfo.mIsSharedPtr = ID_TRUE;

    }
    | TR_END TR_DECLARE TR_POINTER TS_SEMICOLON
    {
        gUlpCOMPStartCond = gUlpSharedPtrPrevCond;
        gUlpParseInfo.mIsSharedPtr = ID_FALSE;
        gUlpCodeGen.ulpClearSharedPtrInfo();
    }
    ;

getdiag_stmt
    : SES_V_GET current_opt SES_V_DIAGNOSTICS statement_information_list
    {
        gUlpStmttype = S_GetStmtDiag;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | SES_V_GET current_opt SES_V_DIAGNOSTICS SES_V_CONDITION condition_number condition_information_list
    {
        gUlpStmttype = S_GetConditionDiag;
        gUlpIsPrintStmt = ID_FALSE;
    }
    ;
 
 current_opt
    : /* empty */
    | SES_V_CURRENT
    ;
 
 statement_information_list
    : statement_information_list TS_COMMA statement_information_item
    | statement_information_item
    ;
 
 statement_information_item
    :  SES_V_HOSTVARIABLE TS_EQUAL_SIGN statement_information_item_name
    {
        ulpValidateHostValueWithDiagType(
                              yyvsp,
                              HV_OUT_TYPE,
                              HV_FILE_NONE,
                              ID_FALSE,
                              (SInt)3,
                              (SInt)1,
                              (SInt)0,
                              (ulpHostDiagType)$<intval>3
                            );
    }
    ;
 
 statement_information_item_name
    : SES_V_NUMBER
    {
        $<intval>$ = H_STMT_DIAG_NUMBER;
    }
    | SES_V_ROW_COUNT
    {
        $<intval>$ = H_STMT_DIAG_ROW_COUNT;
    }
    ;
 
 condition_number
    : SES_V_INTEGER
    {
        SInt sNum;

        sNum = idlOS::atoi($<strval>1);

        if ( sNum < 1 )
        {
            //The count of FOR clause must be greater than zero
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_FOR_iternum_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
        else
        {
            gUlpCodeGen.ulpGenEmSQL( GEN_CONDITIONNUM, (void *) $<strval>1 );
        }
    }
    | SES_V_HOSTVARIABLE
    {
        ulpSymTElement *sSymNode;

        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>1+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
            //declare section에 선언하지 않은 변수도 condition 절에 호스트 변수로
            //사용할 수 있으므로 에러 처리하지 않음
        }

        gUlpCodeGen.ulpGenEmSQL( GEN_CONDITIONNUM, $<strval>1+1 );
    }
    ;
 
 condition_information_list
    : condition_information_list TS_COMMA condition_information_item
    | condition_information_item
    ;
 
 condition_information_item
    :  SES_V_HOSTVARIABLE indicator_opt TS_EQUAL_SIGN condition_information_item_name
    {
        ulpValidateHostValueWithDiagType(
                              yyvsp,
                              HV_OUT_TYPE,
                              HV_FILE_NONE,
                              ID_FALSE,
                              (SInt)4,
                              (SInt)1,
                              (SInt)0,
                              (ulpHostDiagType)$<intval>4
                            );
    }
    ;
 
 condition_information_item_name
    :  SES_V_RETURNED_SQLCODE
    {
        $<intval>$ = H_COND_DIAG_RETURNED_SQLCODE;
    }
    | SES_V_RETURNED_SQLSTATE
    {
        $<intval>$ = H_COND_DIAG_RETURNED_SQLSTATE;
    }
    | SES_V_MESSAGE_TEXT
    {
        $<intval>$ = H_COND_DIAG_MESSAGE_TEXT;
    }
    | SES_V_ROW_NUMBER
    {
        $<intval>$ = H_COND_DIAG_ROW_NUMBER;
    }
    | SES_V_COLUMN_NUMBER
    {
        $<intval>$ = H_COND_DIAG_COLUMN_NUMBER;
    }
    ;

    //////////////////////////////////////////////////////////////////////
    ////
    ////  sql_stmt
    ////   : 주요 DDL, DML, DCL 구문들 문법.
    ////
    //////////////////////////////////////////////////////////////////////

sql_stmt
    // DDL, DCL
    : direct_sql_stmt
    {
        gUlpStmttype = S_DirectOTH;
        gUlpIsPrintStmt = ID_TRUE;
    }
    // DML
    | indirect_sql_stmt
    {
        gUlpIsPrintStmt = ID_TRUE;
    }
    | commit_sql_stmt
    {
        gUlpStmttype = S_Commit;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | rollback_sql_stmt
    {
    }
    ;

direct_sql_stmt
    /* session parameter */
    : alter_session_set_statement
    /* system */
    | alter_system_statement
    /* set */
    | set_statement
    /* transaction */
    | savepoint_statement
    | set_transaction_statement
    | commit_force_database_link_statement          /* BUG-37100 */
    | rollback_force_database_link_statement        /* BUG-37100 */
    /* user */
    | create_user_statement
    | alter_user_statement
    /* privileges */
    | grant_statement
    | revoke_statement
    /* REPLICATION */
    | replication_statement
    /* DDL */
    | create_table_statement
    | get_default_statement
    | get_condition_statement
    | alter_table_statement
    | alter_table_constraint_statement
    | rename_table_statement
    | flashback_table_statement                     /* PROJ-2441 flashback */
    | disjoin_table_statement                       /* PROJ-1810 Partition Exchange */
    | join_table_statement                          /* PROJ-1810 Partition Exchange */
    | truncate_table_statement
    | create_index_statement
    | create_sequence_statement
    | create_view_statement
    | create_or_replace_view_statement
    | alter_sequence_statement
    | alter_index_statement
    | alter_view_statement
    | drop_table_statement
    | purge_table_statement                         /* PROJ-2441 flashback */
    | drop_index_statement
    | drop_sequence_statement
    | drop_user_statement
    | drop_view_statement
    | create_database_link_statement                /* BUG-37100 */
    | drop_database_link_statement                  /* BUG-37100 */
    | alter_database_link_statement                 /* BUG-37100 */
    | close_database_link_statement                 /* BUG-37100 */
    /* for DIRECTORY */
    | create_or_replace_directory_statement
    | drop_directory_statement
    | lock_table_statement
    | SP_alter_procedure_statement
    | SP_alter_function_statement
    | SP_drop_procedure_statement
    | SP_drop_function_statement
    /* typeset */
    | SP_drop_typeset_statement
    /* Database */
    | create_database_statement
    | alter_database_statement
    | drop_database_statement
    /* TABLESPACE */
    | create_tablespace_statement
    | create_temp_tablespace_statement
    | alter_tablespace_ddl_statement
    | alter_tablespace_dcl_statement
    | drop_tablespace_statement
    /* TRIGGER */
    | create_trigger_statement
    | alter_trigger_statement
    | drop_trigger_statement
    /* Synonym */
    | create_synonym_statement
    | alter_queue_statement /* BUG-45921 */
    | drop_synonym_statement
    /* Queue */
    | create_queue_statement
    | drop_queue_statement
    /* PROJ-2211 Materialized View */
    | create_materialized_view_statement
    | alter_materialized_view_statement
    | drop_materialized_view_statement
    /* others */
    | comment_statement
    /* PROJ-1438 Job Scheduler */
    | create_job_statement
    | alter_job_statement
    | drop_job_statement
      /* PROJ-1812 ROLE */
    | create_role_statement
    | drop_role_statement
    ;

indirect_sql_stmt
    /* DML */
    /* DELETE */
    : pre_clause delete_statement
    {
        gUlpStmttype = S_DirectDML;

        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                 idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                                $<strval>2)
                               );

    }
    /* DELETE */
    | delete_statement
    {
        gUlpStmttype = S_DirectDML;

        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                 idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                                $<strval>1)
                               );
    }
    /* INSERT */
    | pre_clause insert_statement
    {
        gUlpStmttype = S_DirectDML;

        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                 idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                                $<strval>2)
                               );

    }
    /* INSERT */
    | insert_statement
    {
        gUlpStmttype = S_DirectDML;

        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                 idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                                $<strval>1)
                               );

    }
    /* UPDATE */
    | pre_clause update_statement
    {
        gUlpStmttype = S_DirectDML;

        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                 idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                                $<strval>2)
                               );

    }
    /* UPDATE */
    | update_statement
    {
        gUlpStmttype = S_DirectDML;

        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                 idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                                $<strval>1)
                               );

    }
    /* MERGE */
    | merge_statement
    {
        gUlpStmttype = S_DirectDML;

        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                 idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                                $<strval>1)
                               );

    }
    /* MOVE */
    | move_statement
    {
        gUlpStmttype = S_DirectDML;

        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                 idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                                $<strval>1)
                               );

    }
    /* SELECT */
    | select_or_with_select_statement_4emsql opt_for_update_clause opt_with_read_only
    {
        gUlpStmttype = S_DirectSEL;

        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                 idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                                $<strval>1)
                               );

    }
    /* DEQUEUE */
    | dequeue_statement opt_fifo opt_wait_clause
    {
        //is_select = sesTRUE;
        gUlpStmttype = S_DirectSEL;

        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                 idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                                $<strval>1)
                               );
    }
    /* ENQUEUE */
    | enqueue_statement
    {
        gUlpStmttype = S_DirectDML;

        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                 idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                                $<strval>1)
                               );
    }
    ;

pre_clause
    : for_clause
    | onerr_clause
    | onerr_clause for_clause
    ;

onerr_clause
    : SES_V_ONERR SES_V_HOSTVARIABLE TS_COMMA SES_V_HOSTVARIABLE
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_STATUSPTR, $<strval>2+1 );
        gUlpCodeGen.ulpGenEmSQL( GEN_ERRCODEPTR, $<strval>4+1 );
    }
    ;

for_clause
    : TR_FOR SES_V_INTEGER
    {
        SInt sNum;

        sNum = idlOS::atoi($<strval>2);

        if ( sNum < 1 )
        {
            //The count of FOR clause must be greater than zero
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_FOR_iternum_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
        else
        {
            gUlpCodeGen.ulpGenEmSQL( GEN_ITERS, (void *) $<strval>2 );
        }
    }
    | TR_FOR SES_V_HOSTVARIABLE
    {
        ulpSymTElement *sSymNode;

        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>2+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
        }

        gUlpCodeGen.ulpGenEmSQL( GEN_ITERS, $<strval>2+1 );
    }
    | SES_V_IDENTIFIER TR_FOR SES_V_INTEGER
    {

        if(idlOS::strncasecmp("ATOMIC", $<strval>1, 6) == 0)
        {
            if ( idlOS::atoi($<strval>3) < 1 )
            {
                //The count of FOR clause must be greater than zero
                // error 처리

                ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                 ulpERR_ABORT_COMP_FOR_iternum_Error );
                gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
            }
            else
            {
                gUlpCodeGen.ulpGenEmSQL( GEN_ITERS, (void *) $<strval>3 );
                gUlpCodeGen.ulpGenEmSQL( GEN_ATOMIC, (void *) "1" );
            }
        }
        else
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | SES_V_IDENTIFIER TR_FOR SES_V_HOSTVARIABLE
    {
        ulpSymTElement *sSymNode;

        if( idlOS::strncasecmp("ATOMIC", $<strval>1, 6) == 0 )
        {
            if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>3+1, gUlpCurDepth ) ) == NULL )
            {
                //host 변수 못찾는 error
            }

            gUlpCodeGen.ulpGenEmSQL( GEN_ITERS, $<strval>3+1 );

            gUlpCodeGen.ulpGenEmSQL( GEN_ATOMIC, (void *) "1" );
        }
        else
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    ;

commit_sql_stmt
    : commit_statement
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_SQLINFO, (void *) "0" );
    }
    | commit_statement SES_V_RELEASE
    {
        // release 구문이 오면 disconnect 해야한다.
        gUlpCodeGen.ulpGenEmSQL( GEN_SQLINFO, (void *) "1" );
    }
    ;

rollback_sql_stmt
    : rollback_statement
    {
        gUlpCodeGen.ulpGenEmSQL( GEN_SQLINFO, (void *) "2" );
    }
    | rollback_statement SES_V_RELEASE
    {
        // release 구문이 오면 disconnect 해야한다.
        gUlpCodeGen.ulpGenEmSQL( GEN_SQLINFO, (void *) "3" );
        // 나중에 rollback 구문을 comment로 출력할때 release 토큰은 제거됨을 주의하자.
        gUlpCodeGen.ulpGenCutQueryTail( $<strval>2 );
    }
    ;

/*****************************************
 * MISC
 ****************************************/

column_name
  : object_name
  | TR_AGER
  | TR_ARCHIVE
  | TR_ARCHIVELOG
  | TR_BACKUP
  | TR_CAST
  | TR_COMPILE
  | TR_DIRECTORY
  | TR_DISABLE
  | TR_ENABLE
  | TR_FLUSH
  | SES_V_HOLD
  | SES_V_LAST
  | TR_LEFT
  | TR_LIMIT
  | TR_LOGANCHOR
  | TR_MOVE
  | TR_NOARCHIVELOG
  | TR_READ
  | TR_RECOVER
  | TR_RIGHT
  | TA_ROWCOUNT
  | TR_SYNONYM
  | TR_UNTIL
  | TR_USING
/*| SES_V_LOGGING
  | SES_V_NOLOGGING
*/| TR_PARALLEL
  | TR_ENQUEUE
  | TR_QUEUE
  | TR_DEQUEUE
  | TR_FIFO
  | TR_LIFO
  | TR_NOPARALLEL
  | TA_STEP
  | TR_SEQUENCE
  | TR_TYPE
  | TR_TYPESET
  | TR_LOB
  | TR_STORE
  | TR_AT
  | TR_LESS
  | TR_THAN
  | TR_MOVEMENT
  | TR_COALESCE
  | TR_MERGE
  | TR_PARTITIONS
  | TR_SPLIT
  | TR_REBUILD
  | TR_VOLATILE
  | TR_OPEN
  | TR_CLOSE
  | TR_COMMENT // BUG-32512
  | TO_SEGMENT // BUG-34238 The apre is changed to supporting the SEGMENT keyword for column name.
  | TO_ACCESS
  | TO_NULLS  // PROJ-2435 order by nulls first/last
  ;

/* member built-in function의 경우 keyword를 허용함
 * associative array에 허용되는 member function
 * COUNT, DELETE, EXISTS, PRIOR, FIRST, LAST, NEXT */
memberfunc_name
  : object_name
  | TR_DELETE
  | TK_EXISTS
  | TR_PRIOR
  | SES_V_FIRST
  | SES_V_LAST
  | SES_V_NEXT
  ;

/* keyword지만 function으로 사용할 수 있음 */
keyword_function_name
  : TR_UNION
  | TO_REPLACE
  ;

alter_session_set_statement
    : TR_ALTER TR_SESSION TR_SET TA_REPLICATION TS_EQUAL_SIGN TR_TRUE
    | TR_ALTER TR_SESSION TR_SET TA_REPLICATION TS_EQUAL_SIGN TR_FALSE
    | TR_ALTER TR_SESSION TR_SET TA_REPLICATION TS_EQUAL_SIGN TR_DEFAULT
    | TR_ALTER TR_SESSION TR_SET TA_REPLICATION TS_EQUAL_SIGN SES_V_IDENTIFIER
    {
        if(idlOS::strncasecmp("EAGER", $<strval>6, 5) != 0 &&
           idlOS::strncasecmp("LAZY", $<strval>6, 4) != 0 &&
           idlOS::strncasecmp("ACKED", $<strval>6, 5) != 0 &&
           idlOS::strncasecmp("NONE", $<strval>6, 4) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER TR_SESSION TR_SET SES_V_IDENTIFIER SES_V_IDENTIFIER TS_EQUAL_SIGN SES_V_ONLY
    /* ALTER SESSION SET EXPLAIN PLAN = ONLY */
    {
        if(idlOS::strncasecmp("EXPLAIN", $<strval>4, 7) != 0 ||
           idlOS::strncasecmp("PLAN", $<strval>5, 4) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER TR_SESSION TR_SET SES_V_IDENTIFIER SES_V_IDENTIFIER TS_EQUAL_SIGN TR_ON
    /* ALTER SESSION SET EXPLAIN PLAN = ON */
    {
        if(idlOS::strncasecmp("EXPLAIN", $<strval>4, 7) != 0 ||
           idlOS::strncasecmp("PLAN", $<strval>5, 4) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER TR_SESSION TR_SET SES_V_IDENTIFIER SES_V_IDENTIFIER TS_EQUAL_SIGN TR_OFF
    /* ALTER SESSION SET EXPLAIN PLAN = OFF */
    {
        if(idlOS::strncasecmp("EXPLAIN", $<strval>4, 7) != 0 ||
           idlOS::strncasecmp("PLAN", $<strval>5, 4) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER TR_SESSION TR_SET SES_V_IDENTIFIER TS_EQUAL_SIGN SES_V_INTEGER
    /* ALTER SESSION SET property = integer */
    | TR_ALTER TR_SESSION TR_SET SES_V_IDENTIFIER SES_V_IDENTIFIER TS_EQUAL_SIGN SES_V_INTEGER
    /* ALTER SESSION SET STACK SIZE = integer */
    {
        if(idlOS::strncasecmp("STACK", $<strval>4, 5) != 0 ||
           idlOS::strncasecmp("SIZE", $<strval>5, 4) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER TR_SESSION TR_SET SES_V_IDENTIFIER TS_EQUAL_SIGN SES_V_IDENTIFIER
    /* ALTER SESSION SET property = VALUE */
    ;

alter_system_statement
    : TR_ALTER SES_V_IDENTIFIER SES_V_IDENTIFIER
    /* ALTER SYSTEM CHECKPOINT */
    /* ALTER SYSTEM COMPACT */
    {
        if(idlOS::strncasecmp("SYSTEM", $<strval>2, 6) != 0 ||
           idlOS::strncasecmp("CHECKPOINT", $<strval>3, 10) != 0 &&
           idlOS::strncasecmp("REORGANIZE", $<strval>3, 10) != 0 &&
           idlOS::strncasecmp("VERIFY", $<strval>3, 6) != 0 &&
           idlOS::strncasecmp("COMPACT", $<strval>3, 7) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
        if(idlOS::strncasecmp("COMPACT", $<strval>3, 7) == 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Not_Supported_ALTER_COMPACT_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER SES_V_IDENTIFIER SES_V_IDENTIFIER SES_V_IDENTIFIER
    /* ALTER SYSTEM MEMORY COMPACT */
    {
        if(idlOS::strncasecmp("SYSTEM", $<strval>2, 6) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
        else
        {
            if(idlOS::strncasecmp("MEMORY", $<strval>3, 6) == 0 &&
               idlOS::strncasecmp("COMPACT", $<strval>4, 7) == 0)
            {
                // Nothing to do 
            }
            else if(idlOS::strncasecmp("SWITCH", $<strval>3, 6) == 0 &&
                    idlOS::strncasecmp("LOGFILE", $<strval>4, 7) == 0)
            {
                // Nothing to do 
            }
            else
            {
                // error 처리

                ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                 ulpERR_ABORT_COMP_Unterminated_String_Error );
                gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
            }
        }
    }
    | TR_ALTER SES_V_IDENTIFIER TR_ARCHIVE SES_V_IDENTIFIER archivelog_start_option
    {
        if(idlOS::strncasecmp("SYSTEM", $<strval>2, 6) != 0 ||
           idlOS::strncasecmp("LOG", $<strval>4, 3) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER SES_V_IDENTIFIER TR_SET SES_V_IDENTIFIER TS_EQUAL_SIGN SES_V_INTEGER
    /* ALTER SYSTEM SET property = integer */
    {
        if(idlOS::strncasecmp("SYSTEM", $<strval>2, 6) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER SES_V_IDENTIFIER TR_SET SES_V_IDENTIFIER TS_EQUAL_SIGN TS_MINUS_SIGN SES_V_INTEGER
    /* ALTER SYSTEM SET property = - integer */
    {
        if(idlOS::strncasecmp("SYSTEM", $<strval>2, 6) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER SES_V_IDENTIFIER TR_SET SES_V_IDENTIFIER TS_EQUAL_SIGN SES_V_IDENTIFIER
    /* ALTER SYSTEM SET property = identifier */
    {
        if(idlOS::strncasecmp("SYSTEM", $<strval>2, 6) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER SES_V_IDENTIFIER TR_SET SES_V_IDENTIFIER TS_EQUAL_SIGN SES_V_LITERAL
    /* ALTER SYSTEM SET property = 'literal' */
    {
        if(idlOS::strncasecmp("SYSTEM", $<strval>2, 6) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER SES_V_IDENTIFIER SES_V_IDENTIFIER TO_ACCESS SES_V_IDENTIFIER
    /* ALTER SYSTEM RELOAD ACCESS LIST */
    {
        if (( idlOS::strncasecmp("SYSTEM", $<strval>2, 6) != 0 ) ||
            ( idlOS::strncasecmp("RELOAD", $<strval>3, 6) != 0 ) ||
            ( idlOS::strncasecmp("LIST", $<strval>5, 4) != 0 ))
        {
            // error 처리
            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER SES_V_IDENTIFIER SES_V_IDENTIFIER TA_SHARD SES_V_IDENTIFIER SES_V_IDENTIFIER opt_local
    /* ALTER SYSTEM RELOAD SHARD META NUMBER */
    {
        if ( ( idlOS::strncasecmp( "SYSTEM", $<strval>2, 6 ) != 0 ) ||
             ( idlOS::strncasecmp( "RELOAD", $<strval>3, 6 ) != 0 ) ||
             ( idlOS::strncasecmp( "META",   $<strval>5, 4 ) != 0 ) ||
             ( idlOS::strncasecmp( "NUMBER", $<strval>6, 6 ) != 0 ) )
        {
            // error 처리
            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG( &gUlpParseInfo.mErrorMgr ) );
        }
    }
    ;

opt_local
  : /* empty */
  | TR_LOCAL
  ;

archivelog_start_option
    : TR_START
    | SES_V_IDENTIFIER
    {
        if(idlOS::strncasecmp("STOP", $<strval>1, 4) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    ;

commit_statement
    : TR_COMMIT opt_work_clause
    ;

savepoint_statement
  : TR_SAVEPOINT object_name
  ;

rollback_statement
    : TR_ROLLBACK opt_work_clause
    {
        gUlpStmttype    = S_Commit;
        gUlpIsPrintStmt = ID_FALSE;
    }
    | TR_ROLLBACK opt_work_clause TR_TO TR_SAVEPOINT object_name
    {
        gUlpStmttype    = S_DirectRB;
        gUlpIsPrintStmt = ID_TRUE;
        gUlpCodeGen.ulpGenEmSQL( GEN_QUERYSTR,
                                 idlOS::strstr( gUlpCodeGen.ulpGetQueryBuf(),
                                                $<strval>1)
                               );
    }
    ;

opt_work_clause
  : /* empty */
  | TR_WORK
  ;

set_transaction_statement
    : TR_SET SES_V_IDENTIFIER transaction_mode
    {
        if(idlOS::strncasecmp("TRANSACTION", $<strval>2, 11) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER TR_SESSION TR_SET SES_V_IDENTIFIER transaction_mode
    {
        if(idlOS::strncasecmp("TRANSACTION", $<strval>4, 11) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    ;

/* BUG-37100 */
commit_force_database_link_statement
    : TR_COMMIT opt_work_clause SES_V_IDENTIFIER TR_DATABASE TR_LINK
    {
        if ( idlOS::strncasecmp( "FORCE", $<strval>3, 5 ) != 0 )
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    ;

/* BUG-37100 */
rollback_force_database_link_statement
    : TR_ROLLBACK opt_work_clause SES_V_IDENTIFIER TR_DATABASE TR_LINK
    {
        if ( idlOS::strncasecmp( "FORCE", $<strval>3, 5 ) != 0 )
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    ;

transaction_mode
  : TR_READ SES_V_ONLY
    /* READ ONLY */
  | TR_READ TR_WRITE
  | TR_ISOLATION TR_LEVEL TR_READ SES_V_IDENTIFIER
    /* ISOLATION LEVEL READ COMMITTED   */
    /* ISOLATION LEVEL READ UNCOMMITTED */
    {
        if(idlOS::strncasecmp("COMMITTED", $<strval>4, 9) != 0 &&
           idlOS::strncasecmp("UNCOMMITTED", $<strval>4, 11) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
  | TR_ISOLATION TR_LEVEL SES_V_IDENTIFIER TR_READ
    /* ISOLATION LEVEL REPEATABLE READ */
    {
        if(idlOS::strncasecmp("REPEATABLE", $<strval>3, 10) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
  | TR_ISOLATION TR_LEVEL SES_V_IDENTIFIER
    /* ISOLATION LEVEL SERIALIZABLE */
    {
        if(idlOS::strncasecmp("SERIALIZABLE", $<strval>3, 12) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
   ;

user_object_name
    : object_name
    | object_name TS_PERIOD object_name
    ;

user_object_column_name
    : object_name TS_PERIOD object_name
    | object_name TS_PERIOD object_name TS_PERIOD object_name
    ;

create_user_statement
    : TR_CREATE TR_USER object_name TO_IDENTIFIED TR_BY object_name
    | TR_CREATE TR_USER object_name TO_IDENTIFIED TR_BY object_name user_options
    ;

alter_user_statement
  : TR_ALTER TR_USER object_name user_option
  ;

user_options
  : user_options create_user_option
  | create_user_option
  ;

create_user_option
  : temporary_tablespace
  | default_tablespace
  | access
  | disable_tcp_option
  ;

user_option
  : password_def
  | temporary_tablespace
  | default_tablespace
  | access_options
  | disable_tcp_option
  ;

disable_tcp_option
  : TR_ENABLE SES_V_IDENTIFIER
  | TR_DISABLE SES_V_IDENTIFIER
  ;

access_options
    : access
    | access_options access
    ;

password_def
    : TO_IDENTIFIED TR_BY object_name
    ;

temporary_tablespace
    : TR_TEMPORARY TA_TABLESPACE object_name
    ;

default_tablespace
    : TR_DEFAULT TA_TABLESPACE object_name
    ;

access
    : TO_ACCESS object_name access_option
    ;

access_option
    : TR_ON
    | TR_OFF
    ;

drop_user_statement
  : TR_DROP TR_USER object_name opt_cascade_tok
  ;

/* PROJ-1812 ROLE */
drop_role_statement
  : TR_DROP TR_ROLE object_name
  ;

create_role_statement
  : TR_CREATE TR_ROLE object_name
  ;

grant_statement
    : grant_system_privileges_statement
    | grant_object_privileges_statement
    ;

grant_system_privileges_statement
    : TR_GRANT
      privilege_list
      TR_TO grantees_clause
    ;

grant_object_privileges_statement
    : TR_GRANT
      privilege_list
      TR_ON user_object_name
      TR_TO grantees_clause
      opt_with_grant_option
    | TR_GRANT
      privilege_list
      TR_ON TR_DIRECTORY object_name
      TR_TO grantees_clause
      opt_with_grant_option
    ;

privilege_list
    : privilege_list TS_COMMA privilege
    | privilege
    ;

privilege
    /* system privileges */
    : TR_ALTER SES_V_IDENTIFIER
    {
        if(idlOS::strncasecmp("SYSTEM", $<strval>2, 6) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_CREATE TR_ANY TO_INDEX
    | TR_ALTER TR_ANY TO_INDEX
    | TR_DROP TR_ANY TO_INDEX
    | TR_CREATE TR_PROCEDURE
    | TR_CREATE TR_ANY TR_PROCEDURE
    | TR_ALTER TR_ANY TR_PROCEDURE
    | TR_DROP TR_ANY TR_PROCEDURE
    | TR_EXECUTE TR_ANY TR_PROCEDURE
    /* Trigger */
    | TR_CREATE TR_TRIGGER
    | TR_CREATE TR_ANY TR_TRIGGER
    | TR_ALTER TR_ANY TR_TRIGGER
    | TR_DROP TR_ANY TR_TRIGGER
    /* Synonym */
    | TR_CREATE TR_SYNONYM
    | TR_CREATE TR_PUBLIC TR_SYNONYM
    | TR_DROP TR_ANY TR_SYNONYM
    | TR_DROP TR_PUBLIC TR_SYNONYM
    | TR_CREATE TR_SEQUENCE
    | TR_CREATE TR_ANY TR_SEQUENCE
    | TR_ALTER TR_ANY TR_SEQUENCE
    | TR_DROP TR_ANY TR_SEQUENCE
    | TR_SELECT TR_ANY TR_SEQUENCE
    | TR_CREATE TR_SESSION
    | TR_ALTER TR_SESSION
    | TR_CREATE TR_TABLE
    | TR_CREATE TR_ANY TR_TABLE
    | TR_ALTER TR_ANY TR_TABLE
    | TR_DELETE TR_ANY TR_TABLE
    | TR_DROP TR_ANY TR_TABLE
    | TR_INSERT TR_ANY TR_TABLE
    | TA_LOCK TR_ANY TR_TABLE
    | TR_SELECT TR_ANY TR_TABLE
    | TR_UPDATE TR_ANY TR_TABLE
    | TR_CREATE TR_USER
    | TR_ALTER TR_USER
    | TR_DROP TR_USER
    | TR_CREATE TR_VIEW
    | TR_CREATE TR_ANY TR_VIEW
    | TR_DROP TR_ANY TR_VIEW
    | TR_GRANT TR_ANY TR_PRIVILEGES
    | TR_CREATE TA_TABLESPACE
    | TR_ALTER TA_TABLESPACE
    | TR_DROP TA_TABLESPACE
    /* Directories */
    | TR_CREATE TR_ANY TR_DIRECTORY
    | TR_DROP TR_ANY TR_DIRECTORY
    /* PROJ-2211 Materialized View */
    | TR_CREATE TO_MATERIALIZED TR_VIEW
    | TR_CREATE TR_ANY TO_MATERIALIZED TR_VIEW
    | TR_ALTER TR_ANY TO_MATERIALIZED TR_VIEW
    | TR_DROP TR_ANY TO_MATERIALIZED TR_VIEW
    /* PROJ-1812 ROLE */
    | TR_CREATE TR_ROLE
    | TR_DROP TR_ANY TR_ROLE
    | TR_GRANT TR_ANY TR_ROLE
    | TR_CREATE TR_ANY SES_V_IDENTIFIER
    {
        if(idlOS::strncasecmp("JOB", $<strval>3, 3) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER TR_ANY SES_V_IDENTIFIER
    {
        if(idlOS::strncasecmp("JOB", $<strval>3, 3) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_DROP TR_ANY SES_V_IDENTIFIER
    {
        if(idlOS::strncasecmp("JOB", $<strval>3, 3) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    /* object privileges */
    | TR_ALTER
    | TR_DELETE
    | TR_EXECUTE
    | TO_INDEX
    | TR_INSERT
    | TR_REFERENCES
    | TR_SELECT
    | TR_UPDATE
    | TR_ALL
    | TR_ALL TR_PRIVILEGES
    /* Directories */
    | TR_READ
    | TR_WRITE
    /* PROJ-1812 ROLE */    
    | object_name
    ;

grantees_clause
    : grantees_clause TS_COMMA grantee
    | grantee
    ;

grantee
    : object_name
    | TR_PUBLIC
    ;

opt_with_grant_option
    : /* empty */
    | TR_WITH TR_GRANT APRE_V_OPTION
    ;

revoke_statement
    : revoke_system_privileges_statement
    | revoke_object_privileges_statement
    ;

revoke_system_privileges_statement
    : TR_REVOKE privilege_list
      TR_FROM grantees_clause
    ;

revoke_object_privileges_statement
    : TR_REVOKE privilege_list
      TR_ON user_object_name
      TR_FROM grantees_clause
      opt_cascade_constraints
      opt_force
    | TR_REVOKE privilege_list
      TR_ON TR_DIRECTORY object_name
      TR_FROM grantees_clause
      opt_cascade_constraints
      opt_force
    ;

opt_cascade_constraints
    : /* empty */
    | TR_CASCADE TR_CONSTRAINTS
    ;

opt_force
    : /* empty */
    | SES_V_IDENTIFIER
    {
        if(idlOS::strncasecmp("FORCE", $<strval>1, 5) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    ;

/* Synonym */
create_synonym_statement
    : TR_CREATE
      TR_SYNONYM
      user_object_name
      TR_FOR
      user_object_name
    | TR_CREATE
      TR_PUBLIC
      TR_SYNONYM
      user_object_name
      TR_FOR
      user_object_name
    ;

drop_synonym_statement
    : TR_DROP
      TR_SYNONYM
      user_object_name
    | TR_DROP
      TR_PUBLIC
      TR_SYNONYM
      user_object_name
    ;

/*****************************************
 * REPLICATION
 ****************************************/
replication_statement
    : TR_CREATE opt_repl_mode TA_REPLICATION object_name
        opt_role opt_conflict_resolution opt_repl_options
        TR_WITH replication_with_hosts repl_tbl_commalist
    | TR_ALTER TA_REPLICATION object_name
        TR_ADD TR_TABLE repl_tbl
    | TR_ALTER TA_REPLICATION object_name
        TR_DROP TR_TABLE repl_tbl
    | TR_ALTER TA_REPLICATION object_name
        TR_ADD SES_V_IDENTIFIER replication_hosts
    {
        if(idlOS::strncasecmp("HOST", $<strval>5, 4) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER TA_REPLICATION object_name
        TR_DROP SES_V_IDENTIFIER replication_hosts
    {
        if(idlOS::strncasecmp("HOST", $<strval>5, 4) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER TA_REPLICATION object_name
        TR_SET SES_V_IDENTIFIER replication_hosts
    {
        if(idlOS::strncasecmp("HOST", $<strval>5, 4) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER TA_REPLICATION object_name
        TR_SET TO_MODE repl_mode
    | TR_ALTER TA_REPLICATION object_name
        TR_SET SES_V_IDENTIFIER enable_or_disable
      /* ALTER REPLICATION replication_name SET RECOVERY ENABLE/DISABLE */
      /* ALTER REPLICATION replication_name SET GAPLESS ENABLE/DISABLE */
      /* ALTER REPLICATION replication_name SET GROUPING ENABLE/DISABLE */
      /* ALTER REPLICATION replication_name SET DDL_REPLICATE ENABLE/DISABLE */
    {
        if ( ( idlOS::strncasecmp("RECOVERY", $<strval>5, 8 ) != 0 ) &&
             ( idlOS::strncasecmp("GAPLESS", $<strval>5, 7 ) != 0 ) &&
             ( idlOS::strncasecmp("GROUPING", $<strval>5, 8 ) != 0 ) &&
             ( idlOS::strncasecmp("DDL_REPLICATE", $<strval>5, 13 ) != 0 ) ) // BUG-46525
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER TA_REPLICATION object_name
        TR_SET TR_PARALLEL SES_V_INTEGER
      /* ALTER REPLICATION replication_name SET PARALLEL 'integer_parallel_factor' */
    | TR_ALTER TA_REPLICATION object_name
        TR_SET TA_OFFLINE TR_ENABLE TR_WITH offline_dirs
      /* ALTER REPLICATION replication_name SET OFFLINE ENABLE WITH 'dir','dir' */
    | TR_ALTER TA_REPLICATION object_name
        TR_SET TA_OFFLINE TR_DISABLE
      /* ALTER REPLICATION replication_name SET OFFLINE DISABLE */
    | TR_DROP TA_REPLICATION object_name
    | TR_ALTER TA_REPLICATION object_name TR_START repl_start_option
    | TR_ALTER TA_REPLICATION object_name TR_START TR_WITH TA_OFFLINE
      /* ALTER REPLICATION replication_name START WITH OFFLINE */
    | TR_ALTER TA_REPLICATION object_name SES_V_IDENTIFIER opt_repl_sync_table
      /* ALTER REPLICATION replication_name SYNC */
      /* ALTER REPLICATION replication_name QUICKSTART */
      /* ALTER REPLICATION replication_name STOP */
      /* ALTER REPLICATION replication_name RESET */
      /* ALTER REPLICATION replication_name FAILOVER */    /* PROJ-2747 */
    {
        if(idlOS::strncasecmp("SYNC", $<strval>4, 4) != 0 &&
           idlOS::strncasecmp("QUICKSTART", $<strval>4, 10) != 0 &&
           idlOS::strncasecmp("STOP", $<strval>4, 4) != 0 &&
           idlOS::strncasecmp("RESET", $<strval>4, 5) != 0 &&
           idlOS::strncasecmp("FAILOVER", $<strval>4, 8) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER TA_REPLICATION object_name SES_V_IDENTIFIER repl_sync_retry opt_repl_sync_table
      /* ALTER REPLICATION replication_name SYNC ONLY */
      /* ALTER REPLICATION replication_name QUICKSTART RETRY */
    {
        if(idlOS::strncasecmp("SYNC", $<strval>4, 4) != 0 &&
           idlOS::strncasecmp("QUICKSTART", $<strval>4, 10) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER TA_REPLICATION object_name TR_FLUSH repl_flush_option
      /* ALTER REPLICATION replication_name FLUSH [ALL WAIT second] */
    ;

opt_repl_options
    : /*empty*/
    | SES_V_IDENTIFIER repl_options
    {
        if(idlOS::strncasecmp("OPTIONS", $<strval>1, 7) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    ;

repl_options
    : repl_options repl_option
    | repl_option
    ;

repl_option
    : SES_V_IDENTIFIER
    {
        if ( ( idlOS::strncasecmp("RECOVERY", $<strval>1, 8 ) != 0 ) &&
             ( idlOS::strncasecmp("GAPLESS", $<strval>1, 7 ) != 0 ) &&
             ( idlOS::strncasecmp("GROUPING", $<strval>1, 8 ) != 0 ) &&
             ( idlOS::strncasecmp("DDL_REPLICATE", $<strval>1, 13 ) != 0 ) && // BUG-46525
             /* BUG-46528 Apply __REPLICATION_USE_V6_PROTOCOL to each replication. */
             ( idlOS::strncasecmp("V6_PROTOCOL", $<strval>1, 11 ) != 0 ) )            

        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_PARALLEL SES_V_INTEGER 
    | TA_OFFLINE offline_dirs
    | TR_LOCAL object_name
    ;

offline_dirs
    : offline_dirs TS_COMMA SES_V_LITERAL
    | SES_V_LITERAL
    ;

repl_mode
    : SES_V_IDENTIFIER
    {
        if(idlOS::strncasecmp("LAZY", $<strval>1, 4) != 0 &&
           idlOS::strncasecmp("EAGER", $<strval>1, 5) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    ;

opt_repl_mode
    : /* empty */
    | repl_mode
    ;

replication_with_hosts
    : replication_hosts
    | SES_V_IDENTIFIER
    {
        if(idlOS::strncasecmp("UNIX_DOMAIN", $<strval>1, 11) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    ;

replication_hosts
    : replication_hosts repl_host
    | repl_host
    ;

repl_host
    : SES_V_LITERAL TS_COMMA SES_V_INTEGER opt_using_conntype
    ;

/* BUG-46209 IB support on Replication */
opt_using_conntype
    : /* empty */
    | TR_USING SES_V_IDENTIFIER 
    {
        if( idlOS::strncasecmp("TCP", $<strval>2, 3) != 0 && 
            idlOS::strncasecmp("IB", $<strval>2, 2) != 0 )
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_USING SES_V_IDENTIFIER SES_V_INTEGER    /* USING conn_type ib_latency */
    {
        if( idlOS::strncasecmp("IB", $<strval>2, 2) != 0 ) 
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    ;


opt_role
    : /* empty */
    | TR_FOR TA_ANALYSIS_PROPAGATION
    | TR_FOR SES_V_IDENTIFIER
    {
        if( (idlOS::strncasecmp("ANALYSIS", $<strval>2, 8) != 0) &&
            (idlOS::strncasecmp("PROPAGATION", $<strval>2, 11) != 0) )
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_FOR SES_V_IDENTIFIER TR_LOGGING
    {
        if( idlOS::strncasecmp("PROPAGABLE", $<strval>2, 10) != 0 )
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    ;

opt_conflict_resolution
    : /* empty */
    | TR_AS SES_V_IDENTIFIER
    {
        if(idlOS::strncasecmp("MASTER", $<strval>2, 6) != 0 &&
           idlOS::strncasecmp("SLAVE", $<strval>2, 5) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    ;

repl_tbl_commalist
    : repl_tbl_commalist TS_COMMA repl_tbl
    | repl_tbl
    ;

repl_tbl
    : TR_FROM object_name TS_PERIOD object_name
        TR_TO object_name TS_PERIOD object_name
    | TR_FROM object_name TS_PERIOD object_name TR_PARTITION object_name
        TR_TO object_name TS_PERIOD object_name TR_PARTITION object_name
    | TS_PERIOD
    ;

repl_flush_option
    : /* empty */
    | TO_WAIT SES_V_INTEGER
    | TR_ALL
    | TR_ALL TO_WAIT SES_V_INTEGER
    ;

repl_sync_retry
    : SES_V_ONLY TR_PARALLEL SES_V_INTEGER
    | SES_V_ONLY
    | SES_V_IDENTIFIER
    {
        if(idlOS::strncasecmp("RETRY", $<strval>1, 5) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_PARALLEL SES_V_INTEGER
    ;

opt_repl_sync_table
    :  // empty
    | TR_TABLE repl_sync_table_commalist
    ;

repl_sync_table_commalist
    : repl_sync_table_commalist TS_COMMA repl_sync_table
    | repl_sync_table
    ;

repl_sync_table
    : object_name TS_PERIOD object_name
    | object_name TS_PERIOD object_name TR_PARTITION object_name
    ;

repl_start_option
    : // empty
    | SES_V_IDENTIFIER
    {
        if(idlOS::strncasecmp("RETRY", $<strval>1, 5) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_AT SES_V_IDENTIFIER TS_OPENING_PARENTHESIS SES_V_INTEGER TS_CLOSING_PARENTHESIS
      /* ALTER REPLICATION replicaiton_name START AT SN(sn) */
    {
        if(idlOS::strncasecmp("SN", $<strval>2, 2) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    ;

/*****************************************
 * DDL
 ****************************************/
truncate_table_statement
  : TA_TRUNCATE TR_TABLE user_object_name
  ;

rename_table_statement
  : TO_RENAME user_object_name TR_TO object_name
  ;

flashback_table_statement
  : TO_FLASHBACK TR_TABLE user_object_name TR_TO TR_BEFORE TR_DROP opt_flashback_rename
  ;

opt_flashback_rename
  : /* empty */
  | TO_RENAME TR_TO object_name
  ;

drop_sequence_statement
  : TR_DROP TR_SEQUENCE user_object_name
  ;

drop_index_statement
  : TR_DROP TO_INDEX user_object_name
  ;

drop_table_statement
  : TR_DROP TR_TABLE user_object_name opt_drop_behavior
  ;

purge_table_statement   /* PROJ-2441 flashback */
  : TO_PURGE TR_TABLE user_object_name
  ;

disjoin_table_statement /* PROJ-1810 Partition Exchange */
  : TA_DISJOIN TR_TABLE object_name TS_OPENING_PARENTHESIS disjoin_partitioning_clause TS_CLOSING_PARENTHESIS
  ;

disjoin_partitioning_clause /* PROJ-1810 Partition Exchange */
  : TR_PARTITION object_name TR_TO TR_TABLE object_name TS_COMMA disjoin_partitioning_clause
  | TR_PARTITION object_name TR_TO TR_TABLE object_name
  ;

join_table_statement /* PROJ-1810 Partition Exchange */
  : TA_CONJOIN TR_TABLE object_name join_partitioning_clause opt_row_movement join_table_options opt_lob_attribute_list
  ;

join_partitioning_clause /* PROJ-1810 Partition Exchange */
  : TR_PARTITION TR_BY object_name TS_OPENING_PARENTHESIS column_commalist TS_CLOSING_PARENTHESIS TS_OPENING_PARENTHESIS join_table_attr_list TS_CLOSING_PARENTHESIS
  ;

join_table_attr_list
  : join_table_attr_list TS_COMMA join_table_attr
  | join_table_attr
  ;

join_table_attr
  : TR_TABLE object_name TR_TO TR_PARTITION object_name TR_VALUES TR_LESS TR_THAN TS_OPENING_PARENTHESIS part_key_cond_list TS_CLOSING_PARENTHESIS
  | TR_TABLE object_name TR_TO TR_PARTITION object_name TR_VALUES TS_OPENING_PARENTHESIS part_key_cond_list TS_CLOSING_PARENTHESIS
  | TR_TABLE object_name TR_TO TR_PARTITION object_name TR_VALUES TR_DEFAULT
  ;

join_table_options
  : /* empty */
  | record_access
  ;

opt_drop_behavior
    : /* empty */
    | TR_CASCADE
    | TR_CASCADE TR_CONSTRAINTS
    ;

alter_sequence_statement
  : TR_ALTER TR_SEQUENCE user_object_name sequence_options
  | TR_ALTER TR_SEQUENCE user_object_name sequence_sync_table
  ;

sequence_options
  : sequence_options sequence_option
  | sequence_option
  ;

sequence_option
    : TR_START TR_WITH SES_V_INTEGER
    | TR_START TR_WITH TS_MINUS_SIGN SES_V_INTEGER
    | SES_V_IDENTIFIER TR_BY SES_V_INTEGER
    /* INCREMENT BY integer */
    {
        if(idlOS::strncasecmp("INCREMENT", $<strval>1, 9) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | SES_V_IDENTIFIER TR_BY TS_MINUS_SIGN SES_V_INTEGER
    /* INCREMENT BY - integer */
    {
        if(idlOS::strncasecmp("INCREMENT", $<strval>1, 9) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | SES_V_IDENTIFIER TR_WITH SES_V_INTEGER
    /* RESTART WITH BIGINT */
    {
        /* TASK-7217 Sharded sequence */
        if(idlOS::strncasecmp("RESTART", $<strval>1, 7) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | SES_V_IDENTIFIER TR_WITH TS_MINUS_SIGN SES_V_INTEGER
    /* RESTART WITH - BIGINT */
    {
        /* TASK-7217 Sharded sequence */
        if(idlOS::strncasecmp("RESTART", $<strval>1, 7) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | SES_V_IDENTIFIER SES_V_INTEGER
    /* CACHE integer */
    /* MAXVALUE integer */
    /* MINVALUE integer */
    {
        if(idlOS::strncasecmp("CACHE", $<strval>1, 5) != 0 &&
           idlOS::strncasecmp("MAXVALUE", $<strval>1, 8) != 0 &&
           idlOS::strncasecmp("MINVALUE", $<strval>1, 8) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | SES_V_IDENTIFIER TS_MINUS_SIGN SES_V_INTEGER
    {
        if(idlOS::strncasecmp("MAXVALUE", $<strval>1, 8) != 0 &&
           idlOS::strncasecmp("MINVALUE", $<strval>1, 8) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | SES_V_IDENTIFIER
    /* NOCACHE */
    /* NOMAXVALUE */
    /* NOMINVALUE */
    /* RESTART */
    {
        if(idlOS::strncasecmp("NOCACHE", $<strval>1, 7) != 0 &&
           idlOS::strncasecmp("NOMAXVALUE", $<strval>1, 10) != 0 &&
           idlOS::strncasecmp("NOMINVALUE", $<strval>1, 10) != 0 &&
           idlOS::strncasecmp("RESTART", $<strval>1, 7) != 0)  /* TASK-7217 Sharded sequence */
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_CYCLE
    | TR_NOCYCLE
    | TA_SHARD opt_fixed  /* TASK-7217 Sharded sequence */
    | TR_LOCAL            /* TASK-7217 Sharded sequence */
    ;

opt_fixed
  : /* empty */
  | TA_FIXED
  | TR_VARIABLE
  ;

alter_table_statement
  : TR_ALTER TR_TABLE user_object_name TR_ADD opt_column_tok
      TS_OPENING_PARENTHESIS
      column_def_commalist_or_table_constraint_def
      TS_CLOSING_PARENTHESIS
      opt_lob_attribute_list
      opt_partition_lob_attr_list
  | TR_ALTER TR_TABLE user_object_name TR_ALTER opt_column_tok
      TS_OPENING_PARENTHESIS column_name
      TR_SET TR_DEFAULT arithmetic_expression TS_CLOSING_PARENTHESIS
  | TR_ALTER TR_TABLE user_object_name TR_ALTER opt_column_tok
      TS_OPENING_PARENTHESIS column_name
      TR_DROP TR_DEFAULT TS_CLOSING_PARENTHESIS
  | TR_ALTER TR_TABLE user_object_name TR_ALTER opt_column_tok
      TS_OPENING_PARENTHESIS column_name
      TR_NOT TR_NULL TS_CLOSING_PARENTHESIS
  | TR_ALTER TR_TABLE user_object_name TR_ALTER opt_column_tok
      TS_OPENING_PARENTHESIS column_name
      TR_NULL TS_CLOSING_PARENTHESIS
  | TR_ALTER TR_TABLE user_object_name TR_ALTER opt_column_tok TR_LOB
      TS_OPENING_PARENTHESIS column_commalist TS_CLOSING_PARENTHESIS
      TR_STORE TR_AS TS_OPENING_PARENTHESIS lob_storage_attribute_list
      TS_CLOSING_PARENTHESIS opt_partition_lob_attr_list
  | TR_ALTER TR_TABLE user_object_name TR_ALTER opt_column_tok
      TR_LOB TR_STORE TR_AS TS_OPENING_PARENTHESIS
      lob_storage_attribute_list TS_CLOSING_PARENTHESIS
      opt_partition_lob_attr_list
  | TR_ALTER TR_TABLE user_object_name TR_DROP opt_column_tok column_name
  | TR_ALTER TR_TABLE user_object_name TO_RENAME TR_TO object_name
  | TR_ALTER TR_TABLE user_object_name TA_MAXROWS SES_V_INTEGER
  | TR_ALTER TR_TABLE user_object_name SES_V_IDENTIFIER opt_partition
    /* ALTER TABLE user_object_name COMPACT */
    {
        if ( ( idlOS::strncasecmp("COMPACT", $<strval>4, 7) != 0 ) &&
             ( idlOS::strncasecmp("AGING", $<strval>4, 5) != 0 ) &&
             ( idlOS::strncasecmp("TOUCH", $<strval>4, 5) != 0 ) )
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
  | TR_ALTER TR_TABLE user_object_name TR_ALL TO_INDEX enable_or_disable
    /* ALTER TABLE user_object_name ALL INDEX ENABLE/DISABLE */
  | TR_ALTER TR_TABLE user_object_name
      TO_RENAME TR_COLUMN column_name TR_TO column_name
  /* PROJ-2359 Table/Partition Access Option */
  | TR_ALTER TR_TABLE user_object_name TO_ACCESS record_access
  | TR_ALTER TR_TABLE user_object_name alter_table_partitioning
  /* PROJ-2465 Tablespace Alteration for Table */
  | TR_ALTER TR_TABLE user_object_name TR_ALTER TA_TABLESPACE object_name
    opt_index_storage opt_lob_storage
  /* PROJ-2600 Online DDL for Tablespace Alteration */
  | TR_ALTER TR_TABLE user_object_name TO_REPLACE user_object_name
    opt_partition
    opt_using_prefix
    opt_rename_force
    opt_ignore_foreign_key_child
  | TR_ALTER TR_TABLE user_object_name TR_DROP TR_LOCAL
    TR_UNIQUE  TS_OPENING_PARENTHESIS
      column_with_opt_sort_mode_commalist TS_CLOSING_PARENTHESIS
      opt_cascade_tok
  | TR_ALTER TR_TABLE user_object_name
    TO_RENAME TR_CONSTRAINT object_name TR_TO object_name
  ;

/* PROJ-2600 Online DDL for Tablespace Alteration */
opt_using_prefix
  : // empty
  | using_prefix
  ;

/* PROJ-2600 Online DDL for Tablespace Alteration */
using_prefix
  : TR_USING SES_V_IDENTIFIER object_name
  ;

/* PROJ-2600 Online DDL for Tablespace Alteration */
opt_rename_force
  : // empty
  | TO_RENAME SES_V_IDENTIFIER
  ;

/* PROJ-2600 Online DDL for Tablespace Alteration */
opt_ignore_foreign_key_child
  : // empty
  | SES_V_IDENTIFIER TR_FOREIGN TR_KEY SES_V_IDENTIFIER
  ;

opt_partition
  : /* empty */
  | TR_PARTITION object_name
  ;

alter_table_partitioning
  : TR_ADD add_partition_spec opt_index_part_attr_list
  | TR_COALESCE TR_PARTITION
  | TR_DROP TR_PARTITION object_name
  | TR_MERGE TR_PARTITIONS object_name
    TS_COMMA object_name TR_INTO partition_spec
    opt_index_part_attr_list
  | TO_RENAME TR_PARTITION object_name TR_TO object_name
  /* PROJ-2359 Table/Partition Access Option */
  | TO_ACCESS TR_PARTITION object_name record_access
  | TR_SPLIT TR_PARTITION object_name TR_AT
    TS_OPENING_PARENTHESIS part_key_cond_list TS_CLOSING_PARENTHESIS
    TR_INTO
    TS_OPENING_PARENTHESIS partition_spec opt_index_part_attr_list
    TS_COMMA partition_spec opt_index_part_attr_list
    TS_CLOSING_PARENTHESIS
  | TR_SPLIT TR_PARTITION object_name TR_VALUES
    TS_OPENING_PARENTHESIS part_key_cond_list TS_CLOSING_PARENTHESIS
    TR_INTO
    TS_OPENING_PARENTHESIS partition_spec opt_index_part_attr_list
    TS_COMMA partition_spec opt_index_part_attr_list
    TS_CLOSING_PARENTHESIS
  | TA_TRUNCATE TR_PARTITION object_name
  | TR_ENABLE TR_ROW TR_MOVEMENT
  | TR_DISABLE TR_ROW TR_MOVEMENT
  | TR_ALTER TR_PARTITION object_name TA_TABLESPACE object_name
    opt_index_storage opt_lob_storage
  ;

opt_lob_storage
  :
  | TR_LOB
    TS_OPENING_PARENTHESIS lob_storage_list TS_CLOSING_PARENTHESIS
  ;

lob_storage_list
  : lob_storage_list TS_COMMA lob_storage_element
  | lob_storage_element
  ;

lob_storage_element
  : column_name TA_TABLESPACE object_name
  ;

opt_index_storage
  :
  | TO_INDEX
    TS_OPENING_PARENTHESIS index_storage_list TS_CLOSING_PARENTHESIS
  ;

index_storage_list
  : index_storage_list TS_COMMA index_storage_element
  | index_storage_element
  ;

index_storage_element
  : object_name TA_TABLESPACE object_name
  ;

opt_index_part_attr_list
  :
  | TO_INDEX
    TS_OPENING_PARENTHESIS index_part_attr_list TS_CLOSING_PARENTHESIS
  ;

index_part_attr_list
  : index_part_attr_list TS_COMMA index_part_attr
  | index_part_attr
  ;

index_part_attr
  : object_name TR_PARTITION object_name
  | object_name TR_PARTITION object_name
    TA_TABLESPACE object_name
  ;

  /* BUG-47599 */
add_partition_spec
  : TR_PARTITION object_name opt_table_part_desc
  | TR_PARTITION object_name TR_VALUES TR_LESS TR_THAN
    TS_OPENING_PARENTHESIS part_key_cond_list TS_CLOSING_PARENTHESIS
    opt_table_part_desc
  ;

partition_spec
  : TR_PARTITION object_name opt_table_part_desc
  ;
  
enable_or_disable
  : TR_ENABLE
  | TR_DISABLE
  ;

alter_table_constraint_statement
  : TR_ALTER TR_TABLE user_object_name TR_ADD table_constraint_def
  | TR_ALTER TR_TABLE user_object_name TR_DROP TR_CONSTRAINT object_name
  | TR_ALTER TR_TABLE user_object_name
      TR_DROP TR_PRIMARY TR_KEY opt_cascade_tok
  | TR_ALTER TR_TABLE user_object_name TR_DROP TR_UNIQUE
      TS_OPENING_PARENTHESIS column_with_opt_sort_mode_commalist
      TS_CLOSING_PARENTHESIS
      opt_cascade_tok
  ;

opt_column_tok
  : /* empty */
  | TR_COLUMN
  ;

opt_cascade_tok
  : /* empty */
  | TR_CASCADE
  ;

alter_index_statement
  : TR_ALTER TO_INDEX user_object_name alter_index_clause
  | TR_ALTER TO_INDEX user_object_name TR_SET alter_index_set_clause
  ;

alter_index_clause
  : SES_V_IDENTIFIER
  | TR_REBUILD
  | TR_REBUILD TR_PARTITION object_name
  | TO_RENAME TR_TO user_object_name
  | alter_index_set_clause
  ;

alter_index_set_clause
  : SES_V_IDENTIFIER on_off_clause
  ;

on_off_clause
  : TR_ON
  | TR_OFF
  | TS_EQUAL_SIGN TR_ON
  | TS_EQUAL_SIGN TR_OFF
  ;

create_sequence_statement
  : TR_CREATE TR_SEQUENCE user_object_name opt_sequence_options opt_sequence_sync_table
  ;

opt_sequence_options
  : /* empty */
  | sequence_options
  ;

opt_sequence_sync_table
  : /* empty */
  | sequence_sync_table
  ;

sequence_sync_table
  : enable_or_disable SES_V_IDENTIFIER TR_TABLE
  ;

create_index_statement
  : opt_index_uniqueness
    user_object_name
    TR_ON
    user_object_name
    TS_OPENING_PARENTHESIS
    expression_with_opt_sort_mode_commalist /* PROJ-1090 Function-based Index */
    TS_CLOSING_PARENTHESIS
    opt_index_partitioning_clause
    opt_index_type
    opt_index_pers
    /*opt_index_disable*/ /* 이 구문은 지원하지 않음. */
    opt_index_attributes
  ;

opt_index_uniqueness
  : TR_CREATE TO_INDEX
  | TR_CREATE TR_UNIQUE TO_INDEX
  | TR_CREATE TR_LOCAL TR_UNIQUE TO_INDEX
  ;

opt_index_type
  : /* empty */
  | SES_V_IDENTIFIER TR_IS SES_V_IDENTIFIER
    /* INDEXTYPE IS BTREE OR RTREE */
    {
        if(idlOS::strncasecmp("INDEXTYPE", $<strval>1, 9) != 0 ||
           idlOS::strncasecmp("BTREE", $<strval>3, 5) != 0 &&
           idlOS::strncasecmp("RTREE", $<strval>3, 5) != 0 &&
           // Altibase Spatio-Temporal DBMS
           idlOS::strncasecmp("TDRTREE", $<strval>3, 7) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
  ;

opt_index_pers
  : /* empty */
  | TR_SET SES_V_IDENTIFIER TS_EQUAL_SIGN TR_ON
    {
        if(idlOS::strncasecmp("PERSISTENT", $<strval>2, 10) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
  | TR_SET SES_V_IDENTIFIER TS_EQUAL_SIGN TR_OFF
    {
        if(idlOS::strncasecmp("PERSISTENT", $<strval>2, 10) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
  ;

opt_index_partitioning_clause
  : /* empty */
  | local_partitioned_index
  ;

local_partitioned_index
  : TR_LOCAL
  | TR_LOCAL TS_OPENING_PARENTHESIS on_partitioned_table_list  TS_CLOSING_PARENTHESIS
  ;

on_partitioned_table_list
  : on_partitioned_table_list TS_COMMA on_partitioned_table
  | on_partitioned_table
  ;

on_partitioned_table
  : TR_PARTITION object_name TR_ON object_name
    opt_index_part_desc
  ;

opt_index_part_desc
  :/* empty */
  | TA_TABLESPACE object_name
  ;

constr_using_option
  : /* empty */
  | TR_USING TO_INDEX opt_index_attributes
  ;

opt_index_attributes
  : /* empty */
  | opt_index_attribute_list
  ;

opt_index_attribute_list
  : opt_index_attribute_list opt_index_attribute_element
  | opt_index_attribute_element
  ;

opt_index_attribute_element
  : TA_TABLESPACE object_name
/*| SES_V_LOGGING
  | SES_V_NOLOGGING
*/| TR_PARALLEL SES_V_INTEGER
  | TR_NOPARALLEL
  ;

create_table_statement
  : TR_CREATE TR_TABLE user_object_name
      TS_OPENING_PARENTHESIS table_element_commalist TS_CLOSING_PARENTHESIS
      table_options opt_lob_attribute_list
  | TR_CREATE TR_TABLE user_object_name
      TS_OPENING_PARENTHESIS table_element_commalist TS_CLOSING_PARENTHESIS
      table_options opt_lob_attribute_list
      TR_AS select_or_with_select_statement
  | TR_CREATE TR_TABLE user_object_name table_options opt_lob_attribute_list
      TR_AS select_or_with_select_statement
  /* PROJ-2600 Online DDL for Tablespace Alteration */
  | TR_CREATE TR_TABLE user_object_name
      TR_FROM TR_TABLE SES_V_IDENTIFIER user_object_name
      using_prefix
  ;

table_options
  : /* empty */
  | table_maxrows
  | table_TBS_limit_options
  | table_maxrows table_TBS_limit_options
  | table_partitioning_clause opt_row_movement
  | table_partitioning_clause opt_row_movement table_TBS_limit_options
  /* PROJ-2359 Table/Partition Access Option */
  | record_access
  ;


opt_row_movement
  : /* empty */
  | TR_ENABLE TR_ROW TR_MOVEMENT
  | TR_DISABLE TR_ROW TR_MOVEMENT
  ;


table_partitioning_clause
  : TR_PARTITION TR_BY object_name
   TS_OPENING_PARENTHESIS column_commalist TS_CLOSING_PARENTHESIS
   TS_OPENING_PARENTHESIS part_attr_list TS_CLOSING_PARENTHESIS
  ;

part_attr_list
  : part_attr_list TS_COMMA part_attr
  | part_attr
  ;

part_attr
    /* Range Partition */
  : TR_PARTITION object_name TR_VALUES TR_LESS TR_THAN
    TS_OPENING_PARENTHESIS part_key_cond_list TS_CLOSING_PARENTHESIS
    opt_table_part_desc
  | TR_PARTITION object_name opt_table_part_desc
  | TR_PARTITION object_name TR_VALUES
    TS_OPENING_PARENTHESIS part_key_cond_list TS_CLOSING_PARENTHESIS
    opt_table_part_desc
  | TR_PARTITION object_name TR_VALUES TR_DEFAULT
    opt_table_part_desc
  ;

part_key_cond_list
  : part_key_cond_list TS_COMMA part_key_cond
  | part_key_cond
;

part_key_cond
  : arithmetic_expression
  ;


table_TBS_limit_options
  : table_TBS_limit_options table_TBS_limit_option
  | table_TBS_limit_option
  ;

table_TBS_limit_option
  : TA_TABLESPACE object_name
  | TR_INSERT SES_V_IDENTIFIER TR_LIMIT SES_V_INTEGER
  {
      // if (strMatch : HIGH,2)
      // else if ( strMatch : LOW, 2)
      if(idlOS::strncasecmp("HIGH", $<strval>2, 4) != 0 &&
          idlOS::strncasecmp("LOW", $<strval>2, 3) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  }
  ;

opt_lob_attribute_list
    : /* empty */
    | lob_attribute_list
    ;

lob_attribute_list
    : lob_attribute_list lob_attribute_element
    | lob_attribute_element
    ;

lob_attribute_element
    : TR_LOB TS_OPENING_PARENTHESIS column_commalist TS_CLOSING_PARENTHESIS
        TR_STORE TR_AS
        TS_OPENING_PARENTHESIS lob_storage_attribute_list TS_CLOSING_PARENTHESIS
    | TR_LOB TR_STORE TR_AS
        TS_OPENING_PARENTHESIS lob_storage_attribute_list TS_CLOSING_PARENTHESIS
    ;

lob_storage_attribute_list
    : lob_storage_attribute_list lob_storage_attribute_element
    | lob_storage_attribute_element
    ;

lob_storage_attribute_element
    : TA_TABLESPACE object_name
    | SES_V_IDENTIFIER
    {
        if(idlOS::strncasecmp("LOGGING", $<strval>1, 7) != 0 &&
           idlOS::strncasecmp("NOLOGGING", $<strval>1, 9) != 0 &&
           idlOS::strncasecmp("BUFFER", $<strval>1, 6) != 0 &&
           idlOS::strncasecmp("NOBUFFER", $<strval>1, 8) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    ;

table_element_commalist
  : table_element_commalist TS_COMMA table_element
  | table_element
  ;

table_element
  : table_constraint_def
  | column_def
/*| SES_V_IDENTIFIER opt_default_clause opt_column_constraint_def_list
*/;

table_constraint_def
  : opt_constraint_name TR_UNIQUE
      key_column_with_opt_sort_mode_commalist      // 3
      opt_index_pers                               // 4
      constr_using_option                          // 5
  | opt_constraint_name TR_PRIMARY TR_KEY
      key_column_with_opt_sort_mode_commalist      // 4
      opt_index_pers                               // 5
      constr_using_option                          // 6
  | opt_constraint_name TR_FOREIGN TR_KEY
      TS_OPENING_PARENTHESIS column_commalist TS_CLOSING_PARENTHESIS
      references_specification
  | opt_constraint_name TR_LOCAL TR_UNIQUE
      key_column_with_opt_sort_mode_commalist      // 3
      opt_index_pers                               // 4
      constr_using_option                          // 5
  | opt_constraint_name TR_LOCAL TR_UNIQUE
      opt_sort_mode
      opt_index_pers
      constr_using_option 
  | opt_constraint_name TR_CHECK_OPENING_PARENTHESIS expression TS_CLOSING_PARENTHESIS
  ;

opt_constraint_name
  : /* empty */
  | TR_CONSTRAINT object_name
  ;

column_def_commalist_or_table_constraint_def
  : column_def_commalist
  | table_constraint_def
  ;

column_def_commalist
  : column_def_commalist TS_COMMA column_def
  | column_def
  ;

column_def
  : column_name opt_rule_data_type opt_variable_flag opt_in_row opt_default_clause
      opt_column_constraint_def_list
  ;

opt_variable_flag
  : /* empty */
  | TA_FIXED
  | TR_VARIABLE
  | TR_VARIABLE_LARGE /* BUG-43840 */
  ;

opt_in_row
  : /* empty */
  | TR_IN TR_ROW SES_V_INTEGER
  ;

opt_column_constraint_def_list
  : /* empty */
  | column_constraint_def_list
  ;

column_constraint_def_list
  : column_constraint_def_list column_constraint
  | column_constraint
  ;

column_constraint
  : opt_constraint_name TR_NULL
  | opt_constraint_name TR_NOT TR_NULL
  | opt_constraint_name TR_CHECK_OPENING_PARENTHESIS expression TS_CLOSING_PARENTHESIS
  | opt_constraint_name TR_UNIQUE
      opt_sort_mode                   // 3
      opt_index_pers                  // 4
      constr_using_option             // 5
  | opt_constraint_name TR_PRIMARY TR_KEY
      opt_sort_mode        // 4
      opt_index_pers       // 5
      constr_using_option  // 6
  | opt_constraint_name references_specification
  | opt_constraint_name TR_LOCAL TR_UNIQUE
    opt_sort_mode opt_index_pers constr_using_option 
  ;

opt_default_clause
  : /* empty */
  | TR_DEFAULT arithmetic_expression
  ;

opt_rule_data_type
  : /* empty */
  | rule_data_type opt_encryption_attribute
  ;

rule_data_type
  : object_name
  | object_name TS_OPENING_PARENTHESIS SES_V_INTEGER TS_CLOSING_PARENTHESIS
  | object_name TS_OPENING_PARENTHESIS SES_V_INTEGER TS_COMMA
    SES_V_INTEGER TS_CLOSING_PARENTHESIS
  | object_name TS_OPENING_PARENTHESIS SES_V_INTEGER TS_COMMA
    TS_PLUS_SIGN SES_V_INTEGER TS_CLOSING_PARENTHESIS
  | object_name TS_OPENING_PARENTHESIS SES_V_INTEGER TS_COMMA
    TS_MINUS_SIGN SES_V_INTEGER TS_CLOSING_PARENTHESIS
  ;

opt_encryption_attribute
  : /* empty */
  | encryption_attribute
  ;

encryption_attribute
  : SES_V_IDENTIFIER TR_USING SES_V_LITERAL
  ;

opt_column_commalist
  : /* empty */
  | TS_OPENING_PARENTHESIS column_commalist TS_CLOSING_PARENTHESIS
  ;

key_column_with_opt_sort_mode_commalist
    : TS_OPENING_PARENTHESIS
        column_with_opt_sort_mode_commalist
      TS_CLOSING_PARENTHESIS
    ;

expression_with_opt_sort_mode_commalist
  : expression_with_opt_sort_mode_commalist TS_COMMA expression opt_sort_mode
  | expression opt_sort_mode
  ;

column_with_opt_sort_mode_commalist
  : column_with_opt_sort_mode_commalist TS_COMMA column_name opt_sort_mode
  | column_name opt_sort_mode
  ;

column_commalist
  : column_commalist TS_COMMA column_name
  | column_name
  ;

references_specification
  : TR_REFERENCES user_object_name opt_column_commalist opt_reference_spec
  ;


opt_reference_spec
  : /* empty */
  | TR_ON TR_INSERT referential_action
  | TR_ON TR_UPDATE referential_action
  | TR_ON TR_DELETE referential_action
  | TR_ON TR_DELETE TR_CASCADE
  ;


referential_action
  : SES_V_IDENTIFIER SES_V_IDENTIFIER
  {
      if(idlOS::strncasecmp("NO", $<strval>1, 2) != 0 ||
         idlOS::strncasecmp("ACTION", $<strval>2, 6) != 0)
      {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  }
  | TR_RESTRICT
  ;

table_maxrows
  : TA_MAXROWS SES_V_INTEGER
  ;

opt_table_maxrows
  : /* empty */
  | table_maxrows
  ;

/* Queue */
create_queue_statement
  : TR_CREATE
    TR_QUEUE
    user_object_name
    TS_OPENING_PARENTHESIS
    SES_V_INTEGER
    opt_variable_flag
    opt_in_row
    TS_CLOSING_PARENTHESIS
    opt_table_maxrows
    tablespace_name_option
  | TR_CREATE
    TR_QUEUE
    user_object_name
    TS_OPENING_PARENTHESIS
    column_def_commalist
    TS_CLOSING_PARENTHESIS
    opt_table_maxrows
    tablespace_name_option
  ;

create_view_statement
  : TR_CREATE opt_no_force TR_VIEW user_object_name
      opt_view_column_def
      TR_AS select_or_with_select_statement
      opt_with_read_only
  ;

create_or_replace_view_statement
  : TR_CREATE TR_OR TO_REPLACE opt_no_force TR_VIEW user_object_name
      opt_view_column_def
      TR_AS select_or_with_select_statement
      opt_with_read_only
  ;

opt_no_force
  : /* empty */
  | SES_V_IDENTIFIER SES_V_IDENTIFIER
    {
        if(idlOS::strncasecmp("NO", $<strval>1, 2) != 0 ||
           idlOS::strncasecmp("FORCE", $<strval>2, 5) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
  | SES_V_IDENTIFIER
    {
        if(idlOS::strncasecmp("FORCE", $<strval>1, 5) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
  ;

opt_view_column_def
  : /* empty */
  | TS_OPENING_PARENTHESIS view_element_commalist TS_CLOSING_PARENTHESIS
  ;

view_element_commalist
  : view_element_commalist TS_COMMA view_element
  | view_element
  ;

view_element
  : column_name
  ;

opt_with_read_only
    : /* empty */
    | TR_WITH TR_READ SES_V_ONLY
      /* WITH READ ONLY */
    ;

alter_view_statement
  : TR_ALTER TR_VIEW user_object_name TR_COMPILE
    /* ALTER VIEW [user_name.]view_name COMPILE */
  ;

drop_view_statement
  : TR_DROP TR_VIEW user_object_name
  ;

/* BUG-37100 */
create_database_link_statement
  : TR_CREATE TR_DATABASE TR_LINK object_name TR_USING object_name
  | TR_CREATE link_type_clause TR_DATABASE TR_LINK object_name TR_USING object_name
  | TR_CREATE TR_DATABASE TR_LINK object_name user_clause TR_USING object_name
  | TR_CREATE link_type_clause TR_DATABASE TR_LINK object_name user_clause TR_USING object_name
  ;

/* BUG-37100 */
drop_database_link_statement
  : TR_DROP TR_DATABASE TR_LINK object_name
  | TR_DROP link_type_clause TR_DATABASE TR_LINK object_name
  ;

/* BUG-37100 */
link_type_clause
  : TR_PUBLIC
  | SES_V_IDENTIFIER
  {
      if ( idlOS::strncasecmp( "PRIVATE", $<strval>1, 7 ) != 0 )
      {
          // error 처리
          
          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  }
  ;

/* BUG-37100 */
user_clause
  : TR_CONNECT TR_TO object_name TO_IDENTIFIED TR_BY object_name
  ;

/* BUG-37100 */
alter_database_link_statement
  : TR_ALTER TR_DATABASE TA_LINKER TR_START
  | TR_ALTER TR_DATABASE TA_LINKER SES_V_IDENTIFIER
  {
      if ( idlOS::strncasecmp( "STOP", $<strval>4, 4 ) != 0 )
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }      
  }
  | TR_ALTER TR_DATABASE TA_LINKER SES_V_IDENTIFIER SES_V_IDENTIFIER
  {
      if ( idlOS::strncasecmp( "STOP", $<strval>4, 4 ) != 0 )
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
      if ( idlOS::strncasecmp( "FORCE", $<strval>5, 5 ) != 0 )
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  }
  ;

/* BUG-37100 */
close_database_link_statement
  : TR_ALTER TR_SESSION TR_CLOSE TR_DATABASE TR_LINK TR_ALL
  | TR_ALTER TR_SESSION TR_CLOSE TR_DATABASE TR_LINK object_name
  ;

/* BUG-37100 */
get_default_statement
  : TR_DEFAULT arithmetic_expression
  ;

get_condition_statement
  : TR_RETURN expression
  ;

/* Queue */
/* BUG-45921 */
alter_queue_statement
    : TR_ALTER TR_QUEUE user_object_name SES_V_IDENTIFIER
    {
        if ( idlOS::strncasecmp( "COMPACT", $<strval>4, 7 ) != 0 )
        {
            /* error 처리 */
            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_ALTER TR_QUEUE user_object_name SES_V_IDENTIFIER SES_V_IDENTIFIER
    {
        if ( ( idlOS::strncasecmp( "MSGID", $<strval>4, 5 ) != 0 ) ||
             ( idlOS::strncasecmp( "RESET", $<strval>5, 5 ) != 0 ) )
        {
            /* error 처리 */
            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    ;

drop_queue_statement
  : TR_DROP TR_QUEUE user_object_name
  ;

comment_statement
  : TR_COMMENT TR_ON TR_TABLE user_object_name TR_IS SES_V_LITERAL
  | TR_COMMENT TR_ON TR_COLUMN user_object_column_name TR_IS SES_V_LITERAL
  ;

/*****************************************
 * DML
 ****************************************/
/* PROJ-2204 join update,delete */
dml_table_reference
    : user_object_name opt_partition_name
    | TS_OPENING_PARENTHESIS select_or_with_select_statement TS_CLOSING_PARENTHESIS
    ;

name_list
    : SES_V_IDENTIFIER TS_COMMA SES_V_IDENTIFIER
    | SES_V_IDENTIFIER
    ;

delete_statement
    : TR_DELETE opt_hints dml_table_reference opt_as_name opt_where_clause opt_limit_clause
    | TR_DELETE opt_hints TR_FROM dml_table_reference opt_as_name opt_where_clause opt_limit_clause
    | TR_DELETE opt_hints name_list TR_FROM dml_table_commalist opt_where_clause
    | TR_DELETE opt_hints TR_FROM name_list TR_USING dml_table_commalist opt_where_clause
    ;

insert_statement 
  : TR_INSERT opt_hints TR_INTO dml_table_reference opt_as_name TR_DEFAULT TR_VALUES
  | TR_INSERT opt_hints TR_INTO dml_table_reference opt_as_name
    opt_table_column_commalist TR_VALUES multi_rows_list
  | TR_INSERT opt_hints TR_INTO dml_table_reference opt_as_name
  select_or_with_select_statement
  | TR_INSERT opt_hints TR_INTO dml_table_reference opt_as_name
      TS_OPENING_PARENTHESIS table_column_commalist TS_CLOSING_PARENTHESIS
      select_or_with_select_statement
  | TR_INSERT opt_hints TR_ALL multi_insert_value_list
      select_or_with_select_statement
  ;

multi_insert_value_list
  : multi_insert_value_list multi_insert_value
  | multi_insert_value
  ;

multi_insert_value
  : TR_INTO dml_table_reference opt_table_column_commalist
      TR_VALUES
    TS_OPENING_PARENTHESIS insert_atom_commalist TS_CLOSING_PARENTHESIS
  ;

insert_atom_commalist
  : insert_atom_commalist TS_COMMA insert_atom
  | insert_atom
  ;

insert_atom
  : arithmetic_expression
  | TR_DEFAULT
  ;

multi_rows_list
  : multi_rows_list TS_COMMA one_row
  | one_row

one_row
  : TS_OPENING_PARENTHESIS insert_atom_commalist TS_CLOSING_PARENTHESIS
  ;

dml_table_commalist
  : dml_table_commalist TS_COMMA dml_table
  | dml_table
  ;

dml_table
  : dml_table_reference opt_as_name
  | dml_joined_table
  ;

dml_joined_table
  : dml_table opt_join_type TR_JOIN dml_table TR_ON expression
  ;

update_statement
  : TR_UPDATE opt_hints dml_table_commalist
      TR_SET assignment_commalist
      opt_where_clause
      opt_limit_clause
  ;

enqueue_statement
  : TR_ENQUEUE
    opt_hints
    TR_INTO
    user_object_name
    opt_column_commalist
    TR_VALUES
    TS_OPENING_PARENTHESIS
    insert_atom_commalist
    TS_CLOSING_PARENTHESIS
  ;

dequeue_statement
  : dequeue_query_term
  ;

dequeue_query_term
  : dequeue_query_spec
  ;

dequeue_query_spec
    : TR_DEQUEUE opt_hints target_list
      opt_into_host_var
      dequeue_from_clause
      opt_where_clause
    ;

dequeue_from_clause
  : TR_FROM dequeue_from_table_reference_commalist
  ;

dequeue_from_table_reference_commalist
  : dequeue_from_table_reference
  ;

dequeue_from_table_reference
  : object_name TS_PERIOD object_name
  | object_name
  ;

opt_fifo
  : /* empty */
  | TR_FIFO
  | TR_LIFO
  ;

assignment_commalist
  : assignment_commalist TS_COMMA assignment
  | assignment
  ;

assignment
  : set_column_def TS_EQUAL_SIGN arithmetic_expression
  | set_column_def TS_EQUAL_SIGN TR_DEFAULT
  | TS_OPENING_PARENTHESIS assignment_column_comma_list
    TS_CLOSING_PARENTHESIS TS_EQUAL_SIGN arithmetic_expression
  ;

set_column_def
  : column_name
  | object_name TS_PERIOD column_name
  ;

assignment_column_comma_list
  : assignment_column_comma_list TS_COMMA assignment_column
  | assignment_column
  ;

assignment_column
  : column_name
  | object_name TS_PERIOD object_name
  ;

/*****************************************
 * MERGE
 ****************************************/
merge_statement
  : TR_MERGE opt_hints
    TR_INTO user_object_name opt_partition_name opt_as_name
    TR_USING sel_from_table_reference
    TR_ON expression
    merge_actions_list
  ;

merge_actions_list
  : merge_actions_list merge_actions
  | merge_actions
  ;

merge_actions
  : TR_WHEN when_condition TR_THEN then_action
  ;

when_condition
  : SES_V_IDENTIFIER
  | TR_NOT SES_V_IDENTIFIER
  | SES_V_IDENTIFIER SES_V_IDENTIFIER
  ;

then_action
  : TR_UPDATE TR_SET assignment_commalist opt_where_clause opt_limit_clause
    opt_delete_where_clause
  | TR_INSERT opt_table_column_commalist
    TR_VALUES TS_OPENING_PARENTHESIS insert_atom_commalist TS_CLOSING_PARENTHESIS
    opt_where_clause
  ;

opt_delete_where_clause
  : /* empty */
  | TR_DELETE opt_where_clause opt_limit_clause
  ;

table_column_commalist
  : table_column_commalist TS_COMMA set_column_def
  | set_column_def
  ;

opt_table_column_commalist
  : /* empty */
  | TS_OPENING_PARENTHESIS table_column_commalist TS_CLOSING_PARENTHESIS
  ;

/*****************************************
 * MOVE
 ****************************************/
move_statement
  : TR_MOVE opt_hints TR_INTO user_object_name opt_partition_name
    opt_column_commalist
    TR_FROM user_object_name opt_move_expression_commalist
    opt_where_clause opt_limit_clause
  ;

opt_move_expression_commalist
  : /* empty */
  | TS_OPENING_PARENTHESIS move_expression_commalist TS_CLOSING_PARENTHESIS
  ;

move_expression_commalist
  : move_expression_commalist TS_COMMA move_expression
  | move_expression
  ;

move_expression
  : arithmetic_expression
  | TR_DEFAULT
  ;

/*****************************************
 * SELECT
 ****************************************/
/* BUG-46824 store procedure select into 절 구현 */
SP_select_or_with_select_statement
  : SP_select_statement
  | SP_with_select_statement
  ;
  
SP_select_statement
  : SP_query_exp opt_order_by_clause opt_limit_or_loop_clause
  ;

SP_with_select_statement
  : subquery_factoring_clause SP_query_exp opt_order_by_clause opt_limit_or_loop_clause
  ;

select_or_with_select_statement
  : select_statement
  | with_select_statement
  ;
  
select_statement
  : query_exp opt_order_by_clause opt_limit_or_loop_clause
  ;

with_select_statement
  : subquery_factoring_clause query_exp opt_order_by_clause opt_limit_or_loop_clause
  ;
  
set_op
  : TR_UNION
  | TR_UNION TR_ALL
  | TR_INTERSECT
  | TO_MINUS
  ;

/* BUG-46824 store procedure select into 절 구현 */
SP_query_exp
  : SP_query_exp set_op SP_query_term
  | SP_query_term
  ;

query_exp
  : query_exp set_op query_term
  | query_term
  ;

opt_subquery_factoring_clause
  : /* empty */
  | TR_WITH subquery_factoring_clause_list
  ;    

subquery_factoring_clause
  : TR_WITH subquery_factoring_clause_list
  ;

subquery_factoring_clause_list
  : subquery_factoring_clause_list TS_COMMA subquery_factoring_element
  | subquery_factoring_element
  ;

subquery_factoring_element
  : object_name opt_view_column_def TR_AS TS_OPENING_PARENTHESIS select_statement TS_CLOSING_PARENTHESIS
  ;  
  
/* BUG-46824 store procedure select into 절 구현 */
SP_query_term
  : TS_OPENING_PARENTHESIS SP_query_exp TS_CLOSING_PARENTHESIS
  | SP_query_spec
  ;

query_term
  : TS_OPENING_PARENTHESIS query_exp TS_CLOSING_PARENTHESIS
  | query_spec
  ;

/* BUG-46824 store procedure select into 절 구현 */
SP_query_spec
  : TR_SELECT opt_hints opt_quantifier target_list
      opt_into_list
      from_clause
      opt_where_clause
      opt_hierarchical_query_clause
      opt_groupby_clause
      opt_having_clause
  | TR_SELECT opt_hints opt_quantifier target_list
      SP_into_host_var
      from_clause
      opt_where_clause
      opt_hierarchical_query_clause
      opt_groupby_clause
      opt_having_clause
  ;

query_spec
  : TR_SELECT opt_hints opt_quantifier target_list
      opt_into_list_host_var
      from_clause
      opt_where_clause
      opt_hierarchical_query_clause
      opt_groupby_clause
      opt_having_clause
  ;

select_or_with_select_statement_4emsql
  : select_statement_4emsql
  | with_select_statement_4emsql
  ;
  
select_statement_4emsql
  : query_exp_4emsql opt_order_by_clause opt_limit_or_loop_clause
  ;
  
with_select_statement_4emsql
  : subquery_factoring_clause_4emsql query_exp_4emsql opt_order_by_clause opt_limit_or_loop_clause
  ;
  
  
query_exp_4emsql
  : query_exp_4emsql set_op query_term_4emsql
  | query_term_4emsql
  ;

subquery_factoring_clause_4emsql
  : TR_WITH subquery_factoring_clause_list_4emsql
  ;

subquery_factoring_clause_list_4emsql
  : subquery_factoring_clause_list_4emsql TS_COMMA subquery_factoring_element_4emsql
  | subquery_factoring_element_4emsql
  ;

subquery_factoring_element_4emsql
  : object_name opt_view_column_def TR_AS TS_OPENING_PARENTHESIS select_statement_4emsql TS_CLOSING_PARENTHESIS
  ; 
  
query_term_4emsql
  : TS_OPENING_PARENTHESIS query_exp_4emsql TS_CLOSING_PARENTHESIS
  | query_spec_4emsql
  ;

query_spec_4emsql
  : TR_SELECT opt_hints opt_quantifier target_list
      opt_into_ses_host_var_4emsql
      from_clause
      opt_where_clause
      opt_hierarchical_query_clause
      opt_groupby_clause
      opt_having_clause
  ;

opt_hints
  : /* empty */
  | TX_HINTS
  ;

opt_groupby_clause
  : /* empty */
  | TR_GROUP TR_BY group_concatenation
  ;

opt_quantifier
  : /* empty */
  | TR_ALL
  | TR_DISTINCT
  ;

target_list
  : TS_ASTERISK
  | select_sublist_commalist
  ;

opt_into_list
    : /* empty */
    | TR_INTO SP_variable_name_commalist // for stored procedure
    ;

opt_into_list_host_var
  : /* empty */
  | TR_INTO SP_variable_name_commalist // for stored procedure
  | TR_INTO out_host_var_list
  {
    gUlpCodeGen.ulpGenRemoveQueryToken( $<strval>1 );
  }
  ;

select_sublist_commalist
  : select_sublist_commalist TS_COMMA select_sublist
  | select_sublist
  ;

select_sublist
  : object_name TS_PERIOD TS_ASTERISK
  | object_name TS_PERIOD object_name TS_PERIOD TS_ASTERISK
  | arithmetic_expression opt_as_name
  ;

opt_as_name
  : /* empty */
  | object_name
  | TR_AS column_name
  | SES_V_LITERAL
  | TR_AS SES_V_LITERAL
  ;

from_clause
  : TR_FROM sel_from_table_reference_commalist
  ;

sel_from_table_reference_commalist
  : sel_from_table_reference_commalist TS_COMMA sel_from_table_reference
  | sel_from_table_reference
  ;

sel_from_table_reference
  : object_name TS_PERIOD object_name opt_partition_name opt_pivot_or_unpivot_clause opt_as_name
  | object_name opt_partition_name opt_pivot_or_unpivot_clause opt_as_name
  | TS_OPENING_PARENTHESIS select_or_with_select_statement TS_CLOSING_PARENTHESIS opt_pivot_or_unpivot_clause opt_as_name
  | TA_REMOTE_TABLE TS_OPENING_PARENTHESIS object_name TS_COMMA SES_V_LITERAL TS_CLOSING_PARENTHESIS opt_as_name                              /* BUG-37100 */
  | TA_SHARD TS_OPENING_PARENTHESIS select_or_with_select_statement TS_CLOSING_PARENTHESIS opt_as_name  /* PROJ-2638 */
  | object_name TS_AT_SIGN object_name opt_as_name     /* BUG-37100 */
  | joined_table
  | TR_TABLE TS_OPENING_PARENTHESIS table_func_argument TS_CLOSING_PARENTHESIS opt_as_name
  | dump_object_table
  ;

table_func_argument
  : unified_invocation
  | TO_VC2COLL TS_OPENING_PARENTHESIS list_expression TS_CLOSING_PARENTHESIS
  | SES_V_IDENTIFIER
  | object_name TS_PERIOD object_name TS_PERIOD column_name
  | object_name TS_PERIOD column_name
  ;

opt_pivot_or_unpivot_clause
    : /* empty */
    | pivot_clause
    | unpivot_clause
    ;

pivot_clause
  : TO_PIVOT TS_OPENING_PARENTHESIS pivot_aggregation_list pivot_for pivot_in TS_CLOSING_PARENTHESIS
  ;

pivot_aggregation_list
  : pivot_aggregation_list TS_COMMA pivot_aggregation
  | pivot_aggregation
  ;

pivot_aggregation
  : SES_V_IDENTIFIER TS_OPENING_PARENTHESIS arithmetic_expression TS_CLOSING_PARENTHESIS opt_as_name
  | SES_V_IDENTIFIER TS_OPENING_PARENTHESIS TS_ASTERISK TS_CLOSING_PARENTHESIS opt_as_name
  ;

pivot_for
  : TR_FOR TS_OPENING_PARENTHESIS column_name TS_CLOSING_PARENTHESIS
  | TR_FOR column_name
  ;

pivot_in
  : TR_IN_BF_LPAREN TS_OPENING_PARENTHESIS pivot_in_item_list TS_CLOSING_PARENTHESIS
  ;

pivot_in_item_list
  : pivot_in_item_list TS_COMMA pivot_in_item
  | pivot_in_item
  ;

pivot_in_item
  : constant_plus_minus_prior opt_as_name
  ;

unpivot_clause
    : TO_UNPIVOT opt_include_nulls TS_OPENING_PARENTHESIS unpivot_column TR_FOR unpivot_column unpivot_in TS_CLOSING_PARENTHESIS
    ;

opt_include_nulls
    :  /* empty */
    | SES_V_IDENTIFIER TO_NULLS
    ;

unpivot_column
    : TS_OPENING_PARENTHESIS unpivot_colname_list TS_CLOSING_PARENTHESIS
    | unpivot_colname
    ;

unpivot_colname_list
    : unpivot_colname_list TS_COMMA unpivot_colname
    | unpivot_colname
    ;

unpivot_colname
    : column_name
    ;

unpivot_in
    : TR_IN_BF_LPAREN TS_OPENING_PARENTHESIS unpivot_in_list TS_CLOSING_PARENTHESIS
    ;

unpivot_in_list
    : unpivot_in_list TS_COMMA unpivot_in_info
    | unpivot_in_info
    ;

unpivot_in_info
    : unpivot_in_col_info TR_AS unpivot_in_alias_info
    | unpivot_in_col_info
    ;

unpivot_in_col_info
    : TS_OPENING_PARENTHESIS unpivot_in_col_list TS_CLOSING_PARENTHESIS
    | unpivot_in_column
    ;

unpivot_in_col_list
    : unpivot_in_col_list TS_COMMA unpivot_in_column
    |  unpivot_in_column
    ;

unpivot_in_column
    : column_name
    ;

unpivot_in_alias_info
    : TS_OPENING_PARENTHESIS unpivot_in_alias_list TS_CLOSING_PARENTHESIS
    | unpivot_in_alias
    ;

unpivot_in_alias_list
    : unpivot_in_alias_list TS_COMMA unpivot_in_alias
    | unpivot_in_alias
    ;

unpivot_in_alias
    : constant_arithmetic_expression
    ;

constant_arithmetic_expression
    : constant_concatenation
    ;

constant_concatenation
    : constant_concatenation TS_CONCATENATION_SIGN constant_addition_subtraction
    | constant_addition_subtraction
    ;

constant_addition_subtraction
    : constant_addition_subtraction TS_PLUS_SIGN  constant_multiplication_division
    | constant_addition_subtraction TS_MINUS_SIGN constant_multiplication_division
    | constant_multiplication_division
    ;

constant_multiplication_division
    : constant_multiplication_division TS_ASTERISK constant_plus_minus_prior
    | constant_multiplication_division TS_SLASH constant_plus_minus_prior
    | constant_plus_minus_prior
    ;

constant_plus_minus_prior
  : TS_PLUS_SIGN  constant_terminal_expression
  | TS_MINUS_SIGN constant_terminal_expression
  | constant_terminal_expression
  ;

constant_terminal_expression
  : TR_NULL
  | SES_V_INTEGER
  | SES_V_NUMERIC
  | SES_V_LITERAL
  | SES_V_TYPED_LITERAL
  | TL_NCHAR_LITERAL
  | TL_UNICODE_LITERAL
  ;

dump_object_table
  : object_name TS_OPENING_PARENTHESIS
    dump_object_list TS_CLOSING_PARENTHESIS opt_as_name
  ;

dump_object_list
  : dump_object_list TS_COMMA dump_object
  | dump_object
  ;

dump_object
  : object_name opt_partition_name
  | object_name TS_PERIOD object_name opt_partition_name
  ;


joined_table
  : sel_from_table_reference opt_join_type TR_JOIN sel_from_table_reference
      TR_ON expression
  ;

opt_join_type
  : /* empty */
  | TR_INNER
  | TR_LEFT opt_outer
  | TR_RIGHT opt_outer
  | TR_FULL opt_outer
  ;

opt_outer
  : /* empty */
  | TR_OUTER
  ;

rollup_cube_clause
  : SES_V_ROLLUP
    TS_OPENING_PARENTHESIS rollup_cube_elements TS_CLOSING_PARENTHESIS
  | SES_V_CUBE
    TS_OPENING_PARENTHESIS rollup_cube_elements TS_CLOSING_PARENTHESIS
  ;

rollup_cube_elements
  : rollup_cube_elements TS_COMMA rollup_cube_element
  | rollup_cube_element
  ;

rollup_cube_element
  : arithmetic_expression
  ;

grouping_sets_clause
  : SES_V_GROUPING_SETS
    TS_OPENING_PARENTHESIS grouping_sets_elements TS_CLOSING_PARENTHESIS
  ;

grouping_sets_elements
  : grouping_sets_elements TS_COMMA grouping_sets_element
  | grouping_sets_element
  | grouping_sets_elements TS_COMMA empty_group_operator
  | empty_group_operator
  ;

empty_group_operator
  : TS_OPENING_PARENTHESIS TS_CLOSING_PARENTHESIS
  ;

grouping_sets_element
  : rollup_cube_clause
  | arithmetic_expression
  ;

group_concatenation
  : group_concatenation TS_COMMA group_concatenation_element
  | group_concatenation_element
  ;

group_concatenation_element
  : rollup_cube_clause
  | grouping_sets_clause
  | arithmetic_expression opt_with_rollup
  ;

opt_with_rollup
  : /* empty */
  | SES_V_WITH_ROLLUP
  ;

opt_having_clause
  : /* empty */
  | TR_HAVING expression
  ;

opt_hierarchical_query_clause
    : /* empty */
    | start_with_clause
      connect_by_clause
      opt_ignore_loop_clause
    | connect_by_clause
      opt_ignore_loop_clause
      opt_start_with_clause
    ;

start_with_clause
    : TR_START TR_WITH expression
    ;

opt_start_with_clause
    : /* empty */
    | start_with_clause
    ;

connect_by_clause
    : TR_CONNECT TR_BY expression
    | TO_CONNECT_BY_NOCYCLE expression
    ;

opt_ignore_loop_clause
    : /* empty */
    | SES_V_IDENTIFIER TR_LOOP
    {
        if(idlOS::strncasecmp("IGNORE", $<strval>1, 6) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    ;

opt_order_by_clause
  : /* empty */
  | TR_ORDER TR_BY sort_specification_commalist
  | TR_ORDER SES_V_IDENTIFIER TR_BY sort_specification_commalist
  {
      if ( idlOS::strncasecmp("SIBLINGS", $<strval>2, 8 ) != 0 )
      {
          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
      else
      {
          /* Nothing to do */
      }
  }
  ;

opt_limit_clause
    : /* empty */
    | limit_clause
    ;

limit_clause
    : TR_LIMIT limit_values
    ;

limit_values
    : limit_values TS_COMMA expression
    | limit_values SES_V_IDENTIFIER expression
    {
      if ( idlOS::strncasecmp("OFFSET", $<strval>2, 6 ) != 0 )
      {
          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
      else
      {
          /* Nothing to do */
      }

    }
    /* TASK-7219 */
    | TR_FOR TA_SHARD expression
    | expression
    ;


opt_limit_or_loop_clause
    : /* empty */
    | limit_clause
    | loop_clause
    ;

loop_clause
    : TR_LOOP expression
    ;

opt_for_update_clause
  : /* empty */
  | TR_FOR TR_UPDATE opt_wait_clause
  ;

opt_wait_clause
  : /* EMPTY */
  | SES_V_IDENTIFIER
    /* NOWAIT */
    {
        if(idlOS::strncasecmp("NOWAIT", $<strval>1, 6) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }

    }
  | TO_WAIT SES_V_INTEGER opt_time_unit_expression    /* BUG-45502 */
  ;

/* BUG-45502 */
opt_time_unit_expression
  : /* EMPTY */
  | TA_SEC
  | TA_MSEC
  | TA_USEC
  | TA_SECOND
  | TA_MILLISECOND
  | TA_MICROSECOND
  ;

sort_specification_commalist
  : sort_specification_commalist TS_COMMA sort_specification
  | sort_specification
  ;

sort_specification
  : arithmetic_expression opt_sort_mode opt_nulls_mode
  ;

opt_sort_mode
  : /* empty */
  | TR_ASC
  | TR_DESC
  ;

opt_nulls_mode
  : /* empty */
  | TO_NULLS SES_V_FIRST
  | TO_NULLS SES_V_LAST
  ;

/*****************************************
 * LOCK TABLE
 ****************************************/
lock_table_statement
  : TA_LOCK TR_TABLE object_name opt_partition_name TR_IN table_lock_mode TO_MODE opt_wait_clause opt_until_next_ddl_clause
  | TA_LOCK TR_TABLE object_name TS_PERIOD object_name opt_partition_name TR_IN table_lock_mode TO_MODE opt_wait_clause opt_until_next_ddl_clause
  ;

table_lock_mode
  : TR_ROW SES_V_IDENTIFIER
    /* ROW SHARE     */
    /* ROW EXCLUSIVE */
    {
        if(idlOS::strncasecmp("SHARE", $<strval>2, 5) != 0 &&
           idlOS::strncasecmp("EXCLUSIVE", $<strval>2, 9) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
  | SES_V_IDENTIFIER TR_UPDATE
    /* SHARE UPDATE */
    {
        if(idlOS::strncasecmp("SHARE", $<strval>1, 5) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
  | SES_V_IDENTIFIER TR_ROW SES_V_IDENTIFIER
    /* SHARE ROW EXCLUSIVE */
    {
        if(idlOS::strncasecmp("SHARE", $<strval>1, 5) != 0 ||
           idlOS::strncasecmp("EXCLUSIVE", $<strval>3, 9) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
  | SES_V_IDENTIFIER
    /* SHARE     */
    /* EXCLUSIVE */
    {
        if(idlOS::strncasecmp("SHARE", $<strval>1, 5) != 0 &&
           idlOS::strncasecmp("EXCLUSIVE", $<strval>1, 9) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
  ;

/* BUG-42853 LOCK TABLE에 UNTIL NEXT DDL 기능 추가 */
opt_until_next_ddl_clause
  : /* empty */
  | TR_UNTIL SES_V_NEXT SES_V_IDENTIFIER
    {
        if ( idlOS::strncasecmp( "DDL",  $<strval>3, 3 ) != 0 )
        {
            // error 처리
            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG( &gUlpParseInfo.mErrorMgr ) );
        }
        else
        {
            /* Nothing to do */
        }
    }
  ;

/*****************************************
 * EXPRESSION
 ****************************************/
opt_where_clause
  : /* empty */
  | TR_WHERE expression
  ;

expression
  : logical_or
  ;

logical_or
  : logical_or TR_OR logical_and
  | logical_and
  ;

logical_and
  : logical_and TR_AND logical_not
  | logical_not
  ;

logical_not
  : TR_NOT condition
  | condition
  ;

condition
  : arithmetic_expression TK_BETWEEN
    arithmetic_expression TR_AND arithmetic_expression
  | arithmetic_expression TR_NOT TK_BETWEEN
    arithmetic_expression TR_AND arithmetic_expression
  | arithmetic_expression TR_LIKE
    arithmetic_expression
  | arithmetic_expression TR_NOT TR_LIKE
    arithmetic_expression
  | arithmetic_expression TR_LIKE
    arithmetic_expression TR_ESCAPE arithmetic_expression
  | arithmetic_expression TR_NOT TR_LIKE
    arithmetic_expression TR_ESCAPE arithmetic_expression
  | arithmetic_expression equal_operator
    arithmetic_expression
  | arithmetic_expression not_equal_operator
    arithmetic_expression
  | arithmetic_expression less_than_operator
    arithmetic_expression
  | arithmetic_expression less_equal_operator
    arithmetic_expression
  | arithmetic_expression greater_than_operator
    arithmetic_expression
  | arithmetic_expression greater_equal_operator
    arithmetic_expression
  | arithmetic_expression equal_all_operator quantified_expression
  | arithmetic_expression not_equal_all_operator quantified_expression
  | arithmetic_expression less_than_all_operator quantified_expression
  | arithmetic_expression less_equal_all_operator quantified_expression
  | arithmetic_expression greater_than_all_operator quantified_expression
  | arithmetic_expression greater_equal_all_operator quantified_expression
  | arithmetic_expression equal_any_operator quantified_expression
  | arithmetic_expression not_equal_any_operator quantified_expression
  | arithmetic_expression less_than_any_operator quantified_expression
  | arithmetic_expression less_equal_any_operator quantified_expression
  | arithmetic_expression greater_than_any_operator quantified_expression
  | arithmetic_expression greater_equal_any_operator quantified_expression
  | arithmetic_expression TR_IS TR_NULL
  | arithmetic_expression TR_IS TR_NOT TR_NULL
  | TK_EXISTS subquery
  | TR_UNIQUE subquery
  | cursor_identifier TS_PERCENT_SIGN SES_V_IDENTIFIER
    {
        if(idlOS::strncasecmp("ISOPEN", $<strval>3, 6) != 0 &&
           idlOS::strncasecmp("NOTFOUND", $<strval>3, 8) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
  | cursor_identifier TS_PERCENT_SIGN SES_V_FOUND
  | arithmetic_expression
  ;

equal_operator
  : TS_EQUAL_SIGN
  ;

not_equal_operator
  : TS_LESS_THAN_SIGN TS_GREATER_THAN_SIGN
  | TS_EXCLAMATION_POINT TS_EQUAL_SIGN
  ;

less_than_operator
  : TS_LESS_THAN_SIGN
  ;

less_equal_operator
  : TS_LESS_THAN_SIGN TS_EQUAL_SIGN
  ;

greater_than_operator
  : TS_GREATER_THAN_SIGN
  ;

greater_equal_operator
  : TS_GREATER_THAN_SIGN TS_EQUAL_SIGN
  ;

equal_all_operator
  : TS_EQUAL_SIGN TR_ALL
  ;

not_equal_all_operator
  : TS_LESS_THAN_SIGN TS_GREATER_THAN_SIGN TR_ALL
  | TS_EXCLAMATION_POINT TS_EQUAL_SIGN TR_ALL
  | TR_NOT TR_IN_BF_LPAREN
  ;

less_than_all_operator
  : TS_LESS_THAN_SIGN TR_ALL
  ;

less_equal_all_operator
  : TS_LESS_THAN_SIGN TS_EQUAL_SIGN TR_ALL
  ;

greater_than_all_operator
  : TS_GREATER_THAN_SIGN TR_ALL
  ;

greater_equal_all_operator
  : TS_GREATER_THAN_SIGN TS_EQUAL_SIGN TR_ALL
  ;

equal_any_operator
  : TS_EQUAL_SIGN TR_ANY
  | TS_EQUAL_SIGN TR_SOME
  | TR_IN_BF_LPAREN
  ;

not_equal_any_operator
  : TS_LESS_THAN_SIGN TS_GREATER_THAN_SIGN TR_ANY
  | TS_LESS_THAN_SIGN TS_GREATER_THAN_SIGN TR_SOME
  | TS_EXCLAMATION_POINT TS_EQUAL_SIGN TR_ANY
  | TS_EXCLAMATION_POINT TS_EQUAL_SIGN TR_SOME
  ;

less_than_any_operator
  : TS_LESS_THAN_SIGN TR_ANY
  | TS_LESS_THAN_SIGN TR_SOME
  ;

less_equal_any_operator
  : TS_LESS_THAN_SIGN TS_EQUAL_SIGN TR_ANY
  | TS_LESS_THAN_SIGN TS_EQUAL_SIGN TR_SOME
  ;

greater_than_any_operator
  : TS_GREATER_THAN_SIGN TR_ANY
  | TS_GREATER_THAN_SIGN TR_SOME
  ;

greater_equal_any_operator
  : TS_GREATER_THAN_SIGN TS_EQUAL_SIGN TR_ANY
  | TS_GREATER_THAN_SIGN TS_EQUAL_SIGN TR_SOME
  ;

cursor_identifier
  : object_name
  | object_name TS_PERIOD object_name
  ;

quantified_expression
  : TS_OPENING_PARENTHESIS list_expression TS_CLOSING_PARENTHESIS
  | subquery
  | object_name TS_PERIOD object_name TS_PERIOD column_name /* BUG-43123 */
  | object_name TS_PERIOD column_name
  | column_name
  | host_variable
  | constant_terminal_expression
  ;

arithmetic_expression
  : concatenation
  ;

concatenation
  : concatenation TS_CONCATENATION_SIGN addition_subtraction
  | addition_subtraction
  ;

addition_subtraction
  : addition_subtraction TS_PLUS_SIGN  multiplication_division
  | addition_subtraction TS_MINUS_SIGN multiplication_division
  | multiplication_division
  ;

multiplication_division
  : multiplication_division TS_ASTERISK plus_minus_prior
  | multiplication_division TS_SLASH    plus_minus_prior
  | plus_minus_prior
  ;

plus_minus_prior
  : TS_PLUS_SIGN  terminal_expression
  | TS_MINUS_SIGN terminal_expression
  | TR_PRIOR terminal_expression
  | terminal_expression
  ;

terminal_expression
  : TR_NULL
  | TR_TRUE
  | TR_FALSE
  | TA_SQLCODE
  | TA_SQLERRM
  | cursor_identifier TS_PERCENT_SIGN TA_ROWCOUNT
  | SES_V_INTEGER
  | SES_V_NUMERIC
  | SES_V_LITERAL
  | SES_V_TYPED_LITERAL
  | TS_QUESTION_MARK
  | host_variable
  /* BUG-43123 */
  | object_name TS_PERIOD object_name TS_PERIOD object_name TS_PERIOD column_name opt_outer_join_operator
  | object_name TS_PERIOD object_name TS_PERIOD column_name opt_outer_join_operator
  | object_name TS_PERIOD column_name opt_outer_join_operator
  | column_name opt_outer_join_operator
  | TI_HOSTVARIABLE TS_PERIOD column_name   /* BUG-43123 */
  | TR_LEVEL
  | unified_invocation
  | TS_OPENING_PARENTHESIS list_expression TS_CLOSING_PARENTHESIS
  | subquery
  | SP_arrayIndex_variable_name
  | case_expression
  ;

/* BUG-43123 */
terminal_column
  : object_name TS_PERIOD object_name TS_PERIOD column_name
  | object_name TS_PERIOD column_name
  | column_name
  ;

opt_outer_join_operator
  :
  | TS_OUTER_JOIN_OPERATOR
  ;

case_expression
    // <SIMPLE CASE>
    // select case i1 when 1 then 777
    //                when 2 then 666
    //                when 3 then 555
    //                else 888
    //        end
    // from t1;
    : TR_CASE
      arithmetic_expression
      case_when_value_list
      opt_case_else_clause
      TR_END
    // <SEARCHED CASE>
    // select case when i1=1 then 777
    //             when i1=2 then 666
    //             when i1=3 then 555
    //             else 888
    //        end
    // from t1;
    | TR_CASE
      case_when_condition_list
      opt_case_else_clause
      TR_END
    ;

case_when_value_list
    : case_when_value_list case_when_value
    | case_when_value
    ;

case_when_value
    : TR_WHEN
      arithmetic_expression
      case_then_value
    ;

case_then_value
    : TR_THEN
      arithmetic_expression
    ;

opt_case_else_clause
    : // EMPTY
    | TR_ELSE
      arithmetic_expression
    ;

case_when_condition_list
    : case_when_condition_list case_when_condition
    | case_when_condition
    ;

case_when_condition
    : TR_WHEN
      expression
      case_then_value
    ;

/* unified invocation rule for stored procedure and SQL statement. */
/* BUG-30096 : parser에 분석함수 구문 (OVER) 누락되어있음.  */
unified_invocation
  : object_name
      TS_OPENING_PARENTHESIS list_expression TS_CLOSING_PARENTHESIS
      opt_within_group_clause
      opt_keep_clause
      over_clause
  | object_name TS_PERIOD memberfunc_name
      TS_OPENING_PARENTHESIS list_expression TS_CLOSING_PARENTHESIS
  | object_name
      TS_OPENING_PARENTHESIS TS_CLOSING_PARENTHESIS
      over_clause
  | object_name TS_PERIOD memberfunc_name
      TS_OPENING_PARENTHESIS TS_CLOSING_PARENTHESIS
  | object_name TS_OPENING_PARENTHESIS
    TS_ASTERISK TS_CLOSING_PARENTHESIS
    opt_keep_clause
    over_clause
  | object_name TS_OPENING_PARENTHESIS
    TR_ALL TS_ASTERISK TS_CLOSING_PARENTHESIS
    opt_keep_clause
    over_clause
  | object_name TS_OPENING_PARENTHESIS
    TR_ALL list_expression TS_CLOSING_PARENTHESIS
    opt_keep_clause
    over_clause
  | object_name TS_OPENING_PARENTHESIS
    TR_DISTINCT list_expression TS_CLOSING_PARENTHESIS
    over_clause
  | object_name TS_PERIOD object_name TS_PERIOD memberfunc_name
    TS_OPENING_PARENTHESIS list_expression TS_CLOSING_PARENTHESIS
  | object_name TS_PERIOD object_name TS_PERIOD memberfunc_name
    TS_OPENING_PARENTHESIS TS_CLOSING_PARENTHESIS
  | keyword_function_name
      TS_OPENING_PARENTHESIS list_expression TS_CLOSING_PARENTHESIS
  | keyword_function_name
      TS_OPENING_PARENTHESIS TS_CLOSING_PARENTHESIS
  | TR_CAST TS_OPENING_PARENTHESIS expression TR_AS rule_data_type
      TS_CLOSING_PARENTHESIS
  | TO_CONNECT_BY_ROOT terminal_column      /* BUG-43123 */
  ;

// PROJ-2527 WITHIN GROUP AGGR
opt_within_group_clause
    : /* empty */
    | TR_WITHIN
      TR_GROUP
      TS_OPENING_PARENTHESIS
      TR_ORDER TR_BY within_group_order_by_column_list
      TS_CLOSING_PARENTHESIS
    ;

within_group_order_by_column_list
    : within_group_order_by_column_list TS_COMMA within_group_order_by_column
    | within_group_order_by_column
    ;

within_group_order_by_column
    : arithmetic_expression
    | arithmetic_expression TR_ASC
    | arithmetic_expression TR_DESC
    ;

/* BUG-30096 : parser에 분석함수 구문 (OVER) 누락되어있음.  */
over_clause
    : /* empty */
    | TR_OVER
      TS_OPENING_PARENTHESIS // (
      opt_over_partition_by_clause
      opt_over_order_by_clause
      opt_window_clause
      TS_CLOSING_PARENTHESIS // )
    ;

opt_over_partition_by_clause
    : /* empty */
    | TR_PARTITION TR_BY partition_by_column_list
    ;

/* BUG-30096 : parser에 분석함수 구문 (OVER) 누락되어있음.  */
partition_by_column_list
    : partition_by_column_list TS_COMMA partition_by_column
    | partition_by_column
    ;

/* BUG-30096 : parser에 분석함수 구문 (OVER) 누락되어있음.  */
partition_by_column
    : arithmetic_expression
    ;

opt_over_order_by_clause
    : /* empty */
    | TR_ORDER TR_BY sort_specification_commalist
    ;

opt_window_clause
    : /* empty */
    | SES_V_IDENTIFIER TK_BETWEEN windowing_start_clause TR_AND windowing_end_clause
    | SES_V_IDENTIFIER windowing_start_clause
    ;

windowing_start_clause
    : SES_V_IDENTIFIER TR_PRECEDING
    | TR_CURRENT_ROW
    | window_value TR_PRECEDING
    | window_value TR_FOLLOWING
    ;

windowing_end_clause
    : SES_V_IDENTIFIER TR_FOLLOWING
    | TR_CURRENT_ROW
    | window_value TR_PRECEDING
    | window_value TR_FOLLOWING
    ;

window_value
    : SES_V_INTEGER
    | SES_V_IDENTIFIER SES_V_INTEGER SES_V_IDENTIFIER
    ;

list_expression
  : list_expression TS_COMMA expression
  | expression
  ;

subquery
  : TS_OPENING_PARENTHESIS opt_subquery_factoring_clause subquery_exp opt_limit_or_loop_clause TS_CLOSING_PARENTHESIS
  ;

subquery_exp
  : subquery_exp set_op subquery_term
  | subquery_term
  ;

subquery_term
  : query_spec
  ;

opt_keep_clause
  : /* empty */
  | TO_KEEP
    TS_OPENING_PARENTHESIS
    SES_V_IDENTIFIER
    keep_option
    TR_ORDER TR_BY
    sort_specification_commalist
    TS_CLOSING_PARENTHESIS
  ;

keep_option
  : SES_V_FIRST
  | SES_V_LAST
  ;

//*************************
// PROCEDURE and FUNCTION
//*************************
SP_create_or_replace_function_statement
    : create_or_replace_function_clause               // 1
          user_object_name                            // 2
          SP_parameter_declaration_commalist_option   // 3
      TR_RETURN                                       // 4
          SP_data_type                                // 5
      SP_as_o_is                                      // 6
          SP_first_block                              // 7
      SP_name_option                                  // 8
    ;

SP_create_or_replace_procedure_statement
    : create_or_replace_procedure_clause              // 1
          user_object_name                            // 2
          SP_parameter_declaration_commalist_option   // 3
      SP_as_o_is                                      // 4
          SP_first_block                              // 5
      SP_name_option                                  // 6
    ;

/* TYPESET */
SP_create_or_replace_typeset_statement
    : create_or_replace_typeset_clause                // 1
          user_object_name                            // 2
      SP_as_o_is                                      // 3
          SP_typeset_block                            // 4
      SP_name_option                                  // 5
    ;

//*********************************************
// COMMON ELEMENTS FOR PROCEDURE, FUNCTION and TYPESET
//*********************************************
create_or_replace_function_clause
    : TR_CREATE TR_FUNCTION
    | TR_CREATE TR_OR TO_REPLACE TR_FUNCTION
    ;

create_or_replace_procedure_clause
    : TR_CREATE TR_PROCEDURE
    | TR_CREATE TR_OR TO_REPLACE TR_PROCEDURE
    ;

create_or_replace_typeset_clause
    : TR_CREATE TR_TYPESET
    | TR_CREATE TR_OR TO_REPLACE TR_TYPESET
    ;

SP_as_o_is
    : TR_AS
    | TR_IS
    ;

/******************************************
 * parameter_declaration_commalist
 ******************************************/
SP_parameter_declaration_commalist_option
    : /* empty */
    | TS_OPENING_PARENTHESIS TS_CLOSING_PARENTHESIS
    | TS_OPENING_PARENTHESIS
          SP_parameter_declaration_commalist
      TS_CLOSING_PARENTHESIS
    ;

SP_parameter_declaration_commalist
    : SP_parameter_declaration_commalist TS_COMMA SP_parameter_declaration
    | SP_parameter_declaration
    ;

SP_parameter_declaration
    : object_name                        // 1
      SP_parameter_access_mode_option    // 2
      SP_data_type                       // 3
      SP_assign_default_value_option     // 4
    ;

SP_parameter_access_mode_option
    : /* empty */
    | TR_IN
    | TR_OUT
    | TR_IN TR_OUT
    ;

/******************************************
 * trivial nonterminals
 ******************************************/
SP_name_option
    : /* empty */
    | object_name
    ;

SP_assign_default_value_option
    : /* empty */
    | TS_COLON TS_EQUAL_SIGN SP_unified_expression
    | TR_DEFAULT SP_unified_expression
    ;

/******************************************
 * SP_expression, SP_boolean_expression
 ******************************************/
SP_arithmetic_expression
    : arithmetic_expression
    ;

SP_boolean_expression
    : expression
    ;

/* both arithmetic and boolean expression. */
SP_unified_expression
    : expression
    ;

/******************************************
 * SP_function_opt_arglist
 ******************************************/
SP_function_opt_arglist
    : object_name
    | object_name TS_PERIOD object_name
    | unified_invocation
    ;

/******************************************
 * SP_ident_opt_arglist
 ******************************************/
SP_ident_opt_arglist
    : object_name
    | object_name TS_OPENING_PARENTHESIS TS_CLOSING_PARENTHESIS
    | object_name TS_OPENING_PARENTHESIS list_expression TS_CLOSING_PARENTHESIS
    ;

/******************************************
 * SP_variable_name_commalist
 ******************************************/
SP_variable_name_commalist
    : SP_variable_name_commalist TS_COMMA SP_variable_name
    | SP_variable_name
    ;

SP_arrayIndex_variable_name
    : object_name      // [1] arrayVar_name
      TS_OPENING_BRACKET // [2] [
      SP_unified_expression // [3] index
      TS_CLOSING_BRACKET // [4] ]
    | object_name      // [1] label_name
      TS_PERIOD          // [2] .
      object_name      // [3] arrayVar_name
      TS_OPENING_BRACKET // [4] [
      SP_unified_expression // [5] index
      TS_CLOSING_BRACKET // [6] ]
    | object_name      // [1] arrayVar_name
      TS_OPENING_BRACKET // [2] [
      SP_unified_expression // [3] index
      TS_CLOSING_BRACKET // [4] ]
      TS_PERIOD          // [5] .
      column_name           // [6] col_name
    | object_name      // [1] label_name
      TS_PERIOD          // [2] .
      object_name      // [3] arrayVar_name
      TS_OPENING_BRACKET // [4] [
      SP_unified_expression // [5] index
      TS_CLOSING_BRACKET // [6] ]
      TS_PERIOD          // [7] .
      column_name           // [8] col_name
    ;

SP_variable_name
    : object_name
    | object_name TS_PERIOD column_name
    | object_name    // 1
          TS_PERIOD    // 2
      object_name    // 3
          TS_PERIOD    // 4
      column_name         // 5
    | SP_arrayIndex_variable_name
    ;

SP_counter_name
    : object_name
    ;

/******************************************
 * SP_data_type
 ******************************************/
SP_data_type
    : column_name               // 1.. T1%ROWTYPE
          TS_PERCENT_SIGN    // 2
      TO_ROWTYPE             // 3
    | object_name          // 1 SYS.T1%ROWTYPE
          TS_PERIOD          // 2
      column_name               // 3
          TS_PERCENT_SIGN    // 4
      TO_ROWTYPE             // 5
    | column_name               // NAME : 1
          TS_PERCENT_SIGN    // %    : 2
      TR_TYPE                // TYPE : 3
    | object_name          // NAME : 1
          TS_PERIOD          // .    : 2
      column_name               // NAME : 3
          TS_PERCENT_SIGN    // %    : 4
      TR_TYPE                // TYPE : 5
    | object_name          // NAME : 1
          TS_PERIOD          // .    : 2
      object_name          // NAME : 3
          TS_PERIOD          // .    : 4
      column_name               // NAME : 5
          TS_PERCENT_SIGN    // %    : 6
      TR_TYPE                // TYPE : 7
    | SP_rule_data_type
    | object_name // primitive type또는 user defined type
    | object_name // typeset_name or label_name[1]
          TS_PERIOD // .[2]
      object_name // type_name[3]
    | object_name // user_name[1]
          TS_PERIOD // .[2]
      object_name // typeset_name[3]
          TS_PERIOD // .[4]
      object_name // type_name[5]
    ;

/*
 * stored procedure내에서는 udt도 허용하므로,
 * 반드시 primitive type이 될 구문만 SP_rule_data_type에 묶는다. */
SP_rule_data_type
    : object_name TS_OPENING_PARENTHESIS SES_V_INTEGER TS_CLOSING_PARENTHESIS
    | object_name TS_OPENING_PARENTHESIS SES_V_INTEGER TS_COMMA
      SES_V_INTEGER TS_CLOSING_PARENTHESIS
    | object_name TS_OPENING_PARENTHESIS SES_V_INTEGER TS_COMMA
      TS_PLUS_SIGN SES_V_INTEGER TS_CLOSING_PARENTHESIS
    | object_name TS_OPENING_PARENTHESIS SES_V_INTEGER TS_COMMA
      TS_MINUS_SIGN SES_V_INTEGER TS_CLOSING_PARENTHESIS
    ;

//*****************************************
//* SP_block
//*****************************************
SP_block
    : TR_DECLARE                             // 1
          SP_item_declaration_list_option       // 2
      TR_BEGIN                               // 3
          SP_statement_list                     // 4
          SP_exception_block_option             // 5
      TR_END                                 // 6
          SP_name_option                        // 7
      TS_SEMICOLON                           // 8
    | TR_BEGIN                               // 9
          SP_statement_list                     // 10
          SP_exception_block_option             // 11
      TR_END                                 // 12
          SP_name_option                        // 13
      TS_SEMICOLON                           // 14
    ;

SP_first_block
    :     SP_item_declaration_list_option       // 1
      TR_BEGIN                               // 2
          SP_statement_list                     // 3
          SP_exception_block_option             // 4
      TR_END                                 // 5
    ;

/* PROJ-1075
 * typeset block에서는 type declaration만 가능. */
SP_typeset_block
    : SP_type_declaration_list                  // 1
      TR_END                                 // 2
    ;

//*****************************************
//* SP_item_declaration_list
//*****************************************
SP_item_declaration_list_option
    : /* empty */
    | SP_item_declaration_list
    ;

SP_item_declaration_list
    : SP_item_declaration_list SP_item_declaration
    | SP_item_declaration
    ;

SP_item_declaration
    : SP_cursor_declaration
    | SP_exception_declaration
    | SP_variable_declaration
    | SP_type_declaration
    ;

//***************************************************
//* SP_type_declaration_list
//***************************************************
SP_type_declaration_list
    : SP_type_declaration_list SP_type_declaration
    | SP_type_declaration
    ;

//************************************************
//* SP_item_declaration -> SP_cursor_declaration
//************************************************
SP_cursor_declaration
    : TR_CURSOR                                            // 1
          object_name                                      // 2
          SP_parameter_declaration_commalist_option        // 3
      TR_IS                                                // 4
          select_or_with_select_statement                  // 5
          opt_for_update_clause                            // 6
      TS_SEMICOLON                                         // 7
    ;

//***************************************************
//* SP_item_declaration -> SP_exception_declaration
//***************************************************
SP_exception_declaration
    : object_name TR_EXCEPTION TS_SEMICOLON
    ;

//***************************************************
//* SP_item_declaration -> SP_variable_declaration
//***************************************************
SP_variable_declaration
    : object_name                                      // 1
      SP_constant_option                               // 2
      SP_data_type                                     // 3
      SP_assign_default_value_option                   // 4
      TS_SEMICOLON                                     // 5
    ;

SP_constant_option
    : /* empty */
    | TO_CONSTANT
    ;

//***************************************************
//* SP_item_declaration -> SP_type_declaration
//***************************************************
SP_type_declaration
    : TR_TYPE                   // TYPE[1]
      object_name             // type_name[2]
      TR_IS SES_V_IDENTIFIER    // IS[3] RECORD[4]
      TS_OPENING_PARENTHESIS    // ([5]
      record_elem_commalist        // record_elem_commalist[6]
      TS_CLOSING_PARENTHESIS    // )[7]
      TS_SEMICOLON              // ;[8]
    {
        if(idlOS::strncasecmp("RECORD", $<strval>4, 6) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    | TR_TYPE                       // TYPE[1]
      object_name              // type_name[2]
      TR_IS TR_TABLE TR_OF          // IS[3] TABLE[4] OF[5]
      SP_array_element              // data_type[6]
      SP_opt_index_by_clause        // index_column[7]
      TS_SEMICOLON                  // ;[8]
    ;

SP_array_element
    : SP_rule_data_type
    | object_name
    | object_name TS_PERIOD object_name
    | object_name TS_PERIOD object_name TS_PERIOD object_name
    ;

SP_opt_index_by_clause
    : TO_INDEX TR_BY rule_data_type
    ;

record_elem_commalist
    : record_elem_commalist TS_COMMA record_elem
    | record_elem
    ;

record_elem
    : column_name rule_data_type
    ;

//*****************************************
//* SP_exception_block
//*****************************************
SP_exception_block_option
    : /* empty */
    | SP_exception_block
    {
        /* BUG-46824 psm execute 여부를 확인하기 위하여 
         * begin ~~ end 사이의 모든 객체를 counting해야한다. */
        gUlpProcObjCount++;
    }
    ;

SP_exception_block
    : TR_EXCEPTION SP_exception_handler_list_option
    ;

SP_exception_handler_list_option
    : /* empty */
    | SP_exception_handler_list
    ;

SP_exception_handler_list
    : SP_exception_handler_list SP_exception_handler
    | SP_exception_handler
    ;

SP_exception_handler
    : TR_WHEN                          // 1
          SP_exception_name_or_list       // 2
      SP_then_statement                   // 3
    | TR_WHEN                          // 1
          TO_OTHERS                    // 2
      SP_then_statement                   // 3
    ;

SP_exception_name_or_list
    : SP_exception_name TR_OR SP_exception_name
    | SP_exception_name
    ;

SP_exception_name
    : object_name
    | object_name TS_PERIOD object_name
    ;

//*****************************************
//* SP_statement
//*****************************************
SP_statement_list
    : SP_statement_list SP_statement
    {
        /* BUG-46824 psm execute 여부를 확인하기 위하여 
         * begin ~~ end 사이의 모든 객체를 counting해야한다. */
        gUlpProcObjCount++;
    }
    | SP_statement
    {
        /* BUG-46824 psm execute 여부를 확인하기 위하여 
         * begin ~~ end 사이의 모든 객체를 counting해야한다. */
        gUlpProcObjCount++;
    }
    ;

SP_statement
    : SP_label_statement
    | SP_assignment_statement
    | SP_close_statement
    | SP_exit_statement
    | SP_fetch_statement
    | SP_goto_statement
    | SP_if_statement
    | SP_case_statement
    | SP_loop_statement
    | SP_null_statement
    | SP_open_statement
    | SP_block
    | SP_raise_statement
    | SP_return_statement
    | SP_continue_statement
    | SP_sql_statement
    ;

SP_label_statement
    : TS_LESS_THAN_SIGN     // 1
      TS_LESS_THAN_SIGN     // 2
          object_name     // 3
      TS_GREATER_THAN_SIGN  // 4
      TS_GREATER_THAN_SIGN  // 5
    ;

SP_sql_statement
    : SP_select_or_with_select_statement opt_for_update_clause TS_SEMICOLON
    | insert_statement TS_SEMICOLON
    | update_statement TS_SEMICOLON
    | delete_statement TS_SEMICOLON
    | merge_statement TS_SEMICOLON
    | move_statement TS_SEMICOLON
    | enqueue_statement TS_SEMICOLON
    | set_transaction_statement TS_SEMICOLON
    | savepoint_statement TS_SEMICOLON
    | commit_statement TS_SEMICOLON
    | rollback_statement TS_SEMICOLON
    | SP_invocation_statement TS_SEMICOLON
    {
        if (gUlpPSMObjName == NULL)
        {
            /* BUG-46824 begin ~~ end 사이의 첫번째 object_name을 저장. */ 
            /* BUG-47868 gUlpPSMObjName는 ulpGetQueryBuf동일한 memory를 참조해야 한다. */
            gUlpPSMObjName = idlOS::strstr(gUlpCodeGen.ulpGetQueryBuf(), $<strval>1);
        }
    }
    ;

SP_invocation_statement
    /* BUG-46824 USER.PACKAGE.PROCEDURE도 호출 가능하도록 수정 */ 
    : SP_ident_opt_simple_arglist
    ;

//*****************************************
//* SP_statement -> SP_assignment_statement
//*****************************************
SP_assignment_statement
    : SP_variable_name            // 1
          TS_COLON             // 2
          TS_EQUAL_SIGN        // 3
      SP_unified_expression       // 4
          TS_SEMICOLON         // 5
    | TR_SET                   // 1
      SP_variable_name            // 2
          TS_EQUAL_SIGN        // 3
      SP_unified_expression       // 4
          TS_SEMICOLON         // 5
    ;

//*****************************************
//* SP_statement -> SP_fetch_statement
//*****************************************
SP_fetch_statement
    : TR_FETCH                          // 1
          object_name                 // 2
      TR_INTO                           // 3
          SP_variable_name_commalist       // 4
      TS_SEMICOLON                      // 5
    | TR_FETCH                          // 1
          object_name                 // 2
      TS_PERIOD                         // 3
          object_name                 // 4
      TR_INTO                           // 5
          SP_variable_name_commalist       // 6
      TS_SEMICOLON                      // 7
    ;

//*****************************************
//* SP_statement -> SP_if_statement
//*****************************************
SP_if_statement
    : TR_IF                              // 1
          SP_boolean_expression             // 2
      SP_then_statement                     // 3
      SP_else_option                        // 4
      TR_END TR_IF TS_SEMICOLON    // 5 6 7
    ;

SP_else_option
    : /* empty */
    | TR_ELSE SP_statement_list
    | SP_else_if
    ;

SP_else_if
    : TA_ELSIF                      // 1
          SP_boolean_expression        // 2
      SP_then_statement                // 3
      SP_else_option                   // 4
    | TR_ELSEIF                     // 1
          SP_boolean_expression        // 2
      SP_then_statement                // 3
      SP_else_option                   // 4
    ;

SP_then_statement
    : TR_THEN
      SP_statement_list
    ;

//*****************************************
//* SP_statement -> SP_case_statement
//*****************************************
SP_case_statement
    : TR_CASE                                      // 1
          SP_case_when_condition_list                 // 2
      SP_case_else_option                             // 3
      TR_END TR_CASE TS_SEMICOLON            // 4 5 6
    | TR_CASE                                      // 1
          SP_arithmetic_expression                    // 2
          SP_case_when_value_list                     // 3
      SP_case_else_option                             // 4
      TR_END TR_CASE TS_SEMICOLON            // 5 6 7
    ;

SP_case_when_condition_list
    : SP_case_when_condition SP_case_when_condition_list
    | SP_case_when_condition
    ;

SP_case_when_condition
    : TR_WHEN                       // 1
          SP_boolean_expression        // 2
      SP_then_statement                // 3
    ;

SP_case_when_value_list
    : SP_case_when_value SP_case_when_value_list
    | SP_case_when_value
    ;

SP_case_when_value
    : TR_WHEN                       // 1
          SP_case_right_operand        // 2
      SP_then_statement                // 3
    ;

SP_case_right_operand
    : SP_arithmetic_expression
    ;

SP_case_else_option
    : /* empty */
    | TR_ELSE SP_statement_list
    ;

//*****************************************
//* SP_statement -> SP_loop_statement
//*****************************************
SP_loop_statement
    : SP_basic_loop_statement
    | SP_while_loop_statement
    | SP_for_loop_statement
    | SP_cursor_for_loop_statement
    ;

SP_common_loop
    : TR_LOOP                    // 1
          SP_statement_list         // 2
      TR_END                     // 3
      TR_LOOP                    // 4
    ;

//*****************************************
//* SP_statement -> SP_loop_statement -> SP_while_loop_statement
//*****************************************
SP_while_loop_statement
    : TR_WHILE                   // 1
          SP_boolean_expression     // 2
      SP_common_loop                // 3
      SP_name_option                // 4
      TS_SEMICOLON               // 5
    ;

//*****************************************
//* SP_statement -> SP_loop_statement -> SP_basic_loop_statement
//*****************************************
SP_basic_loop_statement
    : SP_common_loop SP_name_option TS_SEMICOLON
    ;

//*****************************************
//* SP_statement -> SP_loop_statement -> SP_for_loop_statement
//*****************************************
SP_for_loop_statement
    : TR_FOR                          // 1
          SP_counter_name                // 2
      SES_V_IN                           // 3
          SP_arithmetic_expression       // 4
      TS_DOUBLE_PERIOD                // 5
          SP_arithmetic_expression       // 6
      SP_step_option                     // 7
      SP_common_loop                     // 8
      SP_name_option                     // 9
      TS_SEMICOLON                    // 10
    | TR_FOR                          // 1
          SP_counter_name                // 2
      TR_IN             // 3
          TA_REVERSE                  // 4
          SP_arithmetic_expression       // 5
      TS_DOUBLE_PERIOD                // 6
          SP_arithmetic_expression       // 7
      SP_step_option                     // 8
      SP_common_loop                     // 9
      SP_name_option                     // 10
      TS_SEMICOLON                    // 11
    ;

SP_step_option
    : /* empty */
    | TA_STEP SP_arithmetic_expression
    ;

//*****************************************
//* SP_statement -> SP_loop_statement -> SP_cursor_for_loop_statement
//*****************************************
SP_cursor_for_loop_statement
    : TR_FOR                            // 1
          SP_counter_name                  // 2
      SES_V_IN                             // 3
          SP_ident_opt_arglist             // 4
      SP_common_loop                       // 5
      SP_name_option                       // 6
      TS_SEMICOLON                      // 7
    ;

//*****************************************
//* simple nonterminals for SP_statement
//*****************************************
SP_close_statement
    : TR_CLOSE object_name TS_SEMICOLON
    | TR_CLOSE             // 1
          object_name    // 2
      TS_PERIOD            // 3
          object_name    // 4
      TS_SEMICOLON         // 5
    ;

SP_exit_statement
    : TR_EXIT               // 1
      SP_name_option           // 2
      SP_exit_when_option      // 3
      TS_SEMICOLON          // 4
    ;

SP_exit_when_option
    : /* empty */
    | TR_WHEN SP_boolean_expression
    ;

SP_goto_statement
    : TR_GOTO object_name TS_SEMICOLON
    ;

SP_null_statement
    : TR_NULL TS_SEMICOLON
    ;

SP_open_statement
    : TR_OPEN SP_ident_opt_arglist TS_SEMICOLON
    | TR_OPEN                     // 1
          object_name           // 2
      TS_PERIOD                   // 3
          SP_ident_opt_arglist       // 4
      TS_SEMICOLON                // 5
    ;

SP_raise_statement
    : TO_RAISE SP_exception_name TS_SEMICOLON
    | TO_RAISE                    // 1
      TS_SEMICOLON                // 2
    ;

SP_return_statement
    : TR_RETURN TS_SEMICOLON
    | TR_RETURN SP_unified_expression TS_SEMICOLON
    ;

SP_continue_statement
    : TR_CONTINUE TS_SEMICOLON
    ;

//*****************************************
//* ALTER/DROP PROCEDURE or FUNCTION
//*****************************************
SP_alter_procedure_statement
    : TR_ALTER TR_PROCEDURE user_object_name TR_COMPILE
      /* ALTER PROCEDURE identifier COMPILE */
    ;

SP_alter_function_statement
    : TR_ALTER TR_FUNCTION user_object_name TR_COMPILE
      /* ALTER FUNCTION identifier COMPILE */
    ;

SP_drop_procedure_statement
    : TR_DROP TR_PROCEDURE user_object_name
    ;

SP_drop_function_statement
    : TR_DROP TR_FUNCTION user_object_name
    ;

/* typeset */
SP_drop_typeset_statement
    : TR_DROP TR_TYPESET user_object_name
    ;

//*****************************************
//* EXECUTE PROCEDURE or FUNCTION
//*****************************************
exec_func_stmt
    : assign_return_value SP_function_opt_arglist
    ;

/* BUG-46824 anonymous_block yacc */
SP_anonymous_block_statement
    : SP_anoymous_block_declare_block
      SP_anonymous_block_first_block
    | SP_anonymous_block_first_block
    ;

SP_anoymous_block_declare_block
    /* BUG-47079 */
    : TS_LESS_THAN_SIGN TS_LESS_THAN_SIGN object_name TS_GREATER_THAN_SIGN TS_GREATER_THAN_SIGN
      TR_DECLARE
      SP_item_declaration_list_option
    | TR_DECLARE
      SP_item_declaration_list_option
    | TS_LESS_THAN_SIGN TS_LESS_THAN_SIGN object_name TS_GREATER_THAN_SIGN TS_GREATER_THAN_SIGN
    ;

SP_anonymous_block_first_block
    :  TR_BEGIN                              
          SP_statement_list                     
          SP_exception_block_option             
       TR_END                                 
       SP_name_option
    ;

SP_exec_or_execute
    : TR_EXEC
    | TR_EXECUTE
    ;

SP_ident_opt_simple_arglist
    : SP_ident_opt_arglist
    | object_name TS_PERIOD SP_ident_opt_arglist
    /* BUG-46600 user.package.psm 이 가능하도록 추가 */
    | object_name TS_PERIOD object_name TS_PERIOD SP_ident_opt_arglist
    ;

assign_return_value
    : out_psm_host_var TS_COLON TS_EQUAL_SIGN
    ;

set_statement
    : TR_SET object_name TS_EQUAL_SIGN object_name
    | TR_SET TR_AGER TS_EQUAL_SIGN TR_ENABLE
    | TR_SET TR_AGER TS_EQUAL_SIGN TR_DISABLE
    ;

initsize_spec
    : SES_V_IDENTIFIER TS_EQUAL_SIGN database_size_option
    {
        // strMatch : INITSIZE, 1
        if(idlOS::strncasecmp("INITSIZE", $<strval>1, 8) != 0)
        {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    ;

/* DATABASE */
create_database_statement
  : TR_CREATE TR_DATABASE object_name  // 1 2 3
      initsize_spec      // 4
      archivelog_option  // 5
      character_set_option //6
      /* CREATE DATABASE name INITSIZE = integer m/g NOARCHIVELOG
                CHARACTER SET KSC5601 NATIONAL CHARACTER SET UTF16*/
  ;

archivelog_option
  : TR_ARCHIVELOG
  | TR_NOARCHIVELOG
  ;

character_set_option
    : db_character_set national_character_set
    | national_character_set db_character_set
    ;

db_character_set
    : SES_V_IDENTIFIER         // 1
      TR_SET                   // 2
      object_name              // 3
      {
          if(idlOS::strncasecmp("CHARACTER", $<strval>1, 9) != 0)
          {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
          }
      }
    ;

national_character_set
    : SES_V_IDENTIFIER  // 1
      SES_V_IDENTIFIER  // 2
      TR_SET                   // 3
      object_name              // 4
      {
          if( (idlOS::strncasecmp("NATIONAL", $<strval>1, 8) != 0) &&
              (idlOS::strncasecmp("CHARACTER", $<strval>2, 9) != 0) )
          {
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
          }
      }
    ;

alter_database_statement
  : TR_ALTER TR_DATABASE object_name alter_database_options
  | TR_ALTER TR_DATABASE object_name alter_database_options2
  | TR_ALTER TR_DATABASE archivelog_option
  | TR_ALTER TR_DATABASE TR_BACKUP TR_LOGANCHOR TR_TO SES_V_LITERAL
  | TR_ALTER TR_DATABASE TR_BACKUP TA_TABLESPACE object_name TR_TO SES_V_LITERAL
  | TR_ALTER TR_DATABASE TR_BACKUP TR_DATABASE TR_TO SES_V_LITERAL
  | TR_ALTER TR_DATABASE TR_CREATE SES_V_IDENTIFIER SES_V_LITERAL filespec_option
  {
      // strMatch : DATAFILE, 4
      if(idlOS::strncasecmp("DATAFILE", $<strval>4, 8) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  }
  | TR_ALTER TR_DATABASE TO_RENAME SES_V_IDENTIFIER SES_V_LITERAL TR_TO SES_V_LITERAL
  {
      // strMatch : DATAFILE, 4
      if(idlOS::strncasecmp("DATAFILE", $<strval>4, 8) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  }
  | TR_ALTER TR_DATABASE TR_RECOVER TR_DATABASE until_option usinganchor_option
  /* ALTER DATABASE BEGIN SNAPSHOT */
  | TR_ALTER TR_DATABASE TR_BEGIN SES_V_IDENTIFIER
  {
      if(idlOS::strncasecmp("SNAPSHOT", $<strval>4, 8) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  }
  /* ALTER DATABASE END SNAPSHOT */
  | TR_ALTER TR_DATABASE TR_END SES_V_IDENTIFIER
  {
      if(idlOS::strncasecmp("SNAPSHOT", $<strval>4, 8) != 0)
      {
          // error 처리
          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  }
  ;

until_option
  : /* empty */
  | TR_UNTIL SES_V_IDENTIFIER
  {
      // strMatch : CANCEL, 2
      if(idlOS::strncasecmp("CANCEL", $<strval>2, 6) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  }
  | TR_UNTIL SES_V_IDENTIFIER SES_V_LITERAL
  {
      // strMatch : TIME, 2
      if(idlOS::strncasecmp("TIME", $<strval>2, 4) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  }
  ;

usinganchor_option
  : /* empty */
  | TR_USING TR_BACKUP TR_LOGANCHOR
  ;

filespec_option
  : /* empty */
  | TR_AS SES_V_LITERAL
  ;

alter_database_options
  : SES_V_IDENTIFIER
  {
    // strMatch : 1) PROCESS
    //            2) CONTROL
    //            3) SERVICE
    //            4) META    , 1
    if(idlOS::strncasecmp("PROCESS", $<strval>1, 7) != 0 &&
       idlOS::strncasecmp("CONTROL", $<strval>1, 7) != 0 &&
       idlOS::strncasecmp("SERVICE", $<strval>1, 7) != 0 &&
       idlOS::strncasecmp("META", $<strval>1, 4) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | SES_V_IDENTIFIER SES_V_IDENTIFIER
  {
    // strMatch : 1) META  UPGRADE
    //            2) META RESETLOGS
    //            3) META RESETUNDO
    //            4) SHUTDOWN NORMAL
    if(idlOS::strncasecmp("META", $<strval>1, 4) == 0 &&
       idlOS::strncasecmp("UPGRADE", $<strval>2, 7) == 0)
    {
    }
    else if(idlOS::strncasecmp("META", $<strval>1, 4) == 0 &&
            idlOS::strncasecmp("RESETLOGS", $<strval>2, 9) == 0)
    {
    }
    else if(idlOS::strncasecmp("META", $<strval>1, 4) == 0 &&
            idlOS::strncasecmp("RESETUNDO", $<strval>2, 9) == 0)
    {
    }
    else if(idlOS::strncasecmp("SHUTDOWN", $<strval>1, 8) == 0 &&
            idlOS::strncasecmp("NORMAL", $<strval>2, 6) == 0)
    {
    }
    else
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | SES_V_IDENTIFIER SES_V_IMMEDIATE
  {
    // strMatch : SHUTDOWN, 1
    if(idlOS::strncasecmp("SHUTDOWN", $<strval>1, 8) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | SES_V_IDENTIFIER TR_EXIT
  {
    // strMatch : SHUTDOWN, 1
    if(idlOS::strncasecmp("SHUTDOWN", $<strval>1, 8) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  ;

alter_database_options2
  : TR_SESSION TR_CLOSE SES_V_INTEGER
  | SES_V_IDENTIFIER TR_COMMIT SES_V_INTEGER
  {
    // strMatch : DTX, 1
    if(idlOS::strncasecmp("DTX", $<strval>1, 3) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | SES_V_IDENTIFIER TR_ROLLBACK SES_V_INTEGER
  {
    // strMatch : DTX, 1
    if(idlOS::strncasecmp("DTX", $<strval>1, 3) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  ;

drop_database_statement
  : TR_DROP TR_DATABASE object_name
  ;

create_tablespace_statement
  : TR_CREATE TA_TABLESPACE object_name
      SES_V_IDENTIFIER datafile_spec
      opt_createTBS_options
  {
    // strMatch : DATAFILE, 4
    if(idlOS::strncasecmp("DATAFILE", $<strval>4, 8) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  } /* missing some tablespace syntax : BUGBUG memory tablespace */
  | TR_CREATE     //1
    TR_VOLATILE   //2
    TA_TABLESPACE //3
    object_name //4
    SES_V_IDENTIFIER //5
    SES_V_INTEGER
    autoextend_clause
  {
      // strMatch : SIZE 5,
      if (idlOS::strncasecmp("SIZE", $<strval>5, 4) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  }
  | TR_CREATE     //1
    TR_VOLATILE   //2
    TA_TABLESPACE //3
    object_name //4
    SES_V_IDENTIFIER //5
    SES_V_INTEGER    // 6
    SES_V_IDENTIFIER  //7
    autoextend_clause
  {
      // strMatch : SIZE 5,
      if (idlOS::strncasecmp("SIZE", $<strval>5, 4) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
      if(idlOS::strncasecmp("K", $<strval>7, 1) != 0 &&
         idlOS::strncasecmp("M", $<strval>7, 1) != 0 &&
         idlOS::strncasecmp("G", $<strval>7, 1) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  } /* BUGBUG memory tablespace */
  ;

create_temp_tablespace_statement
  : TR_CREATE TR_TEMPORARY TA_TABLESPACE object_name
      SES_V_IDENTIFIER datafile_spec
      opt_extentsize_option
  {
    // strMatch : TEMPFILE, 5
    if(idlOS::strncasecmp("TEMPFILE", $<strval>5, 8) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  ;

alter_tablespace_dcl_statement
  : TR_ALTER TA_TABLESPACE object_name
      opt_alterTBS_onoff_options
    /* ALTER TABLESPACE name ONLINE/OFFLINE */
  | TR_ALTER TA_TABLESPACE object_name
    TO_RENAME SES_V_IDENTIFIER filename_list TR_TO filename_list
  {
    // strMatch : 1) DATAFILE, 5
    //            2) TEMPFILE, 5
    if(idlOS::strncasecmp("DATAFILE", $<strval>5, 8) != 0 &&
       idlOS::strncasecmp("TEMPFILE", $<strval>5, 8) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | TR_ALTER TA_TABLESPACE object_name
    TR_ALTER SES_V_IDENTIFIER SES_V_LITERAL online_offline_option
  {
    // strMatch : 1) DATAFILE, 5
    //            2) TEMPFILE, 5
    if(idlOS::strncasecmp("DATAFILE", $<strval>5, 8) != 0 &&
       idlOS::strncasecmp("TEMPFILE", $<strval>5, 8) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | TR_ALTER TA_TABLESPACE object_name backupTBS_option
  ;

backupTBS_option
    : TR_BEGIN TR_BACKUP
    | TR_END TR_BACKUP
    ;

alter_tablespace_ddl_statement
  : TR_ALTER TA_TABLESPACE object_name
      TR_AND SES_V_IDENTIFIER datafile_spec
  {
    // strMatch : 1) DATAFILE, 5
    //            2) TEMPFILE, 5
    if(idlOS::strncasecmp("DATAFILE", $<strval>5, 8) != 0 &&
       idlOS::strncasecmp("TEMPFILE", $<strval>5, 8) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | TR_ALTER TA_TABLESPACE object_name
      TR_DROP SES_V_IDENTIFIER filename_list
  {
    // strMatch : 1) DATAFILE, 5
    //            2) TEMPFILE, 5
    if(idlOS::strncasecmp("DATAFILE", $<strval>5, 8) != 0 &&
       idlOS::strncasecmp("TEMPFILE", $<strval>5, 8) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | TR_ALTER TA_TABLESPACE object_name
      TR_ALTER SES_V_IDENTIFIER SES_V_LITERAL autoextend_statement
  {
    // strMatch : 1) DATAFILE, 5
    //            2) TEMPFILE, 5
    if(idlOS::strncasecmp("DATAFILE", $<strval>5, 8) != 0 &&
       idlOS::strncasecmp("TEMPFILE", $<strval>5, 8) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | TR_ALTER TA_TABLESPACE object_name
      TR_ALTER SES_V_IDENTIFIER SES_V_LITERAL
      SES_V_IDENTIFIER size_option
  {
    // strMatch : 1) DATAFILE, 5 && SIZE, 7
    //            2) TEMPFILE, 5 && SIZE, 7
    if(idlOS::strncasecmp("DATAFILE", $<strval>5, 8) == 0 &&
       idlOS::strncasecmp("SIZE", $<strval>7, 4) == 0)
    {
    }
    else if (idlOS::strncasecmp("TEMPFILE", $<strval>5, 8) == 0 &&
             idlOS::strncasecmp("SIZE", $<strval>7, 4) == 0)
    {
    }
    else
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  ;

drop_tablespace_statement
  : TR_DROP TA_TABLESPACE object_name
      opt_droptablespace_options
  ;

datafile_spec
  : datafile_spec TS_COMMA filespec
  | filespec
  ;

filespec
  : SES_V_LITERAL autoextend_clause
    /* filename */
  | SES_V_LITERAL SES_V_IDENTIFIER SES_V_INTEGER autoextend_clause
    /* filename SIZE integer */
  {
    // strMatch : SIZE, 2
    if(idlOS::strncasecmp("SIZE", $<strval>2, 4) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | SES_V_LITERAL SES_V_IDENTIFIER SES_V_INTEGER SES_V_IDENTIFIER autoextend_clause
    /* filename SIZE integer K/M/G */
    /* filename SIZE integer REUSE */
  {
    // if ( strMatch : SIZE, 2)
    // {
    //    if ( strMatch : REUSE, 4)
    //    else if ( strMatch : K, 4)
    //    else if ( strMatch : M, 4)
    //    else if ( strMatch : G, 4)
    // }
    if(idlOS::strncasecmp("SIZE", $<strval>2, 4) != 0 ||
       idlOS::strncasecmp("REUSE", $<strval>4, 5) != 0 &&
       idlOS::strncasecmp("K", $<strval>4, 1) != 0 &&
       idlOS::strncasecmp("M", $<strval>4, 1) != 0 &&
       idlOS::strncasecmp("G", $<strval>4, 1) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | SES_V_LITERAL SES_V_IDENTIFIER autoextend_clause
    /* filename REUSE */
  {
    // strMatch : REUSE, 2
    if(idlOS::strncasecmp("REUSE", $<strval>2, 5) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | SES_V_LITERAL SES_V_IDENTIFIER SES_V_INTEGER SES_V_IDENTIFIER SES_V_IDENTIFIER autoextend_clause
    /* filename SIZE integer K/M/G REUSE */
  {
    // if ( strMatch : SIZE, 2 && REUSE, 5)
    // {
    //    if ( strMatch : K, 4)
    //    else if ( strMatch : M, 4)
    //    else if ( strMatch : G, 4)
    // }
    if(idlOS::strncasecmp("SIZE", $<strval>2, 4) != 0 ||
       idlOS::strncasecmp("REUSE", $<strval>5, 5) != 0 ||
       idlOS::strncasecmp("K", $<strval>4, 1) != 0 &&
       idlOS::strncasecmp("M", $<strval>4, 1) != 0 &&
       idlOS::strncasecmp("G", $<strval>4, 1) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  ;

autoextend_clause
  : /* empty */
  | autoextend_statement
  ;

autoextend_statement
  : SES_V_IDENTIFIER TR_OFF
    /* AUTOEXTEND OFF */
  {
    // strMatch : AUTOEXTEND, 1
    if(idlOS::strncasecmp("AUTOEXTEND", $<strval>1, 10) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | SES_V_IDENTIFIER TR_ON
    /* AUTOEXTEND ON */
  {
    // strMatch : AUTOEXTEND, 1
    if(idlOS::strncasecmp("AUTOEXTEND", $<strval>1, 10) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | SES_V_IDENTIFIER TR_ON SES_V_NEXT size_option
    /* AUTOEXTEND ON NEXT integer          */
    /* AUTOEXTEND ON NEXT integer K/M/G    */
  {
    // strMatch : AUTOEXTEND, 1
    if(idlOS::strncasecmp("AUTOEXTEND", $<strval>1, 10) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | SES_V_IDENTIFIER TR_ON SES_V_IDENTIFIER size_option
    /* AUTOEXTEND ON MAXSIZE integer       */
    /* AUTOEXTEND ON MAXSIZE integer K/M/G */
  {
    // strMatch : AUTOEXTEND, 1
    // strMatch : MAXSIZE, 3
    if(idlOS::strncasecmp("AUTOEXTEND", $<strval>1, 10) != 0 ||
       idlOS::strncasecmp("MAXSIZE", $<strval>3, 7) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | SES_V_IDENTIFIER TR_ON SES_V_NEXT SES_V_INTEGER maxsize_clause
    /* AUTOEXTEND ON NEXT integer maxsize_clause */
  {
    // strMatch : AUTOEXTEND, 1
    if(idlOS::strncasecmp("AUTOEXTEND", $<strval>1, 10) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | SES_V_IDENTIFIER TR_ON SES_V_NEXT SES_V_INTEGER SES_V_IDENTIFIER maxsize_clause
    /* AUTOEXTEND ON NEXT integer K/M/G maxsize_clause */
  {
    // strMatch : AUTOEXTEND, 1
    // strMatch : K|M|G, 5
    if(idlOS::strncasecmp("AUTOEXTEND", $<strval>1, 10) != 0 ||
       idlOS::strncasecmp("K", $<strval>5, 1) != 0 &&
       idlOS::strncasecmp("M", $<strval>5, 1) != 0 &&
       idlOS::strncasecmp("G", $<strval>5, 1) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | SES_V_IDENTIFIER TR_ON SES_V_IDENTIFIER SES_V_IDENTIFIER
    /* AUTOEXTEND ON MAXSIZE UNLIMITED */
  {
    // strMatch : AUTOEXTEND, 1
    // strMatch : MAXSIZE, 3
    // strMatch : UNLIMITED, 4
    if(idlOS::strncasecmp("AUTOEXTEND", $<strval>1, 10) != 0 ||
       idlOS::strncasecmp("MAXSIZE", $<strval>3, 7) != 0 ||
       idlOS::strncasecmp("UNLIMITED", $<strval>4, 9) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  ;

maxsize_clause
  : SES_V_IDENTIFIER SES_V_IDENTIFIER
    /* MAXSIZE UNLIMITED */
  {
    // if( strMatch : MAXSIZE, 1 && UNLIMITED, 2)
    if(idlOS::strncasecmp("MAXSIZE", $<strval>1, 7) != 0 ||
       idlOS::strncasecmp("UNLIMITED", $<strval>2, 9) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | SES_V_IDENTIFIER size_option
    /* MAXSIZE integer */
    /* MAXSIZE integer K/M/G */
  {
    // if( strMatch : MAXSIZE, 1)
    if(idlOS::strncasecmp("MAXSIZE", $<strval>1, 7) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  ;

opt_createTBS_options
  : /* empty */
  | tablespace_option
  | tablespace_option tablespace_option
  ;

tablespace_option
  : extentsize_clause
  | segment_management_clause
  ;

opt_extentsize_option
  : /* empty */
  | extentsize_clause
  ;

extentsize_clause
  : TA_EXTENTSIZE size_option
  ;

segment_management_clause
  : TO_SEGMENT SES_V_IDENTIFIER SES_V_IDENTIFIER
  ;

database_size_option
  : SES_V_INTEGER
  | SES_V_INTEGER SES_V_IDENTIFIER
  {
    // if ( strMatch : K, 2 )
    // else if ( strMatch : M, 2)
    // else if ( strMatch : G, 2)
    if(idlOS::strncasecmp("K", $<strval>2, 1) != 0 &&
       idlOS::strncasecmp("M", $<strval>2, 1) != 0 &&
       idlOS::strncasecmp("G", $<strval>2, 1) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  ;

size_option
  : SES_V_INTEGER
  | SES_V_INTEGER SES_V_IDENTIFIER
  {
    // if ( strMatch : K, 2 )
    // else if ( strMatch : M, 2)
    // else if ( strMatch : G, 2)
    if(idlOS::strncasecmp("K", $<strval>2, 1) != 0 &&
       idlOS::strncasecmp("M", $<strval>2, 1) != 0 &&
       idlOS::strncasecmp("G", $<strval>2, 1) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  ;

opt_alterTBS_onoff_options
  : TA_ONLINE
  | TA_OFFLINE
  ;

online_offline_option
  : TA_ONLINE
  | TA_OFFLINE
  ;

filename_list
  : filename_list TS_COMMA filename
  | filename
  ;

filename
  : SES_V_LITERAL
  ;

opt_droptablespace_options
  : /* empty */
  | SES_V_IDENTIFIER SES_V_IDENTIFIER opt_cascade_constraints
    /* INCLUDING CONTENTS opt_cascade_constraints */
  {
    // if ( strMatch: INCLUDING, 1 && CONTENTS, 2 )
    if(idlOS::strncasecmp("INCLUDING", $<strval>1, 9) != 0 ||
       idlOS::strncasecmp("CONTENTS", $<strval>2, 8) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  | SES_V_IDENTIFIER SES_V_IDENTIFIER TR_AND SES_V_IDENTIFIER opt_cascade_constraints
    /* INCLUDING CONTENTS AND DATAFILES opt_cascade_constraints */
  {
    // if ( strMatch: INCLUDING, 1 && CONTENTS, 2 && DATAFILES, 4 )
    if(idlOS::strncasecmp("INCLUDING", $<strval>1, 9) != 0 ||
       idlOS::strncasecmp("CONTENTS", $<strval>2, 8) != 0 ||
       idlOS::strncasecmp("DATAFILES", $<strval>4, 9) != 0)
    {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
  }
  ;

//*****************************************
//* PROJ-1359 TRIGGER
//*****************************************
create_trigger_statement
  : create_or_replace_trigger_clause    // 1
    user_object_name                    // 2
    trigger_event_clause                // 3
    TR_ON user_object_name           // 5
    trigger_referencing_clause          // 6
    trigger_action_information          // 7
    trigger_action_clause               // 8
  ;

create_or_replace_trigger_clause
    : TR_CREATE TR_TRIGGER
    | TR_CREATE TR_OR TO_REPLACE TR_TRIGGER
    ;

alter_trigger_statement
  : TR_ALTER TR_TRIGGER user_object_name alter_trigger_option
  ;

drop_trigger_statement
  : TR_DROP TR_TRIGGER user_object_name
  ;

trigger_event_clause
  : trigger_event_time_clause trigger_event_type_list
  ;

trigger_event_type_list
    : trigger_event_type_list TR_OR TR_DELETE
    | trigger_event_type_list TR_OR TR_INSERT
    | trigger_event_type_list TR_OR TR_UPDATE trigger_event_columns
    | TR_INSERT
    | TR_DELETE
    | TR_UPDATE trigger_event_columns
    ;

trigger_event_time_clause
  : TR_BEFORE
  | TR_AFTER
  ;

trigger_event_columns
  : /* empty */
  | TR_OF
    column_commalist
  ;

trigger_referencing_clause
  : /* empty */
  | TR_REFERENCING
    trigger_referencing_list
  ;

trigger_referencing_list
  : trigger_referencing_list TS_COMMA trigger_referencing_item
  | trigger_referencing_item
  ;

trigger_referencing_item
  : trigger_old_or_new
    trigger_row_or_table
    trigger_as_or_none
    trigger_referencing_name
  ;

trigger_old_or_new
  : TR_OLD
  | TR_NEW
  ;

trigger_row_or_table
  : /* empty */
  | TR_ROW
  | TR_TABLE
  ;

trigger_as_or_none
  : /* empty */
  | TR_AS
  ;

trigger_referencing_name
  : object_name
  ;

trigger_action_information
  : /* empty */
  | TR_FOR TR_EACH TR_ROW trigger_when_condition
  | TR_FOR TR_EACH TR_STATEMENT
  ;

trigger_when_condition
  : /* empty */
  | TR_WHEN TS_OPENING_PARENTHESIS expression TS_CLOSING_PARENTHESIS
  ;

trigger_action_clause
  : SP_as_o_is
    SP_first_block
  ;

alter_trigger_option
  : TR_ENABLE
  | TR_DISABLE
  | TR_COMPILE
  ;

create_or_replace_directory_statement
  : create_or_replace_directory_clause
    object_name
    TR_AS SES_V_LITERAL
  ;

create_or_replace_directory_clause
  : TR_CREATE TR_DIRECTORY
  | TR_CREATE TR_OR TO_REPLACE TR_DIRECTORY
  ;

drop_directory_statement
  : TR_DROP TR_DIRECTORY object_name
  ;

create_materialized_view_statement
  : TR_CREATE TO_MATERIALIZED TR_VIEW user_object_name
    opt_view_column_def
    table_options
    opt_lob_attribute_list
    opt_mview_build_refresh
    TR_AS select_statement
  ;

opt_mview_build_refresh
  : /* empty */
  | SES_V_IDENTIFIER SES_V_IDENTIFIER
  {
    /* BUILD [IMMEDIATE | DEFERRED] */
    /* REFRESH [COMPLETE | FAST | FORCE] */
    /* NEVER REFRESH */
    idBool sPassFlag = ID_FALSE;

    if ( idlOS::strncasecmp( "BUILD", $<strval>1, 5 ) == 0 )
    {
        if ( ( idlOS::strncasecmp( "IMMEDIATE", $<strval>2, 9 ) == 0 ) ||
             ( idlOS::strncasecmp( "DEFERRED", $<strval>2, 8 ) == 0 ) )
        {
            sPassFlag = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else if ( idlOS::strncasecmp( "REFRESH", $<strval>1, 7 ) == 0 )
    {
        if ( ( idlOS::strncasecmp( "FORCE", $<strval>2, 5 ) == 0 ) ||
             ( idlOS::strncasecmp( "COMPLETE", $<strval>2, 8 ) == 0 ) ||
             ( idlOS::strncasecmp( "FAST", $<strval>2, 4 ) == 0 ) )
        {
            sPassFlag = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else if ( idlOS::strncasecmp( "NEVER", $<strval>1, 5 ) == 0 )
    {
        if ( idlOS::strncasecmp( "REFRESH", $<strval>2, 7 ) == 0 )
        {
            sPassFlag = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( sPassFlag != ID_TRUE )
    {
        ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                         ulpERR_ABORT_COMP_Unterminated_String_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
        COMPerror( ulpGetErrorMSG( &gUlpParseInfo.mErrorMgr ) );
    }
    else
    {
        /* Nothing to do */
    }
  }
  | SES_V_IDENTIFIER mview_refresh_time
  {
    /* REFRESH [ON DEMAND | ON COMMIT] */
    if ( idlOS::strncasecmp( "REFRESH", $<strval>1, 7 ) == 0 )
    {
        /* Nothing to do */
    }
    else
    {
        ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                         ulpERR_ABORT_COMP_Unterminated_String_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
        COMPerror( ulpGetErrorMSG( &gUlpParseInfo.mErrorMgr ) );
    }
  }
  | SES_V_IDENTIFIER SES_V_IDENTIFIER mview_refresh_time
  {
    /* REFRESH [COMPLETE | FAST | FORCE] [ON DEMAND | ON COMMIT] */
    idBool sPassFlag = ID_FALSE;

    if ( idlOS::strncasecmp( "REFRESH", $<strval>1, 7 ) == 0 )
    {
        if ( ( idlOS::strncasecmp( "FORCE", $<strval>2, 5 ) == 0 ) ||
             ( idlOS::strncasecmp( "COMPLETE", $<strval>2, 8 ) == 0 ) ||
             ( idlOS::strncasecmp( "FAST", $<strval>2, 4 ) == 0 ) )
        {
            sPassFlag = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( sPassFlag != ID_TRUE )
    {
        ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                         ulpERR_ABORT_COMP_Unterminated_String_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
        COMPerror( ulpGetErrorMSG( &gUlpParseInfo.mErrorMgr ) );
    }
    else
    {
        /* Nothing to do */
    }
  }
  | SES_V_IDENTIFIER SES_V_IDENTIFIER SES_V_IDENTIFIER SES_V_IDENTIFIER
  {
    /* BUILD [IMMEDIATE | DEFERRED] REFRESH [COMPLETE | FAST | FORCE] */
    /* BUILD [IMMEDIATE | DEFERRED] NEVER REFRESH */
    idBool sPassFlag = ID_FALSE;

    if ( idlOS::strncasecmp( "BUILD", $<strval>1, 5 ) == 0 )
    {
        if ( ( idlOS::strncasecmp( "IMMEDIATE", $<strval>2, 9 ) == 0 ) ||
             ( idlOS::strncasecmp( "DEFERRED", $<strval>2, 8 ) == 0 ) )
        {
            if ( idlOS::strncasecmp( "REFRESH", $<strval>3, 7 ) == 0 )
            {
                if ( ( idlOS::strncasecmp( "FORCE", $<strval>4, 5 ) == 0 ) ||
                     ( idlOS::strncasecmp( "COMPLETE", $<strval>4, 8 ) == 0 ) ||
                     ( idlOS::strncasecmp( "FAST", $<strval>4, 4 ) == 0 ) )
                {
                    sPassFlag = ID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else if ( idlOS::strncasecmp( "NEVER", $<strval>3, 5 ) == 0 )
            {
                if ( idlOS::strncasecmp( "REFRESH", $<strval>4, 7 ) == 0 )
                {
                    sPassFlag = ID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( sPassFlag != ID_TRUE )
    {
        ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                         ulpERR_ABORT_COMP_Unterminated_String_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
        COMPerror( ulpGetErrorMSG( &gUlpParseInfo.mErrorMgr ) );
    }
    else
    {
        /* Nothing to do */
    }
  }
  | SES_V_IDENTIFIER SES_V_IDENTIFIER SES_V_IDENTIFIER mview_refresh_time
  {
    /* BUILD [IMMEDIATE | DEFERRED] REFRESH [ON DEMAND | ON COMMIT] */
    idBool sPassFlag = ID_FALSE;

    if ( idlOS::strncasecmp( "BUILD", $<strval>1, 5 ) == 0 )
    {
        if ( ( idlOS::strncasecmp( "IMMEDIATE", $<strval>2, 9 ) == 0 ) ||
             ( idlOS::strncasecmp( "DEFERRED", $<strval>2, 8 ) == 0 ) )
        {
            if ( idlOS::strncasecmp( "REFRESH", $<strval>3, 7 ) == 0 )
            {
                sPassFlag = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( sPassFlag != ID_TRUE )
    {
        ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                         ulpERR_ABORT_COMP_Unterminated_String_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
        COMPerror( ulpGetErrorMSG( &gUlpParseInfo.mErrorMgr ) );
    }
    else
    {
        /* Nothing to do */
    }
  }
  | SES_V_IDENTIFIER SES_V_IDENTIFIER SES_V_IDENTIFIER SES_V_IDENTIFIER mview_refresh_time
  {
    /* BUILD [IMMEDIATE | DEFERRED] REFRESH [COMPLETE | FAST | FORCE] [ON DEMAND | ON COMMIT] */
    idBool sPassFlag = ID_FALSE;

    if ( idlOS::strncasecmp( "BUILD", $<strval>1, 5 ) == 0 )
    {
        if ( ( idlOS::strncasecmp( "IMMEDIATE", $<strval>2, 9 ) == 0 ) ||
             ( idlOS::strncasecmp( "DEFERRED", $<strval>2, 8 ) == 0 ) )
        {
            if ( idlOS::strncasecmp( "REFRESH", $<strval>3, 7 ) == 0 )
            {
                if ( ( idlOS::strncasecmp( "FORCE", $<strval>4, 5 ) == 0 ) ||
                     ( idlOS::strncasecmp( "COMPLETE", $<strval>4, 8 ) == 0 ) ||
                     ( idlOS::strncasecmp( "FAST", $<strval>4, 4 ) == 0 ) )
                {
                    sPassFlag = ID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( sPassFlag != ID_TRUE )
    {
        ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                         ulpERR_ABORT_COMP_Unterminated_String_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
        COMPerror( ulpGetErrorMSG( &gUlpParseInfo.mErrorMgr ) );
    }
    else
    {
        /* Nothing to do */
    }
  }
  ;

mview_refresh_time
  : TR_ON SES_V_IDENTIFIER
  {
    if ( idlOS::strncasecmp( "DEMAND", $<strval>2, 6 ) == 0 )
    {
        /* Nothing to do */
    }
    else
    {
        ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                         ulpERR_ABORT_COMP_Unterminated_String_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
        COMPerror( ulpGetErrorMSG( &gUlpParseInfo.mErrorMgr ) );
    }
  }
  | TR_ON TR_COMMIT
  ;

alter_materialized_view_statement
  : TR_ALTER TO_MATERIALIZED TR_VIEW user_object_name
    mview_refresh_alter
  ;

mview_refresh_alter
  : SES_V_IDENTIFIER SES_V_IDENTIFIER
  {
    /* REFRESH [COMPLETE | FAST | FORCE] */
    idBool sPassFlag = ID_FALSE;

    if ( idlOS::strncasecmp( "REFRESH", $<strval>1, 7 ) == 0 )
    {
        if ( ( idlOS::strncasecmp( "FORCE", $<strval>2, 5 ) == 0 ) ||
             ( idlOS::strncasecmp( "COMPLETE", $<strval>2, 8 ) == 0 ) ||
             ( idlOS::strncasecmp( "FAST", $<strval>2, 4 ) == 0 ) )
        {
            sPassFlag = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( sPassFlag != ID_TRUE )
    {
        ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                         ulpERR_ABORT_COMP_Unterminated_String_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
        COMPerror( ulpGetErrorMSG( &gUlpParseInfo.mErrorMgr ) );
    }
    else
    {
        /* Nothing to do */
    }
  }
  | SES_V_IDENTIFIER mview_refresh_time
  {
    /* REFRESH [ON DEMAND | ON COMMIT] */
    if ( idlOS::strncasecmp( "REFRESH", $<strval>1, 7 ) == 0 )
    {
        /* Nothing to do */
    }
    else
    {
        ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                         ulpERR_ABORT_COMP_Unterminated_String_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
        COMPerror( ulpGetErrorMSG( &gUlpParseInfo.mErrorMgr ) );
    }
  }
  | SES_V_IDENTIFIER SES_V_IDENTIFIER mview_refresh_time
  {
    /* REFRESH [COMPLETE | FAST | FORCE] [ON DEMAND | ON COMMIT] */
    idBool sPassFlag = ID_FALSE;

    if ( idlOS::strncasecmp( "REFRESH", $<strval>1, 7 ) == 0 )
    {
        if ( ( idlOS::strncasecmp( "FORCE", $<strval>2, 5 ) == 0 ) ||
             ( idlOS::strncasecmp( "COMPLETE", $<strval>2, 8 ) == 0 ) ||
             ( idlOS::strncasecmp( "FAST", $<strval>2, 4 ) == 0 ) )
        {
            sPassFlag = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( sPassFlag != ID_TRUE )
    {
        ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                         ulpERR_ABORT_COMP_Unterminated_String_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
        COMPerror( ulpGetErrorMSG( &gUlpParseInfo.mErrorMgr ) );
    }
    else
    {
        /* Nothing to do */
    }
  }
  ;

drop_materialized_view_statement
  : TR_DROP TO_MATERIALIZED TR_VIEW user_object_name
  ;

/* PROJ-1438 Job Scheduler */
create_job_statement
  : TR_CREATE
    SES_V_IDENTIFIER
    SES_V_IDENTIFIER
    SP_exec_or_execute SP_ident_opt_simple_arglist
    TR_START arithmetic_expression
    opt_end_statement
    opt_interval_statement
    opt_enable_statement
    opt_job_comment_statement
  {
      if(idlOS::strncasecmp("JOB", $<strval>2, 3) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  }
  ;

opt_end_statement
  : /* Nothing to do */
  | TR_END arithmetic_expression
  ;

opt_interval_statement
  :
  | interval_statement
  ;

interval_statement
  : SES_V_IDENTIFIER SES_V_INTEGER SES_V_IDENTIFIER
  ;

// BUG-41713 each job enable disable
opt_enable_statement
  :
  | enable_statement
  ;

// BUG-41713 each job enable disable
enable_statement
  : TR_ENABLE
  | TR_DISABLE
  ;

// BUG-41713 each job enable disable
opt_job_comment_statement
  :
  | job_comment_statement
  ;

// BUG-41713 each job enable disable
job_comment_statement
  : TR_COMMENT SES_V_LITERAL
  ;

alter_job_statement
  : TR_ALTER SES_V_IDENTIFIER SES_V_IDENTIFIER TR_SET SP_exec_or_execute SP_ident_opt_simple_arglist
  {
      if(idlOS::strncasecmp("JOB", $<strval>2, 3) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }

  }
  | TR_ALTER SES_V_IDENTIFIER SES_V_IDENTIFIER TR_SET TR_START arithmetic_expression
  {
      if(idlOS::strncasecmp("JOB", $<strval>2, 3) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }

  }
  | TR_ALTER SES_V_IDENTIFIER SES_V_IDENTIFIER TR_SET TR_END arithmetic_expression
  {
      if(idlOS::strncasecmp("JOB", $<strval>2, 3) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }

  }
  | TR_ALTER SES_V_IDENTIFIER SES_V_IDENTIFIER TR_SET interval_statement
  {
      if(idlOS::strncasecmp("JOB", $<strval>2, 3) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  }
  | TR_ALTER SES_V_IDENTIFIER SES_V_IDENTIFIER TR_SET enable_statement
  {
      // BUG-41713 each job enable disable
      if(idlOS::strncasecmp("JOB", $<strval>2, 3) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  }
  | TR_ALTER SES_V_IDENTIFIER SES_V_IDENTIFIER TR_SET job_comment_statement
  {
      // BUG-41713 each job enable disable
      if(idlOS::strncasecmp("JOB", $<strval>2, 3) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  }
  ;

drop_job_statement
  : TR_DROP SES_V_IDENTIFIER SES_V_IDENTIFIER 
  {
      if(idlOS::strncasecmp("JOB", $<strval>2, 3) != 0)
      {
          // error 처리

          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
      }
  }

/* BUG-17566 : sesc 에서 큰따옴표 이용한 Naming Rule 제약 제거 */
object_name
  : SES_V_IDENTIFIER
  | SES_V_DQUOTE_LITERAL
  | allowed_keywords_for_object_name
  ;

allowed_keywords_for_object_name
  : SES_V_GET
  | SES_V_DIAGNOSTICS
  | SES_V_CONDITION
  | SES_V_NUMBER
  | SES_V_ROW_COUNT
  | SES_V_RETURNED_SQLCODE
  | SES_V_RETURNED_SQLSTATE
  | SES_V_MESSAGE_TEXT
  | SES_V_ROW_NUMBER
  | SES_V_COLUMN_NUMBER
  ;


/*****************************************
 * HOST VARIABLES
 ****************************************/
in_host_var_list
    : SES_V_HOSTVARIABLE indicator_opt
    {
        // yyvsp is a internal variable for bison,
        // host value in/out type = HV_IN_TYPE,
        // host value file type = HV_FILE_NONE,
        // Does it need to transform the query? = TRUE
        // num of tokens = 2,
        // index of host value token = 1,
        // indexs of remove token = 0 (it means none)
        ulpValidateHostValue( yyvsp,
                              HV_IN_TYPE,
                              HV_FILE_NONE,
                              ID_TRUE,
                              (SInt)2,
                              (SInt)1,
                              (SInt)0
                            );
    }
    | SES_V_BLOB_FILE SES_V_HOSTVARIABLE file_option indicator
    {
        ulpValidateHostValue( yyvsp,
                              HV_IN_TYPE,
                              HV_FILE_BLOB,
                              ID_FALSE,
                              (SInt)4,
                              (SInt)2,
                              (SInt)1
                            );
    }
    | SES_V_CLOB_FILE SES_V_HOSTVARIABLE file_option indicator
    {
        ulpValidateHostValue( yyvsp,
                              HV_IN_TYPE,
                              HV_FILE_CLOB,
                              ID_FALSE,
                              (SInt)4,
                              (SInt)2,
                              (SInt)1
                            );
    }
    | in_host_var_list TS_COMMA SES_V_HOSTVARIABLE indicator_opt
    {
        ulpValidateHostValue( yyvsp,
                              HV_IN_TYPE,
                              HV_FILE_NONE,
                              ID_TRUE,
                              (SInt)4,
                              (SInt)3,
                              (SInt)0
                            );
    }
    | in_host_var_list TS_COMMA SES_V_BLOB_FILE SES_V_HOSTVARIABLE file_option indicator
    {
        ulpValidateHostValue( yyvsp,
                              HV_IN_TYPE,
                              HV_FILE_BLOB,
                              ID_FALSE,
                              (SInt)6,
                              (SInt)4,
                              (SInt)3
                            );
    }
    | in_host_var_list TS_COMMA SES_V_CLOB_FILE SES_V_HOSTVARIABLE file_option indicator
    {
        ulpValidateHostValue( yyvsp,
                              HV_IN_TYPE,
                              HV_FILE_CLOB,
                              ID_FALSE,
                              (SInt)6,
                              (SInt)4,
                              (SInt)3
                            );
    }
    ;

host_var_list
    : host_variable
    | host_var_list TS_COMMA host_variable
    ;

host_variable
    : SES_V_HOSTVARIABLE TR_IN indicator_opt
    {
        ulpValidateHostValue( yyvsp,
                              HV_IN_TYPE,
                              HV_FILE_NONE,
                              ID_TRUE,
                              (SInt)3,
                              (SInt)1,
                              (SInt)2
                            );
    }
    | SES_V_HOSTVARIABLE TR_OUT indicator_opt
    {
        ulpValidateHostValue( yyvsp,
                              HV_OUT_PSM_TYPE,
                              HV_FILE_NONE,
                              ID_TRUE,
                              (SInt)3,
                              (SInt)1,
                              (SInt)2
                            );
    }
    | SES_V_HOSTVARIABLE TR_IN TR_OUT indicator_opt
    {
        ulpValidateHostValue( yyvsp,
                              HV_INOUT_TYPE,
                              HV_FILE_NONE,
                              ID_TRUE,
                              (SInt)4,
                              (SInt)1,
                              (SInt)32
                            );
    }
    | SES_V_HOSTVARIABLE indicator_opt
    {
        ulpValidateHostValue( yyvsp,
                              HV_IN_TYPE,
                              HV_FILE_NONE,
                              ID_TRUE,
                              (SInt)2,
                              (SInt)1,
                              (SInt)0
                            );
    }
    | SES_V_BLOB_FILE SES_V_HOSTVARIABLE file_option indicator
    {
        ulpValidateHostValue( yyvsp,
                              HV_IN_TYPE,
                              HV_FILE_BLOB,
                              ID_TRUE,
                              (SInt)4,
                              (SInt)2,
                              (SInt)1
                            );
    }
    | SES_V_CLOB_FILE SES_V_HOSTVARIABLE file_option indicator
    {
        ulpValidateHostValue( yyvsp,
                              HV_IN_TYPE,
                              HV_FILE_CLOB,
                              ID_TRUE,
                              (SInt)4,
                              (SInt)2,
                              (SInt)1
                            );
    }
    ;

free_lob_loc_list
    : free_lob_loc_list TS_COMMA SES_V_HOSTVARIABLE
    {
        ulpSymTElement *sSymNode;

        // in host variable
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>3+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unknown_Hostvar_Error,
                             $<strval>3+1 );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
        else
        {
            if( (sSymNode -> mType != H_BLOBLOCATOR) &&
                (sSymNode -> mType != H_CLOBLOCATOR) )
            {
                //host 변수 type error
                // error 처리

                ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                 ulpERR_ABORT_COMP_Lob_Locator_Error );
                gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
            }

            gUlpCodeGen.ulpGenAddHostVarList( $<strval>3+1, sSymNode ,
                                              gUlpIndName, NULL, NULL, HV_IN_TYPE);

            gUlpCodeGen.ulpIncHostVarNum( 1 );
            gUlpCodeGen.ulpGenAddHostVarArr( 1 );
        }

    }
    | SES_V_HOSTVARIABLE
    {
        ulpSymTElement *sSymNode;

        // in host variable
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>1+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unknown_Hostvar_Error,
                             $<strval>1+1 );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
        else
        {
            gUlpCodeGen.ulpGenAddHostVarList( $<strval>1+1, sSymNode ,
                                              gUlpIndName, NULL, NULL, HV_IN_TYPE);

            gUlpCodeGen.ulpIncHostVarNum( 1 );
            gUlpCodeGen.ulpGenAddHostVarArr( 1 );
        }
    }
    ;

/* BUG-46824 psm에서는 into keyword를 삭제하지 않도록 함 */
SP_into_host_var
    : TR_INTO out_psm_host_var_list
    ;

opt_into_host_var
    : /* empty */
    | TR_INTO out_host_var_list
    {
        gUlpCodeGen.ulpGenRemoveQueryToken( $<strval>1 );
    }
    ;

out_host_var_list
    : SES_V_HOSTVARIABLE indicator_opt
    {
        ulpValidateHostValue( yyvsp,
                              HV_OUT_TYPE,
                              HV_FILE_NONE,
                              ID_FALSE,
                              (SInt)2,
                              (SInt)1,
                              (SInt)1
                            );
    }
    | out_host_var_list TS_COMMA SES_V_HOSTVARIABLE indicator_opt
    {
        ulpValidateHostValue( yyvsp,
                              HV_OUT_TYPE,
                              HV_FILE_NONE,
                              ID_FALSE,
                              (SInt)4,
                              (SInt)3,
                              (SInt)23
                            );
    }
    | SES_V_BLOB_FILE SES_V_HOSTVARIABLE file_option indicator
    {
        ulpValidateHostValue( yyvsp,
                              HV_OUT_TYPE,
                              HV_FILE_BLOB,
                              ID_FALSE,
                              (SInt)4,
                              (SInt)2,
                              (SInt)12
                            );
    }
    | SES_V_CLOB_FILE SES_V_HOSTVARIABLE file_option indicator
    {
        ulpValidateHostValue( yyvsp,
                              HV_OUT_TYPE,
                              HV_FILE_CLOB,
                              ID_FALSE,
                              (SInt)4,
                              (SInt)2,
                              (SInt)12
                            );
    }
    | out_host_var_list TS_COMMA SES_V_BLOB_FILE SES_V_HOSTVARIABLE file_option indicator
    {
        ulpValidateHostValue( yyvsp,
                              HV_OUT_TYPE,
                              HV_FILE_BLOB,
                              ID_FALSE,
                              (SInt)6,
                              (SInt)4,
                              (SInt)234
                            );
    }
    | out_host_var_list TS_COMMA SES_V_CLOB_FILE SES_V_HOSTVARIABLE file_option indicator
    {
        ulpValidateHostValue( yyvsp,
                              HV_OUT_TYPE,
                              HV_FILE_CLOB,
                              ID_FALSE,
                              (SInt)6,
                              (SInt)4,
                              (SInt)234
                            );
    }
    ;

opt_into_ses_host_var_4emsql
    : /* empty */
    | TR_INTO out_host_var_list_4emsql
    {
        gUlpCodeGen.ulpGenRemoveQueryToken( $<strval>1 );
    }
    ;

out_1hostvariable_4emsql
    : SES_V_HOSTVARIABLE
    | SES_V_IDENTIFIER
    ;

out_host_var_list_4emsql
    : out_1hostvariable_4emsql indicator_opt
    {
        ulpValidateHostValue( yyvsp,
                              HV_OUT_TYPE,
                              HV_FILE_NONE,
                              ID_FALSE,
                              (SInt)2,
                              (SInt)1,
                              (SInt)1
                            );
    }
    | out_host_var_list_4emsql TS_COMMA out_1hostvariable_4emsql indicator_opt
    {
        ulpValidateHostValue( yyvsp,
                              HV_OUT_TYPE,
                              HV_FILE_NONE,
                              ID_FALSE,
                              (SInt)4,
                              (SInt)3,
                              (SInt)23
                            );
    }
    | SES_V_BLOB_FILE out_1hostvariable_4emsql file_option indicator
    {
        ulpValidateHostValue( yyvsp,
                              HV_OUT_TYPE,
                              HV_FILE_BLOB,
                              ID_FALSE,
                              (SInt)4,
                              (SInt)2,
                              (SInt)12
                            );
    }
    | SES_V_CLOB_FILE out_1hostvariable_4emsql file_option indicator
    {
        ulpValidateHostValue( yyvsp,
                              HV_OUT_TYPE,
                              HV_FILE_CLOB,
                              ID_FALSE,
                              (SInt)4,
                              (SInt)2,
                              (SInt)12
                            );
    }
    | out_host_var_list_4emsql TS_COMMA SES_V_BLOB_FILE out_1hostvariable_4emsql file_option indicator
    {
        ulpValidateHostValue( yyvsp,
                              HV_OUT_TYPE,
                              HV_FILE_BLOB,
                              ID_FALSE,
                              (SInt)6,
                              (SInt)4,
                              (SInt)234
                            );
    }
    | out_host_var_list_4emsql TS_COMMA SES_V_CLOB_FILE out_1hostvariable_4emsql file_option indicator
    {
        ulpValidateHostValue( yyvsp,
                              HV_OUT_TYPE,
                              HV_FILE_CLOB,
                              ID_FALSE,
                              (SInt)6,
                              (SInt)4,
                              (SInt)234
                            );
    }
    ;

/* BUG-46824 list 형태로 구성 예) :c1, :c2, :c3...*/
out_psm_host_var_list
    : out_psm_host_var
    | out_psm_host_var_list TS_COMMA out_psm_host_var
    ;

out_psm_host_var
    : SES_V_HOSTVARIABLE indicator_opt
    {
        ulpValidateHostValue( yyvsp,
                              HV_OUT_PSM_TYPE,
                              HV_FILE_NONE,
                              ID_TRUE,
                              (SInt)2,
                              (SInt)1,
                              (SInt)0
                            );
    }
    ;


file_option
    : option_keyword_opt SES_V_HOSTVARIABLE
    {
        ulpSymTElement *sSymNode;

        gUlpCodeGen.ulpGenRemoveQueryToken( $<strval>2 );

        // out host variable
        if ( ( sSymNode = gUlpScopeT.ulpSLookupAll( $<strval>2+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unknown_Hostvar_Error,
                             $<strval>2+1 );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
        else
        {
            idlOS::snprintf( gUlpFileOptName, MAX_HOSTVAR_NAME_SIZE * 2,
                             "%s", $<strval>2+1);
        }
    }
    ;

option_keyword_opt
    : /* empty */
    | APRE_V_OPTION
    {
        gUlpCodeGen.ulpGenRemoveQueryToken( $<strval>1 );
    }
    ;

indicator_opt
    : /* empty */
    | indicator
    ;

indicator
    : indicator_keyword_opt SES_V_HOSTVARIABLE
    {
        ulpSymTNode *sFieldSymNode;

        gUlpCodeGen.ulpGenRemoveQueryToken( $<strval>2 );
        if ( ( gUlpIndNode = gUlpScopeT.ulpSLookupAll( $<strval>2+1, gUlpCurDepth ) ) == NULL )
        {
            //host 변수 못찾는 error
            // error 처리

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Unknown_Hostvar_Error,
                             $<strval>2+1 );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
        else
        {
            /* BUG-28566: The indicator must be the type of SQLLEN or int or long(32bit). */
            if( (gUlpIndNode->mIsstruct   == ID_TRUE) &&
                (gUlpIndNode->mStructLink != NULL) )
            {   // indicator가 struct type이라면 모든 필드들은 int/long or SQLLEN type이어야한다.
                // indicator symbol node(gUlpIndNode)안의 struct node pointer(mStructLink)
                // 를 따라가 field hash table(mChild)의 symbol node(mInOrderList)를
                // 얻어온다.
                sFieldSymNode = gUlpIndNode->mStructLink->mChild->mInOrderList;

                // struct 안의 각 필드들의 type을 검사한다.
                while ( sFieldSymNode != NULL )
                {
                    switch ( sFieldSymNode->mElement.mType )
                    {
                        case H_INT:
                        case H_BLOBLOCATOR:
                        case H_CLOBLOCATOR:
                            break;
                        case H_LONG:
                            // indicator는 무조건 4byte이어야함.
                            if( ID_SIZEOF(long) != 4 )
                            {
                                // 잘못된 indicator type error 처리
                                ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                                 ulpERR_ABORT_COMP_Wrong_IndicatorType_Error,
                                                 sFieldSymNode->mElement.mName );
                                gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                                COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
                            }
                            break;
                        default:
                            // 잘못된 indicator type error 처리
                            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                             ulpERR_ABORT_COMP_Wrong_IndicatorType_Error,
                                             sFieldSymNode->mElement.mName );
                            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
                            break;
                    }
                    // 다음 field symbol node를 가리킨다.
                    sFieldSymNode = sFieldSymNode->mInOrderNext;
                }
            }
            else
            {   // struct type이 아니다.
                switch( gUlpIndNode->mType )
                {   // must be the type of SQLLEN or int or long(32bit).
                    case H_INT:
                    case H_BLOBLOCATOR:
                    case H_CLOBLOCATOR:
                        break;
                    case H_LONG:
                        // indicator는 무조건 4byte이어야함.
                        if( ID_SIZEOF(long) != 4 )
                        {
                            // 잘못된 indicator type error 처리
                            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                             ulpERR_ABORT_COMP_Wrong_IndicatorType_Error,
                                             sFieldSymNode->mElement.mName );
                            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
                        }
                        break;
                    default:
                        // 잘못된 indicator type error 처리
                        ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                         ulpERR_ABORT_COMP_Wrong_IndicatorType_Error,
                                         gUlpIndNode->mName );
                        gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                        COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
                        break;
                }
            }

            idlOS::snprintf( gUlpIndName, MAX_HOSTVAR_NAME_SIZE * 2,
                             "%s", $<strval>2+1);
        }
    }
    ;

indicator_keyword_opt
    : /* empty */
    | SES_V_INDICATOR
    {
        gUlpCodeGen.ulpGenRemoveQueryToken( $<strval>1 );
    }
    ;

tablespace_name_option
  : /* empty */
  | TA_TABLESPACE object_name
  ;

opt_table_part_desc
  : tablespace_name_option opt_lob_attribute_list opt_record_access
  ;

/* PROJ-2359 Table/Partition Access Option */
opt_record_access
  : /* empty */
  | record_access
  ;

record_access
  : TR_READ SES_V_ONLY
  | TR_READ TR_WRITE
  | TR_READ SES_V_IDENTIFIER
  {
      if ( idlOS::strncasecmp( "APPEND", $<strval>2, 6 ) != 0 )
      {
          ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                           ulpERR_ABORT_COMP_Unterminated_String_Error );
          gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
          COMPerror( ulpGetErrorMSG( &gUlpParseInfo.mErrorMgr ) );
      }
      else
      {
          /* Nothing to do */
      }
  }
  ;

opt_partition_lob_attr_list
  :
  | TS_OPENING_PARENTHESIS 
    partition_lob_attr_list
  TS_CLOSING_PARENTHESIS
  ;

partition_lob_attr_list
  : partition_lob_attr_list TS_COMMA partition_lob_attr
  | partition_lob_attr
  ;

partition_lob_attr
  : TR_PARTITION object_name lob_attribute_list
  ;

opt_partition_name
  : /* empty */
  | TR_PARTITION TS_OPENING_PARENTHESIS
    object_name TS_CLOSING_PARENTHESIS
  ;

SES_V_IN
  : TR_IN_BF_LPAREN
  | TR_IN
  ;

%%


int doCOMPparse( SChar *aFilename )
{
/***********************************************************************
 *
 * Description :
 *      COMPparse precompiling을 시작되게 하는 initial 함수.
 *
 ***********************************************************************/
    int sRes;

    ulpCOMPInitialize( aFilename );

    gUlpCOMPifstackMgr[++gUlpCOMPifstackInd] = new ulpPPifstackMgr();

    if( gDontPrint2file != ID_TRUE )
    {
        gUlpCodeGen.ulpGenInitPrint();

        /* BUG-42357 */
        if (gUlpProgOption.mOptLineMacro == ID_TRUE)
        {
            if (gUlpProgOption.mOptParseInfo == PARSE_FULL)
            {
                gUlpCodeGen.ulpGenSetCurFileInfo( COMPlineno, 0, gUlpProgOption.mInFile );
            }
            else
            {
                gUlpCodeGen.ulpGenSetCurFileInfo( COMPlineno, -1, gUlpProgOption.mInFile );
            }

            gUlpCodeGen.ulpGenPrintLineMacro();
        }
    }

    sRes = COMPparse( );

    gUlpCodeGen.ulpGenWriteFile();

    delete gUlpCOMPifstackMgr[gUlpCOMPifstackInd];

    gUlpCOMPifstackInd--;

    ulpCOMPFinalize();

    return sRes;
}


idBool ulpCOMPCheckArray( ulpSymTElement *aSymNode )
{
/***********************************************************************
 *
 * Description :
 *      array binding을 해야할지(isarr을 true로 set할지) 여부를 체크하기 위한 함수.
 *      struct A { int a[10]; } sA; 의 경우 isarr를 true로 set하고, isstruct를 
 *      false로 set하기 위해 사용됨.
 *
 ***********************************************************************/
    ulpSymTNode *sFieldSymNode;

    sFieldSymNode = aSymNode->mStructLink->mChild->mInOrderList;

    IDE_TEST( sFieldSymNode == NULL );

    while ( sFieldSymNode != NULL )
    {
        switch ( sFieldSymNode->mElement.mType )
        {
            case H_CLOB:
            case H_BLOB:
            case H_NUMERIC:
            case H_NIBBLE:
            case H_BIT:
            case H_BYTES:
            case H_BINARY:
            case H_BINARY2:  /* BUG-46418 */
            case H_CHAR:
            case H_NCHAR:
            case H_CLOB_FILE:
            case H_BLOB_FILE:
                IDE_TEST( sFieldSymNode->mElement.mArraySize2[0] == '\0' );
                break;
            case H_VARCHAR:
            case H_NVARCHAR:
                IDE_TEST( 1 );
                break;
            default:
                IDE_TEST( sFieldSymNode->mElement.mIsarray != ID_TRUE );
                break;
        }
        sFieldSymNode = sFieldSymNode->mInOrderNext;
    }

    return ID_TRUE;

    IDE_EXCEPTION_END;

    return ID_FALSE;
}


void ulpValidateHostValue( void         *yyvsp,
                           ulpHVarType   aInOutType,
                           ulpHVFileType aFileType,
                           idBool        aTransformQuery,
                           SInt          aNumofTokens,
                           SInt          aHostValIndex,
                           SInt          aRemoveTokIndexs )
{
    (void) ulpValidateHostValueWithDiagType(
                           yyvsp,
                           aInOutType,
                           aFileType,
                           aTransformQuery,
                           aNumofTokens,
                           aHostValIndex,
                           aRemoveTokIndexs, 
                           H_DIAG_UNKNOWN );
}

void ulpValidateHostValueWithDiagType(
                           void           *yyvsp,
                           ulpHVarType     aInOutType,
                           ulpHVFileType   aFileType,
                           idBool          aTransformQuery,
                           SInt            aNumofTokens,
                           SInt            aHostValIndex,
                           SInt            aRemoveTokIndexs, 
                           ulpHostDiagType aDiagType )
{
/***********************************************************************
 *
 * Description :
 *      host 변수가 유효한지 확인하며, 유효하다면 ulpGenHostVarList 에 추가한다.
 *      aNumofTokens는 총 토큰들의 수,
 *      aHostValIndex 는 호스트 변수가 몇번째 토큰에 위치하는지를 나타내며,
 *      aRemoveTokIndexs는 SQL쿼리변환시 몇번째 토큰에 위치하는 토큰들을 제거할지를 나타내준다.
 *      ex> aRemoveTokIndexs이 123이면 1,2,3 에 위치하는 토큰들을 제거해준다.
 *
 ***********************************************************************/
    SInt            sIndexs, sMod;
    SChar          *sFileOptName;
    ulpSymTElement *sSymNode;
    ulpGENhvType    sHVType;
    ulpGENhvType    sArrayStructType;

    switch( aInOutType )
    {
        case HV_IN_TYPE:
            sArrayStructType = IN_GEN_ARRAYSTRUCT;
            break;
        case HV_OUT_TYPE:
        case HV_OUT_PSM_TYPE:
            sArrayStructType = OUT_GEN_ARRAYSTRUCT;
            break;
        case HV_INOUT_TYPE:
            sArrayStructType = INOUT_GEN_ARRAYSTRUCT;
            break;
        default:
            sArrayStructType = GEN_GENERAL;
            break;
    }

    if ( sArrayStructType == INOUT_GEN_ARRAYSTRUCT )
    {
        if ( ((gUlpCodeGen.ulpGenGetEmSQLInfo()->mHostValueType) == IN_GEN_ARRAYSTRUCT) ||
             ((gUlpCodeGen.ulpGenGetEmSQLInfo()->mHostValueType) == OUT_GEN_ARRAYSTRUCT))
        {
            // error 처리
            // array struct type은 다른 host 변수와 같이 올수 없다.

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Repeat_Array_Struct_Error,
                             (*(((YYSTYPE *)yyvsp)[aHostValIndex - aNumofTokens].strval)==':')?
                             (((YYSTYPE *)yyvsp)[aHostValIndex - aNumofTokens].strval)+1:
                             (((YYSTYPE *)yyvsp)[aHostValIndex - aNumofTokens].strval)
                           );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }
    else
    {
        if ( (gUlpCodeGen.ulpGenGetEmSQLInfo()->mHostValueType)
             == sArrayStructType )
        {
            // error 처리
            // array struct type은 다른 host 변수와 같이 올수 없다.

            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_COMP_Repeat_Array_Struct_Error,
                             (*(((YYSTYPE *)yyvsp)[aHostValIndex - aNumofTokens].strval)==':')?
                             (((YYSTYPE *)yyvsp)[aHostValIndex - aNumofTokens].strval)+1:
                             (((YYSTYPE *)yyvsp)[aHostValIndex - aNumofTokens].strval)
                           );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }
    }

    // lookup host variable
    if ( ( sSymNode = gUlpScopeT.ulpSLookupAll(
                                    (((YYSTYPE *)yyvsp)[aHostValIndex - aNumofTokens].strval),
                                    gUlpCurDepth )
         ) == NULL
       )
    {
        //host 변수 못찾는 error
        // error 처리

        ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                         ulpERR_ABORT_COMP_Unknown_Hostvar_Error,
                         (*(((YYSTYPE *)yyvsp)[aHostValIndex - aNumofTokens].strval)==':')?
                         (((YYSTYPE *)yyvsp)[aHostValIndex - aNumofTokens].strval)+1:
                         (((YYSTYPE *)yyvsp)[aHostValIndex - aNumofTokens].strval)
                       );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
        COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
    }
    else
    {
        /* BUG-28788 : FOR절을 이용하여 struct pointer type의 array insert가 안됨  */
        if ( (gUlpCodeGen.ulpGenGetEmSQLInfo()->mIters[0] != '\0') && 
             (sSymNode->mPointer <= 0) )
        {
            /* BUG-44577 array or pointer type이 아닌데 FOR절이 왔다면 error를 report함. 
             * array or pointer type이 아니지만 struct type일 경우 struct안의 변수를 체크한다. */
            if ( sSymNode->mIsstruct == ID_TRUE )
            {
                /* BUG-44577 struct안에 배열 변수가 있는지 확인 */
                if ( ulpValidateFORStructArray(sSymNode) != IDE_SUCCESS)
                {
                    ulpSetErrorCode( &gUlpParseInfo.mErrorMgr, ulpERR_ABORT_FORstmt_Invalid_usage_Error );
                    gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                    COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
                }
            }
            else
            {
                ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                 ulpERR_ABORT_FORstmt_Invalid_usage_Error );
                gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
            }
        }
        else
        {
            /* pointer type */
        }

        // 호스트 변수들에 대해 struct,arraystruct type 설정.
        if ( sSymNode->mIsstruct == ID_TRUE )
        {
            if ( sSymNode->mArraySize[0] != '\0' )
            {
                // array struct

                /* BUG-32100 An indicator of arraystructure type should not be used for a hostvariable. */
                if (gUlpIndNode != NULL)
                {
                    // 구조체 배열을 호스트 변수로 사용하면, 인디케이터를 가질 수 없다.
                    ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                                     ulpERR_ABORT_COMP_Invalid_Indicator_Usage_Error );
                    gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
                    COMPerror( ulpGetErrorMSG( &gUlpParseInfo.mErrorMgr ) );
                }
                else
                {
                    sHVType = sArrayStructType;
                    gUlpCodeGen.ulpGenEmSQL( GEN_HVTYPE, (void *) &sHVType );
                }
            }
            else
            {
                if( ulpCOMPCheckArray ( sSymNode ) == ID_TRUE )
                {
                    // array
                    sHVType = GEN_ARRAY;
                    gUlpCodeGen.ulpGenEmSQL( GEN_HVTYPE, (void *) &sHVType );
                }
                else
                {
                    // struct
                    sHVType = GEN_STRUCT;
                    gUlpCodeGen.ulpGenEmSQL( GEN_HVTYPE, (void *) &sHVType );
                }
            }
        }

        /* TASK-7218 Handling Multiple Errors */
        sSymNode->mDiagType = aDiagType;
        if ( ulpValidateFORGetDiagnostics(sSymNode) != IDE_SUCCESS )
        {
            ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                             ulpERR_ABORT_Incompatible_Type_With_Diag_Item,
                             (*(((YYSTYPE *)yyvsp)[aHostValIndex - aNumofTokens].strval)==':')?
                             (((YYSTYPE *)yyvsp)[aHostValIndex - aNumofTokens].strval)+1:
                             (((YYSTYPE *)yyvsp)[aHostValIndex - aNumofTokens].strval)
                           );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &gUlpParseInfo.mErrorMgr );
            COMPerror( ulpGetErrorMSG(&gUlpParseInfo.mErrorMgr) );
        }

        // remove some tokens
        for( sIndexs = aRemoveTokIndexs; sIndexs > 0 ; sIndexs/=10 )
        {
            sMod = sIndexs%10;
            if (sMod > 0)
            {
                gUlpCodeGen.ulpGenRemoveQueryToken(
                        (((YYSTYPE *)yyvsp)[sMod - aNumofTokens].strval)
                                                  );
            }
        }

        // set value type for file mode
        switch(aFileType)
        {
            case HV_FILE_CLOB:
                sSymNode->mType = H_CLOB_FILE;
                sFileOptName = gUlpFileOptName;
                break;
            case HV_FILE_BLOB:
                sSymNode->mType = H_BLOB_FILE;
                sFileOptName = gUlpFileOptName;
                break;
            default:
                sFileOptName = NULL;
                break;
        }

        gUlpCodeGen.ulpGenAddHostVarList(
                        (((YYSTYPE *)yyvsp)[aHostValIndex- aNumofTokens].strval),
                        sSymNode ,
                        gUlpIndName,
                        gUlpIndNode,
                        sFileOptName,
                        aInOutType      );

        if ( sSymNode->mIsstruct == ID_TRUE )
        {
            IDE_ASSERT(sSymNode->mStructLink->mChild != NULL);
            gUlpCodeGen.ulpIncHostVarNum( sSymNode->mStructLink->mChild->mCnt );
            if( aTransformQuery == ID_TRUE )
            {
                gUlpCodeGen.ulpGenAddHostVarArr( sSymNode->mStructLink->mChild->mCnt );
            }
        }
        else
        {
            gUlpCodeGen.ulpIncHostVarNum( 1 );
            if( aTransformQuery == ID_TRUE )
            {
                gUlpCodeGen.ulpGenAddHostVarArr( 1 );
            }
        }
    }

    gUlpIndName[0] = '\0';
    gUlpIndNode    = NULL;

    switch(aFileType)
    {
        case HV_FILE_CLOB:
        case HV_FILE_BLOB:
            gUlpFileOptName[0] = '\0';
            break;
        default:
            break;
    }
}

/* =========================================================
 *  ulpValidateFORGetDiagnostics
 *
 *  Description :
 *     ulpValidateHostValue에서 호출되는 함수로써, 
 *     GET DIAGNOSTICS 문에 사용되는 host 변수를 체크한다.
 *     run-time시에 타입 호환 검사를 하면 에러가 발생하여도
 *     반환할 방법이 없으므로 precompile 시점에 검사를 함.
 *
 *  Parameters :  
 *     ulpSymTElement *aElement : 체크해야될 host 변수 정보
 * ========================================================*/
IDE_RC ulpValidateFORGetDiagnostics(ulpSymTElement *aElement)
{
    ulpHostType sType = aElement->mType;

    switch(aElement->mDiagType)
    {
    case H_STMT_DIAG_NUMBER:
    case H_STMT_DIAG_ROW_COUNT:
    case H_COND_DIAG_RETURNED_SQLCODE:
    case H_COND_DIAG_ROW_NUMBER:
    case H_COND_DIAG_COLUMN_NUMBER:
        IDE_TEST( sType != H_INTEGER && sType != H_INT );
        break;
    case H_COND_DIAG_RETURNED_SQLSTATE:
    case H_COND_DIAG_MESSAGE_TEXT:
        IDE_TEST( sType != H_CHAR && sType != H_VARCHAR );
        break;
    default:
        break;
    }
    return IDE_SUCCESS;
        
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* =========================================================
 *  ulpValidateFORStructArray
 *
 *  Description :
 *     ulpValidateHostValue에서 호출되는 함수들로, 
       FOR절에 사용되는 host 변수를 체크한다.
 *
 *  Parameters :  
 *     ulpSymTElement *aElement : 체크해야될 host 변수 정보
 * ========================================================*/
IDE_RC ulpValidateFORStructArray(ulpSymTElement *aElement)
{
    ulpStructTNode *sStructTNode;
    ulpSymTNode    *sSymTNode;
    ulpSymTElement *sFirstFieldNode;
    ulpSymTElement *sFieldNode;
    SInt            i;
        
    sStructTNode    = (ulpStructTNode*)aElement->mStructLink;
    
    /* BUG-44577 struct안에 변수가 없음 */
    IDE_TEST( sStructTNode->mChild->mCnt <= 0 );
    
    sSymTNode       = sStructTNode->mChild->mInOrderList;
    sFirstFieldNode = &(sSymTNode->mElement);
                
    IDE_TEST( (sFirstFieldNode->mIsstruct == ID_TRUE) || (sFirstFieldNode->mIsarray == ID_FALSE));
        
    /* BUG-44577 char type일 경우 무조건 2차원 배열이 와야 한다. */
    if ( (sFirstFieldNode->mType == H_CHAR)    ||
         (sFirstFieldNode->mType == H_VARCHAR) ||
         (sFirstFieldNode->mType == H_NCHAR)   ||
         (sFirstFieldNode->mType == H_NVARCHAR) )
    {
        IDE_TEST( (sFirstFieldNode->mArraySize[0] == '\0') || (sFirstFieldNode->mArraySize2[0] == '\0') );
    }
                
    for (i = 1; i < sStructTNode->mChild->mCnt; i++)
    {
        sSymTNode = sSymTNode->mInOrderNext;
        sFieldNode = &(sSymTNode->mElement);
        
        IDE_TEST( (sFieldNode->mIsstruct == ID_TRUE) || (sFieldNode->mIsarray == ID_FALSE));
        
        /* BUG-44577 char type일 경우 무조건 2차원 배열이 와야 한다. */
        if ( (sFirstFieldNode->mType == H_CHAR)    ||
             (sFirstFieldNode->mType == H_VARCHAR) ||
             (sFirstFieldNode->mType == H_NCHAR)   ||
             (sFirstFieldNode->mType == H_NVARCHAR) )
        {
            IDE_TEST( (sFirstFieldNode->mArraySize[0] == '\0') || (sFirstFieldNode->mArraySize2[0] == '\0') );
        }
        
        IDE_TEST( idlOS::strcmp(sFirstFieldNode->mArraySize, sFieldNode->mArraySize) != 0 );
    }

    return IDE_SUCCESS;
        
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


/*
 *  Member functions of the ulpParseInfo.
 *
 */

ulpParseInfo::ulpParseInfo()
{
    mSaveId              = ID_FALSE;
    mFuncDecl            = ID_FALSE;
    mStructDeclDepth     = 0;
    mArrDepth            = 0;
    mArrExpr             = ID_FALSE;
    mConstantExprStr[0]  = '\0';
    mStructPtr           = NULL;
    mHostValInfo4Typedef = NULL;
    mVarcharDecl         = ID_FALSE;
    /* BUG-27875 : 구조체안의 typedef type인식못함. */
    mSkipTypedef         = ID_FALSE;

    /* BUG-35518 Shared pointer should be supported in APRE */
    mIsSharedPtr         = ID_FALSE;
    IDU_LIST_INIT( &mSharedPtrVarList );

    IDU_LIST_INIT( &mVarcharVarList );

    /* BUG-28118 : system 헤더파일들도 파싱돼야함.                       *
     * 6th. problem : Nested structure 정의중 scope를 잘못 계산하는 문제 */
    mHostValInfo[mStructDeclDepth]
            = (ulpSymTElement *) idlOS::malloc( ID_SIZEOF( ulpSymTElement ) );

    if ( mHostValInfo[mStructDeclDepth] == NULL )
    {
        ulpSetErrorCode( &gUlpParseInfo.mErrorMgr,
                         ulpERR_ABORT_Memory_Alloc_Error );
        ulpPrintfErrorCode( stderr,
                            &gUlpParseInfo.mErrorMgr);
        IDE_ASSERT(0);
    }
    else
    {
        (void) ulpInitHostInfo();
    }
}


/* BUG-28118 : system 헤더파일들도 파싱돼야함.                       *
 * 6th. problem : Nested structure 정의중 scope를 잘못 계산하는 문제 */
void ulpParseInfo::ulpFinalize(void)
{
/***********************************************************************
 *
 * Description :
 *    malloc 된 mHostValInfo 배열이 free되지 않았다면 free해준다.
 *
 * Implementation :
 *
 ***********************************************************************/

    for( ; mStructDeclDepth >= 0 ; mStructDeclDepth-- )
    {
        idlOS::free( mHostValInfo[mStructDeclDepth] );
        mHostValInfo[mStructDeclDepth] = NULL;
    }
}


/* BUG-28118 : system 헤더파일들도 파싱돼야함.                       *
 * 6th. problem : Nested structure 정의중 scope를 잘못 계산하는 문제 */
void ulpParseInfo::ulpInitHostInfo( void )
{
/***********************************************************************
 *
 * Description :
 *    host 변수 정보 초기화 함수로 특정 host 변수를 파싱하면서 setting된
 *    변수에대한 정보를 파싱을 마친후 본함수가 호출되어 다시 초기화 해준다.
 * Implementation :
 *
 ***********************************************************************/
    mHostValInfo[mStructDeclDepth]->mName[0]       = '\0';
    mHostValInfo[mStructDeclDepth]->mType          = H_UNKNOWN;
    mHostValInfo[mStructDeclDepth]->mIsTypedef     = ID_FALSE;
    mHostValInfo[mStructDeclDepth]->mIsarray       = ID_FALSE;
    mHostValInfo[mStructDeclDepth]->mArraySize[0]  = '\0';
    mHostValInfo[mStructDeclDepth]->mArraySize2[0] = '\0';
    mHostValInfo[mStructDeclDepth]->mIsstruct      = ID_FALSE;
    mHostValInfo[mStructDeclDepth]->mStructName[0] = '\0';
    mHostValInfo[mStructDeclDepth]->mStructLink    = NULL;
    mHostValInfo[mStructDeclDepth]->mIssign        = ID_TRUE;
    mHostValInfo[mStructDeclDepth]->mPointer       = 0;
    mHostValInfo[mStructDeclDepth]->mAlloc         = ID_FALSE;
    mHostValInfo[mStructDeclDepth]->mMoreInfo      = 0;
    mHostValInfo[mStructDeclDepth]->mIsExtern      = ID_FALSE;
}


void ulpParseInfo::ulpCopyHostInfo4Typedef( ulpSymTElement *aD,
                                            ulpSymTElement *aS )
{
/***********************************************************************
 *
 * Description :
 *    typedef 구문 처리를 위한 ulpSymTElement copy 함수로, typedef 된 특정 type을
 *    선언시 사용할때 호출되어 해당 type에 대한 정보를 복사해줌.
 *   예)  typedef struct A { int a; };
          A sA;           <----   이경우 type A에 대한 정보를 변수 sA 정보에 복사해줌.
 * Implementation :
 *
 ***********************************************************************/
    // mIsTypedef, mName은 복사 대상이 아님.
    aD->mType     = aS->mType;
    aD->mIsarray  = aS->mIsarray;
    aD->mIsstruct = aS->mIsstruct;
    aD->mIssign   = aS->mIssign;
    aD->mPointer  = aS->mPointer;
    aD->mAlloc    = aS->mAlloc;
    aD->mIsExtern = aS->mIsExtern;
    if ( aS->mArraySize[0] != '\0' )
    {
        idlOS::strncpy( aD->mArraySize, aS->mArraySize, MAX_NUMBER_LEN - 1 );
    }
    if ( aS->mArraySize2[0] != '\0' )
    {
        idlOS::strncpy( aD->mArraySize2, aS->mArraySize2, MAX_NUMBER_LEN - 1 );
    }
    if ( aS->mStructName[0] != '\0' )
    {
        idlOS::strncpy( aD->mStructName, aS->mStructName, MAX_HOSTVAR_NAME_SIZE - 1 );
    }
    if ( aS->mStructLink != NULL )
    {
        aD->mStructLink  = aS->mStructLink;
    }
}


// for debug : print host variable info.
void ulpParseInfo::ulpPrintHostInfo(void)
{
    idlOS::printf( "\n=== hostvar info ===\n"
                   "mName       =[%s]\n"
                   "mType       =[%d]\n"
                   "mIsTypedef  =[%d]\n"
                   "mIsarray    =[%d]\n"
                   "mArraySize  =[%s]\n"
                   "mArraySize2 =[%s]\n"
                   "mIsstruct   =[%d]\n"
                   "mStructName =[%s]\n"
                   "mStructLink =[%d]\n"
                   "mIssign     =[%d]\n"
                   "mPointer    =[%d]\n"
                   "mAlloc      =[%d]\n"
                   "mIsExtern   =[%d]\n"
                   "====================\n",
                   mHostValInfo[mStructDeclDepth]->mName,
                   mHostValInfo[mStructDeclDepth]->mType,
                   mHostValInfo[mStructDeclDepth]->mIsTypedef,
                   mHostValInfo[mStructDeclDepth]->mIsarray,
                   mHostValInfo[mStructDeclDepth]->mArraySize,
                   mHostValInfo[mStructDeclDepth]->mArraySize2,
                   mHostValInfo[mStructDeclDepth]->mIsstruct,
                   mHostValInfo[mStructDeclDepth]->mStructName,
                   mHostValInfo[mStructDeclDepth]->mStructLink,
                   mHostValInfo[mStructDeclDepth]->mIssign,
                   mHostValInfo[mStructDeclDepth]->mPointer,
                   mHostValInfo[mStructDeclDepth]->mAlloc,
                   mHostValInfo[mStructDeclDepth]->mIsExtern );
}

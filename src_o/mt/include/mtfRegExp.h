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
 

/***********************************************************************
 * $Id: mtfRegExp.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_MTF_REGEXP_
#define _O_MTF_REGEXP_ 1

#define MTF_REGEXP_MAX_CHAR              0xFF
#define MTF_REGEXP_MAX_PATTERN_LENGTH   (1024)
#define MTF_REGEXP_MAX_MATCH_COUNT      ( MTF_REGEXP_MAX_PATTERN_LENGTH / 2 )

/*
 implements the following expressions
 
 \    Quote the next metacharacter
 ^    Match the beginning of the string
 .    Match any character
 $    Match the end of the string
 |    Alternation
 ()    Grouping (creates a capture)
 []    Character class
 
 
 ==GREEDY CLOSURES==
 *       Match 0 or more times
 +       Match 1 or more times
 ?       Match 1 or 0 times
 {n}    Match exactly n times
 {n,}   Match at least n times
 {n,m}  Match at least n but not more than m times
 
 
 ==ESCAPE CHARACTERS==
 \t        tab                   (HT, TAB)
 \n        newline               (LF, NL)
 \r        return                (CR)
 \f        form feed             (FF)
 
 
 ==PREDEFINED CLASSES==
 \l        lowercase next char
 \u        uppercase next char
 \a        letters
 \A        non letters
 \w        alphanimeric [0-9a-zA-Z_]
 \W        non alphanimeric
 \s        space
 \S        non space
 \d        digits
 \D        non nondigits
 \x        exadecimal digits
 \X        non exadecimal digits
 \c        control charactrs
 \C        non control charactrs
 \p        punctation
 \P        non punctation
 \b        word boundary
 \B        non word boundary
 
 
 ==POSIX []
 [:alnum:]  --> [A-Za-z0-9]
 [:alpha:]  --> \a
 [:blank:]  --> [ \t]
 [:cntrl:]  --> \c
 [:digit:]  --> \d
 [:graph:]  --> [\x21-\x7E]
 [:lower:]  --> \l
 [:print:]  --> [\x20-\x7E]
 [:punct:]  --> \p
 [:space:]  --> \s
 [:upper:]  --> \u
 [:word:]   --> \w
 [:xdigit:] --> \x
*/

typedef struct {
    const SChar *begin;
    SInt         len;
} mtfRegExpMatch;

enum mtfRegExpNodeEnum
{
    OP_GREEDY = (MTF_REGEXP_MAX_CHAR + 1), // * + ? {n} //256
    OP_OR,                                              //257
    OP_EXPR,                    //parentesis ()         //258
    OP_NOCAPEXPR,               //parentesis (?:)       //259
    OP_DOT,                                             //260
    OP_CLASS,                   // [                    //261
    OP_CCLASS,                  //\l \u \a \A \W ...    //262
    OP_NCLASS,                  //negates class the [^  //263
    OP_RANGE,                                           //264
    OP_CHAR,                                            //265
    OP_EOL,                                             //266
    OP_BOL,                                             //267
    OP_WB                       // word boundy '\b'     //268
};

#define MTF_REGEXP_SYMBOL_ANY_CHAR              ('.')
#define MTF_REGEXP_SYMBOL_GREEDY_ONE_OR_MORE    ('+')
#define MTF_REGEXP_SYMBOL_GREEDY_ZERO_OR_MORE   ('*')
#define MTF_REGEXP_SYMBOL_GREEDY_ZERO_OR_ONE    ('?')
#define MTF_REGEXP_SYMBOL_BRANCH                ('|')
#define MTF_REGEXP_SYMBOL_END_OF_STRING         ('$')
#define MTF_REGEXP_SYMBOL_BEGINNING_OF_STRING   ('^')
#define MTF_REGEXP_SYMBOL_ESCAPE_CHAR           ('\\')

typedef SInt mtfRegExpNodeType;

typedef struct mtfRegExpNode
{
    mtfRegExpNodeType type;
    SInt              left;
    SInt              right;
    SInt              next;
}mtfRegExpNode;

typedef struct mtfRegExpression
{
    const SChar    *p;             // pattern
    SChar           patternBuf[MTF_REGEXP_MAX_PATTERN_LENGTH];
    UShort          patternLen;
    SInt            first;         // first node
    SInt            nsize;         // count of node
    SInt            nsubexpr;
    mtfRegExpNode   nodes[1];
}mtfRegExpression;

// 최대 pattern 길의의 4배의 node를 생성할 수 있다.
#define MTF_REG_EXPRESSION_SIZE( aPatternLen ) \
    ( ID_SIZEOF(mtfRegExpression) + ID_SIZEOF(mtfRegExpNode) * (aPatternLen) * 4 )

typedef struct mtfRegSearch
{
    const SChar    *endOfLine;
    const SChar    *beginOfLine;
    mtfRegExpMatch  matches[MTF_REGEXP_MAX_MATCH_COUNT];
    SInt            currsubexp;
}mtfRegSearch;


class mtfRegExp {
private:
    static IDE_RC expList( mtfRegExpression *aExp, SInt *aOutNodeId );
    static SInt newNode( mtfRegExpression *aExp, mtfRegExpNodeType aNodeType );
    static IDE_RC escapeChar( mtfRegExpression *aExp, SInt *aOutEscapeChar );
    static SInt charClass( mtfRegExpression *aExp, SInt aClassId );
    static IDE_RC charNode( mtfRegExpression *aExp,
                            idBool            aIsClass,
                            SInt             *aOutNodeId );
    static IDE_RC expClass( mtfRegExpression *aExp, SInt *aOutNodeId );
    static IDE_RC parseNumber(mtfRegExpression *aExp, SInt *aOutNumber );
    static IDE_RC element( mtfRegExpression *aExp, SInt *aOutNodeId );
    static SInt matchCClass( SInt aCClass, SChar aMatchChar );
    static idBool matchClass( mtfRegExpression  *aExp,
                              mtfRegExpNode     *aNode,
                              SChar              aMatchChar );
    static const SChar *matchNode( mtfRegExpression *aExp,
                                   mtfRegSearch     *aSearch,
                                   mtfRegExpNode    *aNode,
                                   const SChar      *aStr,
                                   mtfRegExpNode    *aNextNode );
    
    static SInt isAlnum( UChar aChar );
    static SInt isAlpha( UChar aChar );
    static SInt isSpace( UChar aChar );
    static SInt isCntrl( UChar aChar );
    static SInt isDigit( UChar aChar );
    static SInt isLower( UChar aChar );
    static SInt isUpper( UChar aChar );
    static SInt isPrint( UChar aChar );
    static SInt isXdigit( UChar aChar );
    static SInt isPunct( UChar aChar );

public:
    static IDE_RC expCompile( mtfRegExpression *aExp,
                              const SChar      *aPatternStr,
                              SInt              aPatternLen );
    static idBool match( mtfRegExpression *aExp,
                         mtfRegSearch     *aSearch,
                         const SChar      *aText,
                         SInt              aTextLength );
    static idBool search( mtfRegExpression *aExp,
                          const SChar      *aText,
                          SInt              aTextLength,
                          const SChar     **aOutBegin,
                          const SChar     **aOutEnd );
    static idBool searchRange( mtfRegExpression  *aExp,
                               mtfRegSearch      *aSearch,
                               const SChar       *aTextBegin,
                               const SChar       *aTextEnd,
                               const SChar      **aOutBegin,
                               const SChar      **aOutEnd );
    static SInt getSubExpCount( mtfRegExpression *aExp );
    static idBool getSubExp( mtfRegExpression *aExp,
                             mtfRegSearch     *aSearch,
                             SInt              aSubMathcIndex,
                             mtfRegExpMatch   *aSubMatch );
};

#endif /* _O_MTF_REGEXP_ */

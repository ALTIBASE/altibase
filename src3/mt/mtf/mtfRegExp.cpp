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
 * $Id: mtfRegExp.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtfRegExp.h>

/**
 * create mtfRegExpNode and set default value type, left, right, next
 *
 * caller : charClass(), charNode(), element(), expClass(), expCompile(),
 *          expCompile4Const(), expList()
 * callee :
 *
 * @param mtfRegExpression *aExp      : the compiled expression
 * @param mtfRegExpNodeType aNodeType : node type
 * 
 * @return SInt new node id
 */
SInt mtfRegExp::newNode( mtfRegExpression *aExp, mtfRegExpNodeType aNodeType )
{
    mtfRegExpNode   sNode;
    SInt            sNewNodeId;
    sNode.type = aNodeType;
    sNode.next = sNode.right = sNode.left = -1;
    
    if ( aNodeType == OP_EXPR )
    {
        sNode.right = aExp->nsubexpr++;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_DASSERT( aExp->nsize < aExp->patternLen * 4 );
    
    aExp->nodes[aExp->nsize++] = sNode;
    sNewNodeId = aExp->nsize - 1;
    
    return (SInt)sNewNodeId;
}

/**
 * find escape char '\' and next char 'v','n','t','r','f' convert escape char 
 * otherwise not convert
 * ex) '\v' '\n' '\t' '\r' '\f'
 *
 * caller : expClass()
 * callee :
 *
 * @param mtfRegExpression *aExp          : the compiled expression
 * @param SInt *aOutEscapeChar            : a converted escape char
 *
 * @return IDE_SUCCESS or IDE_FAILURE
 */
IDE_RC mtfRegExp::escapeChar( mtfRegExpression *aExp, SInt *aOutEscapeChar )
{
    SChar sRet;
    if ( *aExp->p == MTF_REGEXP_SYMBOL_ESCAPE_CHAR ) // '\\'
    {
        aExp->p++;
        switch( *aExp->p )
        {
            case 'v':
                aExp->p++;
                sRet = '\v';
                break;
            case 'n':
                aExp->p++;
                sRet = '\n';
                break;
            case 't':
                aExp->p++;
                sRet = '\t';
                break;
            case 'r':
                aExp->p++;
                sRet = '\r';
                break;
            case 'f':
                aExp->p++;
                sRet = '\f';
                break;
            default:
                sRet = ( *aExp->p++ );
                break;
        }
    }
    else
    {
        sRet = ( *aExp->p++ );
    }
    
    *aOutEscapeChar = sRet;
    
    return IDE_SUCCESS;
}

/**
 * create a OP_CCLASS node
 * left of node set aClassId l u a A w W s S d D x X c C p
 *
 * caller : charNode()
 * callee : newNode()
 *
 * @param mtfRegExpression *aExp  : the compiled expression
 * @param SInt aClassId           : ClassId ex) l u a A w W ...
 *
 * @return new node id
 */

SInt mtfRegExp::charClass( mtfRegExpression *aExp, SInt aClassId )
{
    SInt sNewNodeId;
    
    sNewNodeId = newNode( aExp, OP_CCLASS );
    aExp->nodes[sNewNodeId].left = aClassId;
    
    return sNewNodeId;
}

/**
 * find escape char '\' and next char 'v','n','t','r','f' convert escape char
 * ex) '\v' '\n' '\t' '\r' '\f'
 * and predefined classes ex \l, \u \a \A \w \W \s \S \d \D \x \X \c \C \p \P \b \B
 *
 * caller : expClass(), element()
 * callee : charClss(), newNode()
 *
 * @param mtfRegExpression *aExp          : the compiled expression
 * @param SInt *aOutEscapeChar            : a converted escape char
 *
 * @return IDE_SUCCESS or IDE_FAILURE
 */
IDE_RC mtfRegExp::charNode( mtfRegExpression *aExp,
                            idBool            aIsClass,
                            SInt             *aOutNodeId )
{
    SChar sNodeType;
    SInt  sNodeId;
    
    if ( *aExp->p == MTF_REGEXP_SYMBOL_ESCAPE_CHAR )
    {
        aExp->p++;
        switch( *aExp->p )
        {
            case 'n':
                aExp->p++;
                sNodeId = newNode( aExp, '\n' );
                IDE_CONT( NORMAL_EXIT );
            case 't':
                aExp->p++;
                sNodeId = newNode( aExp, '\t' );
                IDE_CONT( NORMAL_EXIT );
            case 'r':
                aExp->p++;
                sNodeId = newNode( aExp, '\r' );
                IDE_CONT( NORMAL_EXIT );
            case 'f':
                aExp->p++;
                sNodeId = newNode( aExp, '\f' );
                IDE_CONT( NORMAL_EXIT );
            case 'v':
                aExp->p++;
                sNodeId = newNode( aExp, '\v' );
                IDE_CONT( NORMAL_EXIT );
            case 'a':
            case 'A':
            case 'w':
            case 'W':
            case 's':
            case 'S':
            case 'd':
            case 'D':
            case 'x':
            case 'X':
            case 'c':
            case 'C':
            case 'p':
            case 'P':
            case 'l':
            case 'u':
                sNodeType = *aExp->p;
                aExp->p++;
                sNodeId = charClass( aExp, sNodeType );
                IDE_CONT( NORMAL_EXIT );
            case 'b':
            case 'B':
                if ( aIsClass == ID_FALSE )
                {
                    sNodeId = newNode( aExp, OP_WB );
                    aExp->nodes[sNodeId].left = *aExp->p;
                    aExp->p++;
                    IDE_CONT( NORMAL_EXIT );
                } // else default
                else
                {
                    /* Nothing to do */
                }

            default:
                sNodeType = *aExp->p;
                aExp->p++;
                sNodeId = newNode( aExp, sNodeType );
                IDE_CONT( NORMAL_EXIT );
        }
    }
    else
    {
        sNodeType = *aExp->p;
        aExp->p++;
        sNodeId = newNode( aExp, sNodeType );
    }
    
    IDE_EXCEPTION_CONT( NORMAL_EXIT );
    
    *aOutNodeId = sNodeId;
    
    return IDE_SUCCESS;
}

/**
 * this function called by element()
 * '[' after
 * 1. [^  --> negates class : create OP_NCLASS otherwise OP_CLASS
 * 2. range [ - ]
 *
 * caller : element()
 * callee : charNode(), escapeChar() ,newNode()
 *
 * @param mtfRegExpression *aExp          : the compiled expression
 * @param SInt *aOutNodeId                : new node id
 *
 * @return IDE_SUCCESS or IDE_FAILURE
 */
IDE_RC mtfRegExp::expClass( mtfRegExpression *aExp, SInt *aOutNodeId )
{
    SInt sRet   = -1;
    SInt sFirst = -1;
    SInt sChain = 0;

    SInt sNodeId;
    SInt sNodeType;
    SInt sTemp;
    
    SInt sTempChain;
    
    if ( *aExp->p == MTF_REGEXP_SYMBOL_BEGINNING_OF_STRING ) // '^'
    {
        sRet = newNode( aExp, OP_NCLASS ); // [^ ==> negates class
        aExp->p++;
    }
    else
    {
        sRet = newNode( aExp, OP_CLASS ); // character calss [
    }
    
    /* POSIX expression */
    if ( *aExp->p == ':' )
    {
        switch ( *( aExp->p + 1 ) )
        {
            /* aExp->p 성능을 위해 NULL검사를 하지 않는다. */
            case 'a':
                if ( ( *(aExp->p +2) == 'l' ) &&    //[:alnum:]
                     ( *(aExp->p +3) == 'n' ) &&
                     ( *(aExp->p +4) == 'u' ) &&
                     ( *(aExp->p +5) == 'm' ) &&
                     ( *(aExp->p +6) == ':' ) )
                {
                    //[A-Za-z0-9]
                    sTemp = newNode( aExp, 'A' );
                    sTemp = newNode( aExp, OP_RANGE );
                    aExp->nodes[sRet].next = sTemp; // [ node chain

                    aExp->nodes[sTemp].left = 'A';
                    aExp->nodes[sTemp].right = 'Z';
                    aExp->nodes[sTemp].next = sTemp + 2; // range ndoe chain

                    sTemp = newNode( aExp, 'a' );
                    sTemp = newNode( aExp, OP_RANGE );
                    aExp->nodes[sTemp].left = 'a';
                    aExp->nodes[sTemp].right = 'z';
                    aExp->nodes[sTemp].next = sTemp + 2; // range ndoe chain

                    sTemp = newNode( aExp, '0' );
                    sTemp = newNode( aExp, OP_RANGE );
                    aExp->nodes[sTemp].left = '0';
                    aExp->nodes[sTemp].right = '9';
                    
                    sChain = sTemp;
                    
                    aExp->p += 7;
                }
                else
                {
                    if ( ( *(aExp->p +2) == 'l' ) &&    //[:alpha:]
                         ( *(aExp->p +3) == 'p' ) &&
                         ( *(aExp->p +4) == 'h' ) &&
                         ( *(aExp->p +5) == 'a' ) &&
                         ( *(aExp->p +6) == ':' ) )
                    {
                        //[A-Za-z] or \a
                        sTemp = charClass( aExp, 'a' );
                        aExp->nodes[sRet].next = sTemp;
                    
                        sChain = sTemp;
                        aExp->p += 7;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                break;
            case 'b':
                if ( ( *(aExp->p +2) == 'l' ) &&    //[:blank:]
                     ( *(aExp->p +3) == 'a' ) &&
                     ( *(aExp->p +4) == 'n' ) &&
                     ( *(aExp->p +5) == 'k' ) &&
                     ( *(aExp->p +6) == ':' ) )
                {
                    // [ \t]
                    sTemp = newNode( aExp, ' ' );
                    aExp->nodes[sRet].next = sTemp;
                    sTemp = newNode( aExp, '\t' );
                    
                    sChain = sTemp;
                    aExp->p += 7;
                }
                else
                {
                    /* Nothing to do */
                }
                break;
            case 'c':
                if ( ( *(aExp->p +2) == 'n' ) &&    //[:cntrl:]
                     ( *(aExp->p +3) == 't' ) &&
                     ( *(aExp->p +4) == 'r' ) &&
                     ( *(aExp->p +5) == 'l' ) &&
                     ( *(aExp->p +6) == ':' ) )
                {
                    //[\x00-\x1F\x7F] or \c
                    sTemp = charClass( aExp, 'c' );
                    aExp->nodes[sRet].next = sTemp;
                    
                    sChain = sTemp;
                    aExp->p += 7;
                }
                else
                {
                    /* Nothing to do */
                }
                break;
            case 'd':
                if ( ( *(aExp->p +2) == 'i' ) &&    //[:digit:]
                     ( *(aExp->p +3) == 'g' ) &&
                     ( *(aExp->p +4) == 'i' ) &&
                     ( *(aExp->p +5) == 't' ) &&
                     ( *(aExp->p +6) == ':' ) )
                {
                    //[0-9] or \d
                    sTemp = charClass( aExp, 'd' );
                    aExp->nodes[sRet].next = sTemp;
                    
                    sChain = sTemp;
                    aExp->p += 7;
                }
                else
                {
                    /* Nothing to do */
                }
                break;
            case 'g':
                if ( ( *(aExp->p +2) == 'r' ) &&    //[:graph:]
                     ( *(aExp->p +3) == 'a' ) &&
                     ( *(aExp->p +4) == 'p' ) &&
                     ( *(aExp->p +5) == 'h' ) &&
                     ( *(aExp->p +6) == ':' ) )
                {
                    //[\x21-\x7E]
                    sTemp = charClass( aExp, 'x' );  // \x
                    aExp->nodes[sRet].next = sTemp; // [ node chain
                    aExp->nodes[sTemp].next = sTemp + 1;
                    
                    sTemp = newNode( aExp, '2' );
                    aExp->nodes[sTemp].next = sTemp + 2;
                    sTemp = newNode( aExp, '1' );
                    
                    sTemp = newNode( aExp, OP_RANGE );
                    aExp->nodes[sTemp].left = '1';
                    aExp->nodes[sTemp].right = 'x';
                    aExp->nodes[sTemp].next = sTemp + 1;
                    
                    sTemp = newNode( aExp, '7' );
                    aExp->nodes[sTemp].next = sTemp + 1;
                    
                    sTemp = newNode( aExp, 'E' );
                    
                    sChain = sTemp;
                    aExp->p += 7;
                }
                else
                {
                    /* Nothing to do */
                }
                break;
            case 'l':
                if ( ( *(aExp->p +2) == 'o' ) &&    //[:lower:]
                     ( *(aExp->p +3) == 'w' ) &&
                     ( *(aExp->p +4) == 'e' ) &&
                     ( *(aExp->p +5) == 'r' ) &&
                     ( *(aExp->p +6) == ':' ) )
                {
                    //[a-z] or \l
                    sTemp = charClass( aExp, 'l' );
                    aExp->nodes[sRet].next = sTemp;
                    
                    sChain = sTemp;
                    aExp->p += 7;
                }
                else
                {
                    /* Nothing to do */
                }
                break;
            case 'p':
                if ( ( *(aExp->p +2) == 'r' ) &&    //[:print:]
                     ( *(aExp->p +3) == 'i' ) &&
                     ( *(aExp->p +4) == 'n' ) &&
                     ( *(aExp->p +5) == 't' ) &&
                     ( *(aExp->p +6) == ':' ) )
                {
                    //[\x20-\x7E]
                    sTemp = charClass( aExp, 'x' );  // \x
                    aExp->nodes[sRet].next = sTemp; // [ node chain
                    aExp->nodes[sTemp].next = sTemp + 1;
                    
                    sTemp = newNode( aExp, '2' );
                    aExp->nodes[sTemp].next = sTemp + 2;
                    sTemp = newNode( aExp, '0' );
                    
                    sTemp = newNode( aExp, OP_RANGE );
                    aExp->nodes[sTemp].left = '0';
                    aExp->nodes[sTemp].right = 'x';
                    aExp->nodes[sTemp].next = sTemp + 1;
                    
                    sTemp = newNode( aExp, '7' );
                    aExp->nodes[sTemp].next = sTemp + 1;
                    
                    sTemp = newNode( aExp, 'E' );
                    sChain = sTemp;
                    aExp->p += 7;
                }
                else
                {
                    if ( ( *(aExp->p +2) == 'u' ) &&    //[:punct:]
                         ( *(aExp->p +3) == 'n' ) &&
                         ( *(aExp->p +4) == 'c' ) &&
                         ( *(aExp->p +5) == 't' ) &&
                         ( *(aExp->p +6) == ':' ) )
                    {
                        //[][!"#$%&'()*+,./:;<=>?@\^_`{|}~-] \p
                        sTemp = charClass( aExp, 'p' );
                        aExp->nodes[sRet].next = sTemp;
                        aExp->p += 7;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                break;
            case 's':
                if ( ( *(aExp->p +2) == 'p' ) &&    //[:space:]
                     ( *(aExp->p +3) == 'a' ) &&
                     ( *(aExp->p +4) == 'c' ) &&
                     ( *(aExp->p +5) == 'e' ) &&
                     ( *(aExp->p +6) == ':' ) )
                {
                    //[ \t\r\n\v\f] or \s
                    sTemp = charClass( aExp, 's' );
                    aExp->nodes[sRet].next = sTemp;
                    sChain = sTemp;
                    aExp->p += 7;
                }
                else
                {
                    /* Nothing to do */
                }
                break;
            case 'u':
                if ( ( *(aExp->p +2) == 'p' ) &&    //[:upper:]
                     ( *(aExp->p +3) == 'p' ) &&
                     ( *(aExp->p +4) == 'e' ) &&
                     ( *(aExp->p +5) == 'r' ) &&
                     ( *(aExp->p +6) == ':' ) )
                {
                    //[A-Z] or \u
                    sTemp = charClass( aExp, 'u' );
                    aExp->nodes[sRet].next = sTemp;
                    sChain = sTemp;
                    aExp->p += 7;
                }
                else
                {
                    /* Nothing to do */
                }
                break;
            case 'w':
                if ( ( *(aExp->p +2) == 'o' ) &&    //[:word:] // non standard
                     ( *(aExp->p +3) == 'r' ) &&
                     ( *(aExp->p +4) == 'd' ) &&
                     ( *(aExp->p +5) == ':' ) )
                {
                    //[A-Za-z0-9_] or \w
                    sTemp = charClass( aExp, 'w' );
                    aExp->nodes[sRet].next = sTemp;
                    sChain = sTemp;
                    aExp->p += 6;
                }
                else
                {
                    /* Nothing to do */
                }
                break;
            case 'x':
                if ( ( *(aExp->p +2) == 'd' ) &&    //[:xdigit:]
                     ( *(aExp->p +3) == 'i' ) &&
                     ( *(aExp->p +4) == 'g' ) &&
                     ( *(aExp->p +5) == 'i' ) &&
                     ( *(aExp->p +6) == 't' ) &&
                     ( *(aExp->p +7) == ':' ) )
                {
                    //[A-Fa-f0-9] or \x
                    sTemp = charClass( aExp, 'x' );
                    aExp->nodes[sRet].next = sTemp;
                    sChain = sTemp;
                    aExp->p += 8;
                }
                else
                {
                    /* Nothing to do */
                }
                break;
            default:
                break;
        }
    }
    else
    {
        IDE_TEST_RAISE( *aExp->p == ']', ERR_REGEXP_REQUIRED_CLASS_EMPTY );
        sChain = sRet;
    }
    
    while ( ( *aExp->p != ']' ) && ( *aExp->p != '\0' ) )
    {
        if ( ( *aExp->p == '-' ) && ( sFirst != -1 ) ) // range process
        {
            IDE_TEST_RAISE( *aExp->p++ == ']', ERR_REGEXP_UNFINISHED_RANGE );
            
            sNodeId = newNode( aExp, OP_RANGE ); // rnage node ex) [a-z]
            IDE_TEST_RAISE( sFirst > *aExp->p, ERR_REGEXP_INVALID_RANGE );
            
            IDE_TEST_RAISE( aExp->nodes[sFirst].type == OP_CCLASS,
                           ERR_REGEXP_CLASS_INVALID_CHAR );
            
            aExp->nodes[sNodeId].left = aExp->nodes[sFirst].type;
            
            IDE_TEST( escapeChar( aExp, &sNodeType ) != IDE_SUCCESS );
            
            aExp->nodes[sNodeId].right = sNodeType;
            aExp->nodes[sChain].next = sNodeId;
            sChain = sNodeId;
            sFirst = -1;
        }
        else
        {
            if ( sFirst != -1 )
            {
                sTempChain = sFirst;
                aExp->nodes[sChain].next = sTempChain;
                sChain = sTempChain;
                IDE_TEST( charNode( aExp, ID_TRUE, &sFirst ) != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( charNode( aExp, ID_TRUE, &sFirst ) != IDE_SUCCESS );
            }
        }
    }
    if ( sFirst != -1 )
    {
        sTempChain = sFirst;
        aExp->nodes[sChain].next = sTempChain;
        sChain = sTempChain;
        sFirst = -1;
    }
    else
    {
        /* Nothing to do */
    }

    /* hack? */
    aExp->nodes[sRet].left = aExp->nodes[sRet].next;
    aExp->nodes[sRet].next = -1;
    
    *aOutNodeId = sRet;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_REGEXP_REQUIRED_CLASS_EMPTY )
    IDE_SET( ideSetErrorCode( mtERR_ABORT_REGEXP_CLASS_EMPTY ) );
    
    IDE_EXCEPTION( ERR_REGEXP_UNFINISHED_RANGE )
    IDE_SET( ideSetErrorCode( mtERR_ABORT_REGEXP_UNFINISHED_RANGE ) );
    
    IDE_EXCEPTION( ERR_REGEXP_INVALID_RANGE )
    IDE_SET( ideSetErrorCode( mtERR_ABORT_REGEXP_INVALID_RANGE ) );

    IDE_EXCEPTION( ERR_REGEXP_CLASS_INVALID_CHAR )
    IDE_SET( ideSetErrorCode( mtERR_ABORT_REGEXP_CLASS_INVALID_CHAR ) );
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/**
 * parse number
 *
 * caller : element()
 * callee :
 *
 * @param mtfRegExpression *aExp          : the compiled expression
 * @param SInt *aOutNumber                : a converted number value
 *
 * @return IDE_SUCCESS or IDE_FAILURE
 */
IDE_RC mtfRegExp::parseNumber( mtfRegExpression *aExp, SInt *aOutNumber )
{
    SInt sRet;
    SInt sPosition = 10;
    
    sRet = *aExp->p - '0';
    
    aExp->p++;
    
    while ( isDigit( *aExp->p ) )
    {
        sRet = sRet * 10 + ( *aExp->p++ - '0' );
        IDE_TEST_RAISE( sPosition == 1000000000, ERR_REGEXP_OVERFLOW );
        sPosition *= 10;
    };
    
    *aOutNumber = sRet;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_REGEXP_OVERFLOW )
    IDE_SET( ideSetErrorCode( mtERR_ABORT_REGEXP_CONST_OVERFLOW ) );
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/**
 * '(' '[' '$' '.' '*' '+' '?' '{n}' '{n,}' '{n,m}'  analysis and make node
 * etc character
 *
 * caller : expList(), element()
 * callee : expClass(), newNode(), parseNumber(), expList(), element()
 *
 * @param mtfRegExpression *aExp          : the compiled expression
 * @param SInt *aOutNodeId                : new node id
 *
 * @return IDE_SUCCESS or IDE_FAILURE
 */
IDE_RC mtfRegExp::element( mtfRegExpression *aExp, SInt *aOutNodeId )
{
    SInt   sRet = -1;
    
    idBool sIsGreedy = ID_FALSE;
    UShort sMin =0;
    UShort sMax =0;
    SInt   sNum;
    
    SInt   sExpr;
    SInt   sNewNode;
    
    SInt   sNewGreedyNode;
    SInt   sNewElementNode;

    switch( *aExp->p )
    {
        case '(':                                               // subexpression caputure
            aExp->p++;
            
            if ( *aExp->p == '?' )                              // subexpression no capture
            {
                aExp->p++;

                IDE_TEST_RAISE( (*aExp->p) != ':', ERR_REGEXP_REQUIRED_PAREN );
                aExp->p++;
                
                sExpr = newNode( aExp, OP_NOCAPEXPR );
            }
            else
            {
                sExpr = newNode( aExp, OP_EXPR );
            }
            IDE_TEST( expList( aExp, &sNewNode ) != IDE_SUCCESS );
            aExp->nodes[sExpr].left = sNewNode;
            sRet = sExpr;

            IDE_TEST_RAISE( (*aExp->p) != ')', ERR_REGEXP_REQUIRED_PAREN );
            aExp->p++;
            break;
        case '[':
            aExp->p++;
            IDE_TEST( expClass( aExp, &sRet ) != IDE_SUCCESS );
            
            IDE_TEST_RAISE( (*aExp->p) != ']', ERR_REGEXP_REQUIRED_PAREN );
            aExp->p++;
            break;
        case MTF_REGEXP_SYMBOL_END_OF_STRING:                   //'$'
            aExp->p++;
            sRet = newNode( aExp, OP_EOL );
            break;
        case MTF_REGEXP_SYMBOL_ANY_CHAR:                        //'.'
            aExp->p++;
            sRet = newNode( aExp, OP_DOT );
            break;
        default:
            IDE_TEST( charNode( aExp, ID_FALSE, &sRet ) != IDE_SUCCESS );
            break;
    }
    
    // GREEDY CLOSURES PROCESS
    switch ( *aExp->p )
    {
        case MTF_REGEXP_SYMBOL_GREEDY_ZERO_OR_MORE:  //'*'  0 ~ 0xFFFF
            sMin = 0;
            sMax = 0xFFFF;
            aExp->p++;
            sIsGreedy = ID_TRUE;
            break;
        case MTF_REGEXP_SYMBOL_GREEDY_ONE_OR_MORE:   //'+'  1 ~ 0xFFFF
            sMin = 1;
            sMax = 0xFFFF;
            aExp->p++;
            sIsGreedy = ID_TRUE;
            break;
        case MTF_REGEXP_SYMBOL_GREEDY_ZERO_OR_ONE:   //'?'  0 ~ 1
            sMin = 0;
            sMax = 1;
            aExp->p++;
            sIsGreedy = ID_TRUE;
            break;
        case '{':                                    // {m} ,{m,}, {m,n}
            aExp->p++;
            IDE_TEST_RAISE( !isDigit( *aExp->p ),
                            ERR_REGEXP_REQUIRED_NUMBER );
            
            IDE_TEST( parseNumber( aExp, &sNum ) != IDE_SUCCESS );
            sMin = (UShort)sNum;
            /*******************************/
            switch( *aExp->p )
            {
                case '}':
                    sMax = sMin; aExp->p++;             //{m} m ~ m
                    break;
                case ',':
                    aExp->p++;
                    sMax = 0xFFFF;                    //{m,} m ~ 0xFFFF
                    if ( isDigit( *aExp->p ) )
                    {
                        IDE_TEST( parseNumber( aExp, &sNum ) != IDE_SUCCESS );
                        sMax = (UShort)sNum;
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    IDE_TEST_RAISE( (*aExp->p) != '}', ERR_REGEXP_REQUIRED_PAREN );
                    aExp->p++;
                    break;
                default:
                    IDE_RAISE( ERR_REGEXP_REQUIRED_COMMA );
            }
            /*******************************/
            sIsGreedy = ID_TRUE;
            break;
    }/*switch ( *aExp->p ) */
    
    if ( sIsGreedy == ID_TRUE )
    {
        sNewGreedyNode = newNode( aExp, OP_GREEDY );
        aExp->nodes[sNewGreedyNode].left = sRet;
        aExp->nodes[sNewGreedyNode].right = ( (sMin) << 16 ) | sMax;
        sRet = sNewGreedyNode;
    }
    else
    {
        /* Nothing to do */
    }
    
    if ( ( *aExp->p != MTF_REGEXP_SYMBOL_BRANCH ) &&              // '|'
         ( *aExp->p != ')' ) &&
         ( *aExp->p != MTF_REGEXP_SYMBOL_GREEDY_ZERO_OR_MORE ) && // '*'
         ( *aExp->p != MTF_REGEXP_SYMBOL_GREEDY_ONE_OR_MORE ) &&  // '+'
         ( *aExp->p != '\0') )
    {
        IDE_TEST( element( aExp, &sNewElementNode ) != IDE_SUCCESS );
        aExp->nodes[sRet].next = sNewElementNode;
    }
    else
    {
        /* Nothing to do */
    }

    *aOutNodeId = sRet;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REGEXP_REQUIRED_PAREN )
    IDE_SET( ideSetErrorCode( mtERR_ABORT_REGEXP_REQUIRED_PAREN ) );
    
    IDE_EXCEPTION( ERR_REGEXP_REQUIRED_NUMBER )
    IDE_SET( ideSetErrorCode( mtERR_ABORT_REGEXP_REQUIRED_NUMBER ) );
    
    IDE_EXCEPTION( ERR_REGEXP_REQUIRED_COMMA )
    IDE_SET( ideSetErrorCode( mtERR_ABORT_REGEXP_REQUIRED_COMMA ) );
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/**
 * this function start of compile expression
 * ^ , 
 * element('(' '[' '$' '.' '*' '+' '?' '{n}' '{n,}' '{n,m}') 
 * |
 * caller : expList(), element(), compile(), compile4Const()
 * callee : newNode(), expList(), element()
 *
 * @param mtfRegExpression *aExp          : the compiled expression
 * @param SInt *aOutNodeId                : new node id
 *
 * @return IDE_SUCCESS or IDE_FAILURE
 */

IDE_RC mtfRegExp::expList( mtfRegExpression *aExp, SInt *aOutNodeId )
{
    SInt sRet = -1;
    SInt sNode;
    SInt sTempNode;
    SInt sTempRight;
    
    if ( *aExp->p == MTF_REGEXP_SYMBOL_BEGINNING_OF_STRING )    // '^'
    {
        aExp->p++;
        sRet = newNode( aExp, OP_BOL );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( element( aExp, &sNode ) != IDE_SUCCESS );
    
    if ( sRet != -1 )
    {
        aExp->nodes[sRet].next = sNode;
    }
    else
    {
        sRet = sNode;
    }
    
    if ( *aExp->p == MTF_REGEXP_SYMBOL_BRANCH )                 // '|'
    {
        aExp->p++;
        sTempNode = newNode( aExp, OP_OR );
        aExp->nodes[sTempNode].left = sRet;

        IDE_TEST( expList( aExp, &sTempRight ) != IDE_SUCCESS );
        aExp->nodes[sTempNode].right = sTempRight;
        sRet = sTempNode;
    }
    else
    {
        /* Nothing to do */
    }
    
    *aOutNodeId = sRet;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

// expression compile
//------------------------------------------------------------------------------
// search

/**
 * OP_CCLASS ex) \a \A \w \W ...  match check
 *
 * caller : matchClass(), matchNode()
 *
 * callee :
 *
 * @param SInt  aCClass     : OP_CCLASS node-> left value ex) a A w W s S ...
 * @param SChar aMatchChar  : target character
 *
 * @return ID_TRUE or ID_FALSE
 */
SInt mtfRegExp::matchCClass( SInt aCClass, SChar aMatchChar )
{
    SInt sRes = 0;
    
    switch( aCClass )
    {
        case 'a':
            sRes = isAlpha( aMatchChar );
            break;
        case 'A':
            sRes = !isAlpha( aMatchChar );
            break;
        case 'w':
            if ( isAlnum( aMatchChar ) || ( aMatchChar == '_' ) )
            {
                sRes = 1;
            }
            else
            {
                /* Nothing to do */
            }
            break;
        case 'W':
            if ( !isAlnum( aMatchChar ) && ( aMatchChar != '_' ) )
            {
                sRes = 1;
            }
            else
            {
                /* Nothing to do */
            }
            break;
        case 's':
            sRes = isSpace( aMatchChar );
            break;
        case 'S':
            sRes = !isSpace( aMatchChar );
            break;
        case 'd':
            sRes = isDigit( aMatchChar );
            break;
        case 'D':
            sRes = !isDigit( aMatchChar );
            break;
        case 'x':
            sRes = isXdigit( aMatchChar );
            break;
        case 'X':
            sRes = !isXdigit( aMatchChar );
            break;
        case 'c':
            sRes = isCntrl( aMatchChar );
            break;
        case 'C':
            sRes = !isCntrl( aMatchChar );
            break;
        case 'p':
            sRes = isPunct( aMatchChar );
            break;
        case 'P':
            sRes = !isPunct( aMatchChar );
            break;
        case 'l':
            sRes = isLower( aMatchChar );
            break;
        case 'u':
            sRes = isUpper( aMatchChar );
            break;
        default:
            sRes = 0;
            break;
    }
    return sRes;
}

/**
 * OP_RANGE, OP_CCLASS, etc character
 *
 * caller : matchNode()
 *
 * callee : matchCClass()
 *
 * @param mtfRegExpression *aExp          : the compiled expression
 * @param mtfRegExpNode    *aNode         : target node
 * @param SChar             aMatchChar    : target character
 *
 * @return ID_TRUE or ID_FALSE
 */

idBool mtfRegExp::matchClass( mtfRegExpression  *aExp,
                              mtfRegExpNode     *aNode,
                              SChar              aMatchChar )
{
    idBool sRes = ID_FALSE;
    do
    {
        switch( aNode->type )
        {
            case OP_RANGE:
                if ( ( aMatchChar >= aNode->left ) &&
                     ( aMatchChar <= aNode->right) )
                {
                    sRes = ID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
                break;
            case OP_CCLASS:
                if ( matchCClass( aNode->left, aMatchChar ) )
                {
                    sRes = ID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
                break;
            default:
                if ( aMatchChar == aNode->type )
                {
                    sRes = ID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
                break;
        }
    } while ( ( aNode->next != -1 ) && ( aNode = &aExp->nodes[aNode->next] ) );
    
    return sRes;
}

/**
 *
 *
 * caller : match(), matchNode(), searchRange()
 *
 * callee : matchNode(), matchClass(), matchCClass()
 *
 * @param mtfRegExpression *aExp          : the compiled expression
 * @param mtfRegExpNode    *aNode         : target node
 * @param SChar             aMatchChar    : target character
 *
 * @return match String
 */
const SChar * mtfRegExp::matchNode( mtfRegExpression *aExp,
                                    mtfRegSearch     *aSearch,
                                    mtfRegExpNode    *aNode,
                                    const SChar      *aStr,
                                    mtfRegExpNode    *aNextNode )
{
    const SChar       *sResStr;
    mtfRegExpNodeType  sNodeType = aNode->type;
    
    // for OP_GREEDY
    mtfRegExpNode      *sGreedyStopNode = NULL;
    SInt                sMin;
    SInt                sMax;
    SInt                sMatchesCount;
    const SChar        *sStr;
    const SChar        *sGoodStr;
    const SChar        *sStopStr;
    mtfRegExpNode      *sGreedyNextNode = NULL;
    
    // for OP_OR
    const SChar        *sOrStr;
    mtfRegExpNode      *sTempNode;
    
    // for OP_EXPR , OP_NOCAPEXPR
    mtfRegExpNode      *sExprNode;
    mtfRegExpNode      *sSubNextNode;
    const SChar        *sCurStr;
    SInt                sCapture;

    switch ( sNodeType )
    {
        case OP_GREEDY:
            sMin = ( aNode->right >> 16 ) & 0x0000FFFF;
            sMax = aNode->right & 0x0000FFFF;
            sMatchesCount = 0;
            sStr = aStr;
            sGoodStr = aStr;
            
            if ( aNode->next != -1 )
            {
                sGreedyStopNode = &aExp->nodes[aNode->next];
            }
            else
            {
                sGreedyStopNode = aNextNode;
            }
            
            while ( ( sMatchesCount == 0xFFFF ) || ( sMatchesCount < sMax ) )
            {
                sStr = matchNode( aExp, aSearch, &aExp->nodes[aNode->left], sStr, sGreedyStopNode );
                if ( sStr == NULL )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
                
                sMatchesCount++;
                sGoodStr = sStr;
                if ( sGreedyStopNode != NULL )
                {
                    //checks that 0 matches satisfy the expression(if so skips)
                    //if not would always stop(for instance if is a '?')
                    if ( ( sGreedyStopNode->type != OP_GREEDY ) ||
                         ( ( sGreedyStopNode->type == OP_GREEDY ) &&
                         ( ( sGreedyStopNode->right >> 16 ) & 0x0000FFFF ) != 0 ) )
                    {
                        sGreedyNextNode = NULL;
                        if ( sGreedyStopNode->next != -1 )
                        {
                            sGreedyNextNode = &aExp->nodes[sGreedyStopNode->next];
                        }
                        else if ( ( aNextNode != NULL ) && ( aNextNode->next != -1 ) )
                        {
                            sGreedyNextNode = &aExp->nodes[aNextNode->next];
                        }
                        else
                        {
                            /* Nothing to do */
                        }

                        sStopStr = matchNode( aExp, aSearch, sGreedyStopNode, sStr, sGreedyNextNode );
                        if ( sStopStr != NULL )
                        {
                            //if satisfied stop it
                            if ( ( ( sMin == sMax ) && ( sMin == sMatchesCount ) )     ||
                                 ( ( sMatchesCount >= sMin ) && ( sMax == 0xFFFF ) ) ||
                                 ( ( sMatchesCount >= sMin ) && ( sMatchesCount <= sMax ) ) )
                            {
                                break;
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

                if ( sStr >= aSearch->endOfLine )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            
            if ( ( ( sMin == sMax ) && ( sMin == sMatchesCount ) )     ||
                 ( ( sMatchesCount >= sMin ) && ( sMax == 0xFFFF ) )   ||
                 ( ( sMatchesCount >= sMin ) && ( sMatchesCount <= sMax ) ) )
            {
                sResStr = sGoodStr;
                IDE_CONT( NORMAL_EXIT );
            }
            else
            {
                sResStr = NULL;
                IDE_CONT( NORMAL_EXIT );
            }

        case OP_OR:
            sOrStr = aStr;
            sTempNode = &aExp->nodes[aNode->left];
            while ( ( sOrStr = matchNode( aExp, aSearch, sTempNode, sOrStr, NULL ) ) )
            {
                if ( sTempNode->next != -1 )
                {
                    sTempNode = &aExp->nodes[sTempNode->next];
                }
                else
                {
                    sResStr = sOrStr;
                    IDE_CONT( NORMAL_EXIT );
                }
            }
            sOrStr = aStr;
            sTempNode = &aExp->nodes[aNode->right];
            while ( ( sOrStr = matchNode( aExp, aSearch, sTempNode, sOrStr, NULL ) ) )
            {
                if ( sTempNode->next != -1 )
                {
                    sTempNode = &aExp->nodes[sTempNode->next];
                }
                else
                {
                    sResStr = sOrStr;
                    IDE_CONT( NORMAL_EXIT );
                }
            }
            sResStr = NULL;
            IDE_CONT( NORMAL_EXIT );
            
        case OP_EXPR:
        case OP_NOCAPEXPR:
            sExprNode = &aExp->nodes[aNode->left];
            sCurStr = aStr;
            sCapture = -1;
            if ( ( aNode->type != OP_NOCAPEXPR ) &&
                 ( aNode->right == aSearch->currsubexp ) )
            {
                sCapture = aSearch->currsubexp;
                aSearch->matches[sCapture].begin = sCurStr;
                aSearch->currsubexp++;
            }
            else
            {
                /* Nothing to do */
            }
            
            do
            {
                sSubNextNode = NULL;
                if ( sExprNode->next != -1 )
                {
                    sSubNextNode = &aExp->nodes[sExprNode->next];
                }
                else
                {
                    sSubNextNode = aNextNode;
                }
                
                sCurStr = matchNode( aExp, aSearch, sExprNode, sCurStr, sSubNextNode );
                if ( sCurStr == NULL )
                {
                    if ( sCapture != -1 )
                    {
                        aSearch->matches[sCapture].begin = 0;
                        aSearch->matches[sCapture].len = 0;
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    sResStr = NULL;
                    IDE_CONT( NORMAL_EXIT );
                }
                else
                {
                    /* Nothing to do */
                }
                
            } while ( ( sExprNode->next != -1 ) && ( sExprNode = &aExp->nodes[sExprNode->next] ) );
            
            if ( sCapture != -1 )
            {
                aSearch->matches[sCapture].len = (SInt)( sCurStr - aSearch->matches[sCapture].begin );
            }
            else
            {
                /* Nothing to do */
            }

            sResStr = sCurStr;
            IDE_CONT( NORMAL_EXIT );

        case OP_WB:
            if ( ( ( aStr == aSearch->beginOfLine ) && !isSpace( *aStr ) )     ||
                 ( ( aStr == aSearch->endOfLine ) && !isSpace( *( aStr-1 ) ) ) ||
                 ( !isSpace( *aStr ) &&   isSpace( *( aStr+1 ) ) )             ||
                 (  isSpace( *aStr ) &&  !isSpace( *( aStr+1 ) ) ) )
            {
                sResStr = ( aNode->left == 'b' ) ? aStr:NULL;
                IDE_CONT( NORMAL_EXIT );
            }
            else
            {
                sResStr = ( aNode->left == 'b' ) ? NULL:aStr;
                IDE_CONT( NORMAL_EXIT );
            }

        case OP_BOL:
            if ( aStr == aSearch->beginOfLine )
            {
                sResStr = aStr;
                IDE_CONT( NORMAL_EXIT );
            }
            else
            {
                sResStr = NULL;
                IDE_CONT( NORMAL_EXIT );
            }
        case OP_EOL:
            if ( aStr == aSearch->endOfLine )
            {
                sResStr = aStr;
                IDE_CONT( NORMAL_EXIT );
            }
            else
            {
                sResStr = NULL;
                IDE_CONT( NORMAL_EXIT );
            }
        case OP_DOT:
            if ( aStr < aSearch->endOfLine )
            {
                aStr++; //*str++
                sResStr = aStr;
                IDE_CONT( NORMAL_EXIT );
            }
            else
            {
                sResStr = NULL;
                IDE_CONT( NORMAL_EXIT );
            }
        case OP_NCLASS:
        case OP_CLASS:
            // OP_CLASS 와 OP_NCLASS [^ 의 결과는 반대임
            if ( aStr < aSearch->endOfLine )
            {
                if ( matchClass( aExp, &aExp->nodes[aNode->left], *aStr ) ?
                     ( sNodeType == OP_CLASS ? 1:0 ) :
                     ( sNodeType == OP_NCLASS ? 1:0 ) )
                {
                    aStr++; //*str++
                    sResStr = aStr;
                    IDE_CONT( NORMAL_EXIT );
                }
                else
                {
                    sResStr = NULL;
                    IDE_CONT( NORMAL_EXIT );
                }
            }
            else
            {
                sResStr = NULL;
                IDE_CONT( NORMAL_EXIT );
            }
        case OP_CCLASS:
            if ( aStr < aSearch->endOfLine )
            {
                if ( matchCClass( aNode->left, *aStr ) )
                {
                    aStr++; //*str++
                    sResStr = aStr;
                    IDE_CONT( NORMAL_EXIT );
                }
                else
                {
                    sResStr = NULL;
                    IDE_CONT( NORMAL_EXIT );
                }
            }
            else
            {
                sResStr = NULL;
                IDE_CONT( NORMAL_EXIT );
            }
        default: /* char */
            if ( aStr < aSearch->endOfLine )
            {
                if ( *aStr != aNode->type )
                {
                    sResStr = NULL;
                    IDE_CONT( NORMAL_EXIT );
                }
                else
                {
                    aStr++; //*str++
                    sResStr = aStr;
                    IDE_CONT( NORMAL_EXIT );
                }
            }
            else
            {
                sResStr = NULL;
                IDE_CONT( NORMAL_EXIT );
            }
    }

    sResStr = NULL;
    
    IDE_EXCEPTION_CONT( NORMAL_EXIT );
    
    return sResStr;
}

SInt mtfRegExp::isAlnum ( UChar aChar )
{
    SInt sRet;
    sRet = isDigit( aChar );
    
    if ( sRet == 0 )
    {
        if ( isAlpha( aChar ) == 1 )
        {
            sRet = 1;
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
    
    return sRet;
}

SInt mtfRegExp::isAlpha( UChar aChar )
{
    SInt sRet = 0;
    
    if ( ( aChar >= 'A' ) && ( aChar <= 'Z' ) )
    {
        sRet = 1;
    }
    else
    {
        /* Nothing to do */
    }
    
    if ( sRet == 0 )
    {
        if ( ( aChar >= 'a' ) && ( aChar <= 'z' ) )
        {
            sRet = 1;
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
    
    return sRet;
}

SInt mtfRegExp::isSpace( UChar aChar )
{
    SInt sRet = 0;
    
    if ( ( aChar == '\t' ) ||
         ( aChar == '\n' ) ||
         ( aChar == '\v' ) ||
         ( aChar == '\f' ) ||
         ( aChar == '\r' ) ||
         ( aChar == ' ' ) )
    {
        sRet = 1;
    }
    else
    {
        /* Nothing to do */
    }
    
    return sRet;
}

SInt mtfRegExp::isCntrl( UChar aChar )
{
    SInt sRet = 0;
    
    if ( aChar <= 0x1F )
    {
        sRet = 1;
    }
    else
    {
        /* Nothing to do */
    }
    
    if ( sRet == 0 )
    {
        if ( aChar == 0x7F )
        {
            sRet = 1;
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
    
    return sRet;
}

SInt mtfRegExp::isDigit( UChar aChar )
{
    SInt sRet = 0;
    
    if ( ( aChar >= '0' ) && ( aChar <= '9' ) )
    {
        sRet = 1;
    }
    else
    {
        /* Nothing to do */
    }
    
    return sRet;
}

SInt mtfRegExp::isLower( UChar aChar )
{
    SInt sRet = 0;
    
    if ( ( aChar >= 'a' ) && ( aChar <= 'z' ) )
    {
        sRet = 1;
    }
    else
    {
        /* Nothing to do */
    }
    
    return sRet;
}

SInt mtfRegExp::isUpper( UChar aChar )
{
    SInt sRet = 0;
    
    if ( ( aChar >= 'A' ) && ( aChar <= 'Z' ) )
    {
        sRet = 1;
    }
    else
    {
        /* Nothing to do */
    }
    
    return sRet;
}


SInt mtfRegExp::isPrint( UChar aChar )
{
    SInt sRet = 0;
    
    if ( ( aChar >= 0x20 ) && ( aChar <= 0x7E ) )
    {
        sRet = 1;
    }
    else
    {
        /* Nothing to do */
    }
    
    return sRet;
}

SInt mtfRegExp::isPunct( UChar aChar )
{
    SInt sRet = 0;
    
    if ( isPrint( aChar ) &&
         !isLower( aChar ) &&
         !isUpper( aChar ) &&
         !isSpace( aChar ) &&
         !isDigit( aChar ) )
    {
        sRet = 1;
    }
    else
    {
        /* Nothing to do */
    }
    
    return sRet;
}

SInt mtfRegExp::isXdigit( UChar aChar )
{
    SInt sRet = 0;
    
    sRet = isDigit(aChar);
    
    if ( sRet == 0 )
    {
        if ( ( ( aChar >= 'A' ) && ( aChar <= 'F' ) ) ||
             ( ( aChar >= 'a' ) && ( aChar <= 'f' ) ) )
        {
            sRet = 1;
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
    
    return sRet;
}

// private function
//------------------------------------------------------------------------------
// API

/**
 * compiles an expression and returns a pointer to the compiled version with *aExp.
 * in case of failure returns NULL.
 *
 * caller : mtfCompileExpression()
 *
 * callee : newNode(), expList()
 *
 * @param mtfRegExpression *aExp        : in parameter compiled expression pointer
 * @param const SChar      *aPatternStr : a pointer to  string containing the pattern that
 has to be compiled.
 * @param SInt              aPatternLen : length of aPtternStr
 * mtcCallBack             *aCallBack   : mtcCallBack
 * @return IDE_SUCCESS or IDE_FAILURE
 */
IDE_RC mtfRegExp::expCompile( mtfRegExpression *aExp,
                              const SChar      *aPatternStr,
                              SInt              aPatternLen )
{
    SInt  sRes;
    UInt  sSize = ( aPatternLen * 4);

    /* BUG-43859 [mt-function] regexp_substr 함수 수행 중 서버 사망할 수 있습니다. */
    if ( sSize >= MTF_REGEXP_MAX_PATTERN_LENGTH )
    {
        sSize = MTF_REGEXP_MAX_PATTERN_LENGTH;
    }
    else
    {
        /* Nothing to do */
    }

    idlOS::memset( (void *)aExp->patternBuf, 0, sSize );
    idlOS::memcpy( (void *)aExp->patternBuf, aPatternStr, aPatternLen );

    aExp->patternLen = aPatternLen;
    aExp->p = aExp->patternBuf;
    
    aExp->nsize = 0;
    aExp->nsubexpr = 0;
    aExp->first = newNode( aExp, OP_EXPR );
    
    IDE_TEST( expList( aExp, &sRes ) != IDE_SUCCESS );
    aExp->nodes[aExp->first].left = sRes;
    
    IDE_TEST_RAISE( *(aExp->p) != '\0', ERR_REGEXP_UNEXPECTED_CAHR );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_REGEXP_UNEXPECTED_CAHR )
    IDE_SET( ideSetErrorCode( mtERR_ABORT_REGEXP_UNEXPECTED_CAHR ) );
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * returns ID_TRUE if the string specified in the parameter aText is an
 * exact match of the expression, otherwise returns ID_FALSE.
 *
 * caller :
 * callee : matchNode()
 *
 * @param mtfRegExpression  *aExp       : the compiled expression
 * @param const SChar       *aText      : the string that has to be tested
 * @return ID_TRUE or ID_FALSE
 */
idBool mtfRegExp::match( mtfRegExpression *aExp,
                         mtfRegSearch     *aSearch,
                         const SChar      *aText,
                         SInt              aTextLength )
{
    idBool       sRes;
    const SChar *sResStr = NULL;
    
    aSearch->beginOfLine = aText;
    aSearch->endOfLine = aText + aTextLength;
    aSearch->currsubexp = 0;
    
    sResStr = matchNode( aExp, aSearch, aExp->nodes, aText, NULL);
    
    if ( ( sResStr == NULL ) || ( sResStr != aSearch->endOfLine ) )
    {
        sRes = ID_FALSE;
    }
    else
    {
        sRes = ID_TRUE;
    }
    
    return sRes;
}

/**
 * searches the first match of the expression in the string delimited
 * by the parameter aTextBegin and aTextEnd.
 * if the match is found returns ID_TRUE and the sets aOutBegin to the beginning of the
 * match and aOutEnd at the end of the match; otherwise returns ID_FALSE.
 *
 * caller : search()
 * callee : mathcNode()
 *
 * @param mtfRegExpression  *aExp       : the compiled expression
 * @param const SChar       *aTextBegin : a pointer to the beginnning of the string that has to be tested
 * @param const SChar       *aTextEnd   : a pointer to the end of the string that has to be tested
 * @param const SChar      **aOutBegin  : a pointer to a string pointer that will be set with the beginning of the match
 * @param const SChar      **aOutEnd    : a pointer to a string pointer that will be set with the end of the match
 * @return ID_TRUE or ID_FALSE
 */

idBool mtfRegExp::searchRange( mtfRegExpression  *aExp,
                               mtfRegSearch      *aSearch,
                               const SChar       *aTextBegin,
                               const SChar       *aTextEnd,
                               const SChar      **aOutBegin,
                               const SChar      **aOutEnd )
{
    idBool       sRes;
    const SChar *sCurStr  = NULL;
    SInt         sNode    = aExp->first;
   
    // BUG-45386
    // 'ABC', 'B*' 이런 경우 endOfLine에서도 매치되어야 한다.
    // 따라서 TextBegin = TextEnd인 경우에도 search를 한다.
    if ( aTextBegin > aTextEnd )
    {
        sRes = ID_FALSE;
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }
    
    aSearch->beginOfLine = aTextBegin;
    aSearch->endOfLine   = aTextEnd;
    do
    {
        sCurStr = aTextBegin;
        while ( sNode != -1 )
        {
            aSearch->currsubexp = 0;
            sCurStr = matchNode( aExp, aSearch, &aExp->nodes[sNode], sCurStr, NULL );
            if ( sCurStr == NULL )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
            sNode = aExp->nodes[sNode].next;
        }
        aTextBegin++; //*text_begin++;
    } while ( ( sCurStr == NULL ) && ( aTextBegin <= aTextEnd ) );
    
    if ( sCurStr == NULL )
    {
        sRes = ID_FALSE;
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }
    
    --aTextBegin;
    
    if ( aOutBegin != NULL )
    {
        *aOutBegin = aTextBegin;
    }
    else
    {
        /* Nothing to do */
    }
    
    if ( aOutEnd != NULL )
    {
        *aOutEnd = sCurStr;
    }
    else
    {
        /* Nothing to do */
    }
    
    sRes = ID_TRUE;
    
    IDE_EXCEPTION_CONT( NORMAL_EXIT );
    
    return sRes;
}

/**
 * searches the first match of the expressin in the string specified in the parameter aText.
 * if the match is found returns ID_TRUE and the sets aOutBegin to the beginning of the
 * match and aOutEnd at the end of the match; otherwise returns ID_FALSE.
 *
 * caller : mtfRegExpLikeCalculate(), mtfRegExpLikeCalculateFast()
 * callee : searchRange()
 *
 * @param mtfRegExpression  *aExp       : the compiled expression
 * @param const SChar       *aText      : the string that has to be tested
 * @param const SChar      **aOutBegin  : a pointer to a string pointer that will be set with the beginning of the match
 * @param const SChar      **aOutEnd    : a pointer to a string pointer that will be set with the end of the match
 * @return ID_TRUE or ID_FALSE
*/
idBool mtfRegExp::search( mtfRegExpression *aExp,
                          const SChar      *aText,
                          SInt              aTextLength,
                          const SChar     **aOutBegin,
                          const SChar     **aOutEnd )
{
    mtfRegSearch sSearch;

    return searchRange( aExp,
                        &sSearch,
                        aText,
                        aText + aTextLength,
                        aOutBegin,
                        aOutEnd );
}

/**
 * returns the number of sub expressions matched by the expression
 *
 * @param mtfRegExpression  *aExp       : the compiled expression
 * @return SInt
 */
SInt mtfRegExp::getSubExpCount( mtfRegExpression *aExp )
{
    return aExp->nsubexpr;
}

/**
 * retrieve the begin and end pointer to the length of the sub expression indexed
 * by aExpIndex. The result is passed trhough the struct mtfRegExpMatch:
 *
 *this function works also after a match operation has been performend.
 *
 * caller :
 *
 * callee :
 *
 * @param mtfRegExpression *aExp           : the compiled expression
 * @param SInt              aSubMathcIndex : the index of the submatch
 * @param mtfRegExpMatch   *aSubMatch      : a pointer to structure that will store result
 *
 * @return ID_TRUE or ID_FALSE
 */
idBool mtfRegExp::getSubExp( mtfRegExpression *aExp,
                             mtfRegSearch     *aSearch,
                             SInt              aSubMathcIndex,
                             mtfRegExpMatch   *aSubMatch )
{
    idBool sRes;
    
    if ( ( aSubMathcIndex < 0 ) || ( aSubMathcIndex >= aExp->nsubexpr ) )
    {
        sRes = ID_FALSE;
    }
    else
    {
        *aSubMatch = aSearch->matches[aSubMathcIndex];
    
        sRes =  ID_TRUE;
    }
    
    return sRes;
}

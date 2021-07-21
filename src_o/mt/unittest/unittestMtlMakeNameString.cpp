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
 
#include <actTest.h>
#include <mtl.h>

#define BUFFER_SIZE ( 10 )

/* unittest
 * mtl::makeQuotedName
 * mtl::makeNameInFunc
 * mtl::makeNameInSQL
 */
acp_sint32_t main( void )
{
    acp_char_t sOutputString[BUFFER_SIZE];
    acp_char_t sSimpleString[BUFFER_SIZE];
    acp_char_t sQuotedString[BUFFER_SIZE];
    acp_char_t sUppercaseString[BUFFER_SIZE];

    ACT_TEST_BEGIN();

    acpMemSet( sSimpleString, 0, BUFFER_SIZE );
    acpMemSet( sQuotedString, 0, BUFFER_SIZE );
    acpMemSet( sUppercaseString, 0, BUFFER_SIZE );

    acpMemCpy( sSimpleString, "String", sizeof( "String" ) );
    acpMemCpy( sQuotedString, "\"String\"", sizeof( "\"String\"" ) );
    acpMemCpy( sUppercaseString, "STRING", sizeof( "STRING" ) );

    /*
     * makeQuotedName function :
     * output : quoted string
     */
    acpMemSet( sOutputString, 0, BUFFER_SIZE );
    mtl::makeQuotedName( sOutputString,
                         sSimpleString,
                         strlen( sSimpleString ) );

    ACT_CHECK( 0 == acpMemCmp( sOutputString, sQuotedString, BUFFER_SIZE ) );

    /* makeNameInFunc :
     * input : non quoted string => output : uppercase string
     * input : quoted string => output : quoted string
     */
    acpMemSet( sOutputString, 0, BUFFER_SIZE );
    mtl::makeNameInFunc( sOutputString,
                         sSimpleString,
                         strlen( sSimpleString ) );

    ACT_CHECK( 0 == acpMemCmp( sOutputString, sUppercaseString, BUFFER_SIZE ) );

    acpMemSet( sOutputString, 0, BUFFER_SIZE );
    mtl::makeNameInFunc( sOutputString,
                         sQuotedString,
                         strlen( sQuotedString ) );

    ACT_CHECK( 0 == acpMemCmp( sOutputString, sQuotedString, BUFFER_SIZE ) );

    /* makeNameInSQL :
     * input : non quoted string => output : uppercase string
     * input : quoted string => output : non quoted string
     */
    acpMemSet( sOutputString, 0, BUFFER_SIZE );
    mtl::makeNameInSQL( sOutputString,
                        sSimpleString,
                        strlen( sSimpleString ) );

    ACT_CHECK( 0 == acpMemCmp( sOutputString, sUppercaseString, BUFFER_SIZE ) );

    acpMemSet( sOutputString, 0, BUFFER_SIZE );
    mtl::makeNameInSQL( sOutputString,
                        sQuotedString,
                        strlen( sQuotedString ) );

    ACT_CHECK( 0 == acpMemCmp( sOutputString, sSimpleString, BUFFER_SIZE ) );

    ACT_TEST_END();

    return 0;
}

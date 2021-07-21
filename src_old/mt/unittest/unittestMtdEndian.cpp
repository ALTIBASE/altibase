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
#include <mtd.h>

extern mtdModule mtdChar;
extern mtdModule mtdInteger;

#define SAMPLE_SIZE ( 4 )

void testMtdChar( void )
{
    mtdCharType sMtdChar;

    /* set mtdCharType's length
     *
     *  typedef struct mtdCharType {
     *       UShort length;
     *       UChar  value[1];
     *  } mtdCharType;
     */
    sMtdChar.length = SAMPLE_SIZE;

    ACT_CHECK( sMtdChar.length == SAMPLE_SIZE );

    mtdChar.endian( ( void * ) &sMtdChar );

    /* endian conversion result :
          00000000 00000100 (4)
       => 00000100 00000000 (1024)
    */
    ACT_CHECK( sMtdChar.length == 1024 );
}

void testMtdInteger( void )
{
    mtdIntegerType   sMtdInteger;

    /* set mtdIntegerType value
     *
     * typedef SInt mtdIntegerType;
     */
    sMtdInteger = ( mtdIntegerType ) SAMPLE_SIZE;

    ACT_CHECK( sMtdInteger == SAMPLE_SIZE );

    mtdInteger.endian( ( void * ) &sMtdInteger );

    /* endian conversion result :
          00000000 00000000 00000000 00000100 (4)
       => 00000100 00000000 00000000 00000000 (67108864)
    */
    ACT_CHECK( sMtdInteger == 67108864 );
}

acp_sint32_t main( void )
{
    ACT_TEST_BEGIN();

    testMtdChar();
    testMtdInteger();

    ACT_TEST_END();

    return 0;
}

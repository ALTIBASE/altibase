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
 
#include <stdio.h>
#include <stdlib.h>

int main( int argc, char **argv )
{
    char list[128];
    FILE *in = NULL;
    FILE *out = NULL;
    char out_log[128];
    char in_log[128];
    int runner = 0;
    int line = 0; 
    int a = 0;
    int b = 0;

    if( argc != 4 )
    {
        printf( "usage: %s %s %s\n", argv[0], "line", "runner" );
        return -1; 
    }

    line = atoi(argv[1]);
    runner = atoi(argv[2]);

    sprintf( in_log, "%s.dat", argv[3] ); 
    in = fopen( in_log, "r" );
    if( in == NULL )
    {
        printf( "fopen error: %s\n", in_log );
        return -1;
    }

    for( int a = 1; a <= runner; a++ )
    {
        sprintf( out_log, "%s_%d.dat", argv[3], a );
        printf( "%s\n", out_log );
        out = fopen( out_log, "w" );
        if( out == NULL )
        {
            printf( "fopen error: %s\n", out_log );
            return -1;
        }
        b = 0;

        while( fgets(list, sizeof(list), in) != NULL )
        {
            fprintf( out, "%s", list );
            b++; 
            if( (b == line) && (a != runner) )
            {
                fclose( out);
                out = NULL;
                break;
            }
        }
    }
    if( out != NULL )
    {
        fclose( out);
        out = NULL;
    }

    fclose( in );
    in = NULL;

    return 0;
}

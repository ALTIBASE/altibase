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
 
/*
#include <stdio.h>
*/

// simple
#define AAAA BBBB

#  define A (B \ 
abcd \
abcd)


// simple argument    

#define TEST(a,b)  target_speed(a, b)    

// nested argument

#define MYSYM1    1234
#define MYSYM2    5678
#define FINAL(a,b)  TARGET_FINAL(a,b)
    
#ifdef NOTDEF    
/*
#ifdef ABCD
int a = 10;
#else
int b = 20;
#endif

*/
#endif


#ifdef NONE \
    BB
int b;
#endif

// typedef int Int; ok


typedef Int Bbb;

typedef	__FILE FILE;

typedef enum {
    a = 10,
    b = 20
} AABB;

int main(int argc, char **argv) // 6. c++ comment 
{
    int i, a;
    
    for (i = 0; i < 10; i++)
    {
        printf(/* sting..
                  asdjfjasdljf
                */
               "accs testing../* ddt */ \n"
/* asdjfjasdf
asdfkasd;fkslad;fj                                 
                */ );
    }
}

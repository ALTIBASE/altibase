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
#define FINAL(a,b)  target_func(a,b)
    

int main(int argc, char **argv) // 6. c++ comment 
{
    int i, a;

    printf(AAAA);
    FINAL(MYSYM1, MYSYM2);
    
}

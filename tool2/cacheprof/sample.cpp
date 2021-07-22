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
#include <csProto.h>

char array[128];

int main()
{
    cs_cache_clear();
    cs_cache_start();
    fprintf(stdout, "array=%x\n", array);

    array[0] = 1;  // miss
    array[1] = 1;
    array[2] = 1;
    array[3] = 1;
    array[4] = 1;
    array[5] = 1;
    array[6] = 1;
    array[7] = 1;

    array[8] = 1;  // miss
    array[9] = 1;
    array[10] = 1;
    array[11] = 1;
    array[12] = 1;
    array[13] = 1;
    array[14] = 1;
    array[15] = 1;

    array[16] = 1;

    cs_cache_stop();
    cs_cache_save("myData");
}

/*
$ ./sample.prof     :: 1024k 32 2 
   array=40000378
   Dump of Cache Miss Statistics (Total = 6) 
   SumOf Miss ||  FileName:Procedure:Line:ReadMiss:WriteMiss 
         4 ||  sample.cpp:main:10:4:0
         1 ||  sample.cpp:main:12:1:0
         1 ||  sample.cpp:main:21:1:0

 */


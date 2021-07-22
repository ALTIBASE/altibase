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
 

/*****************************************************************************
 * $Id: testStat.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 ****************************************************************************/

#include <csStat.h>


#define HASH_SIZE   10

csStat gStat;

int main()
{
    long i;
    gStat.initialize(HASH_SIZE);
    for (i = 0; i < 100; i++)
    {
        gStat.update(0, "test1.cpp", "main", rand());
        gStat.update(0, "test1.cpp", "main", rand());
        gStat.update(0, "test4.cpp", "func1", rand());
        gStat.update(0, "test1.cpp", "main", rand());
        gStat.update(1, "test2.cpp", "main", rand());
        gStat.update(1, "test1.cpp", "main", rand());
        gStat.update(1, "test1.cpp", "func2", rand());
        gStat.update(1, "test3.cpp", "main", rand());
        gStat.update(1, "test1.cpp", "main", rand());
        gStat.update(1, "test5.cpp", "main", rand());
    }

    gStat.Dump(stderr);
}
 

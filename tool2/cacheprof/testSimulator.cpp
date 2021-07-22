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
 * $Id: testSimulator.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 ****************************************************************************/

#include <csSimul.h>

#define CAPACITY   128
#define LINE_SIZE  (4)
#define WAYS       2

#define TEST_START   128
#define TEST_RANGE  (512)

csSimul gSimul;

int main()
{
    Address i;
    gSimul.initialize(CAPACITY, LINE_SIZE, WAYS);

    for (i = TEST_START; i < (TEST_START + TEST_RANGE); i++)
    {
        gSimul.DumpTest(i);
        gSimul.DumpTest(i);
    }
}

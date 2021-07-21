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
 
#ifndef _O_CHECKIPC_H_
#define _O_CHECKIPC_H_ 1

#include <idl.h>

timeval getCurTime()
{
    timeval t1;
    gettimeofday(&t1, NULL);

    return t1;
}

long getTimeGap(timeval t1, timeval t2)
{
    long usec = (t2.tv_sec - t1.tv_sec);

    long usec2 = (t2.tv_usec - t1.tv_usec);
#ifdef NOTDEF
    printf("t1 [%ld:%ld]  t2 [%ld:%ld]\n",
           t1.tv_sec, t1.tv_usec,
           t2.tv_sec, t2.tv_usec);
#endif    
    return (usec * 1000000) + usec2;
}










#endif

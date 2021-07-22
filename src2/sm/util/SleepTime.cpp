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
 

/***********************************************************************
 * $Id: SleepTime.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 ***********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <idtBaseThread.h>
#include <iduSema.h>


static PDL_Time_Value mySleep;

timeval getCurTime()
{
    timeval t1;
    gettimeofday(&t1, NULL);

    return t1;
}

ULong getTimeGap(timeval& t1, timeval& t2)
{
    SLong usec = (t2.tv_sec - t1.tv_sec);

    SLong usec2 = (t2.tv_usec - t1.tv_usec);
#ifdef NOTDEF
    idlOS::printf("t1 [%ld:%ld]  t2 [%ld:%ld]\n",
                  t1.tv_sec, t1.tv_usec,
                  t2.tv_sec, t2.tv_usec);
#endif    
    return (ULong)((usec * 1000000) + usec2);
}

#define TEST_COUNT   (1000)

int main(int argc, char **argv)
{
    ULong i;
    timeval t1, t2;
    pthread_mutex_t lock_;
    pthread_cond_t count_nonzero_;
    PDL_Time_Value aTime;

    mySleep.initialize(0, 1);

    assert(pthread_mutex_init(&lock_, NULL) == 0);
    assert(pthread_cond_init(&count_nonzero_, NULL) == 0);

    t1= getCurTime();
    
    assert(pthread_mutex_lock(&lock_) == 0);
    for (i = 0; i < TEST_COUNT; i++)
    {
        timeval v;
        timespec k;
        gettimeofday(&v, NULL);
        k.tv_sec = v.tv_sec;
        k.tv_nsec = (v.tv_usec * 1000) + 1000;

        assert(pthread_cond_timedwait (&count_nonzero_, 
                                      &lock_, &k) != 0);
    }
    t2= getCurTime();
    assert(pthread_mutex_unlock(&lock_) == 0);

    idlOS::printf("cout=%d time = %lld \n", TEST_COUNT, getTimeGap(t1, t2));
    
    idlOS::printf("average time = %f \n",
                  (SDouble)getTimeGap(t1, t2) / TEST_COUNT);
}

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
 * $Id: checkMultiCrypt.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 ***********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <idtBaseThread.h>
#include <iduSema.h>

//#define TEST_THR_SAFE (1)

#define THREAD_NUM (10)

#define TEST_COUNT  (100000)

extern "C" void* thread_func(void*);


int main()
{
    pthread_mutex_t mutex;
    pthread_t       thread_id[THREAD_NUM];
    char            connname[THREAD_NUM][10];
    int             i = 0;
    pthread_attr_t  sThrAttr;
    SInt            sFlag;

    idlOS::srand((UInt)idlOS::time(NULL));
    /*  ATTR Init */
    if (pthread_attr_init (&sThrAttr) != 0)
    {
        return -1;
    }
    
    /*  SCOPE control */
    sFlag = PTHREAD_SCOPE_SYSTEM;
    
    if (pthread_attr_setscope(&sThrAttr, sFlag) != 0)
    {
        return -1;
    }
    
    /* JOIN control */
    sFlag = PTHREAD_CREATE_JOINABLE; //PTHREAD_CREATE_DETACHED;// : 
    if (pthread_attr_setdetachstate(&sThrAttr, sFlag) != 0)
    {
        return -1;
    }

    for (i = 0; i < THREAD_NUM; i++)
    {
        snprintf(connname[i], 10, "CONN%d", i+1);
        if(pthread_create(&thread_id[i], &sThrAttr, thread_func, (void*)connname[i]))
        {
            idlOS::printf("Can't create thread %d\n", i+1);
        }
    }
    
    for (i = 0; i < THREAD_NUM; i++)
    {
        if(pthread_join(thread_id[i], NULL))
        {
            idlOS::printf("Error waiting for thread %d to terminate\n", i+1);
        }
    }
}

static UChar *gPass = (UChar *)"gamestar";

void* thread_func(void* p_param)
{
    UInt  i;
    UChar sCrypted[128];
    UChar sSalt[3];
    UChar *sRes;
    IDL_CRYPT_DATA sData;

    fprintf(stderr, "start!! %d\n", (UInt)idlOS::getThreadID());
    
    idlOS::memset(&sData, 0, ID_SIZEOF(sData));
    idlOS::memset(&sCrypted, 0, ID_SIZEOF(sCrypted));
    idlOS::memset(&sSalt, 0, ID_SIZEOF(sSalt));

    sSalt[0] = (idlOS::rand() % 64) + 33;
    sSalt[1] = (idlOS::rand() % 64) + 33;
#if defined(TEST_THR_SAFE)
    sRes = idlOS::crypt(gPass, sSalt, &sData);
#else
    sRes = (UChar *)crypt((char *)gPass, (char *)sSalt);
#endif
    idlOS::strcpy((SChar *)sCrypted, (SChar *)sRes);

    for (i = 0; i < TEST_COUNT; i++)
    {
#if defined(TEST_THR_SAFE)
        sRes = idlOS::crypt(gPass, sSalt, &sData);
#else
        sRes = (UChar *)crypt((char *)gPass, (char *)sSalt);
#endif

        if (idlOS::strcmp((SChar *)sRes, (SChar *)sCrypted) != 0)
        {
            fprintf(stderr, "FATAL ERROR!!!! org=[%s] now=[%s] (%d) \n",
                    sCrypted, sRes, (UInt)idlOS::getThreadID());
            idlOS::exit(0);
        }
    }
    fprintf(stderr, "SUCCESS!!!! org=[%s] now=[%s]  (%d)\n", sCrypted, sRes, (UInt)idlOS::getThreadID());
    return NULL;
}

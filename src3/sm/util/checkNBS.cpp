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
 * $Id: checkNBS.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 ***********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <idtBaseThread.h>
#include <iduSema.h>
#include <iduStack.h>

#define SERIAL_BEGIN(serial_code) {\
volatile UInt vstSeed = 0;\
 if ( vstSeed++ == 0)\
 {\
     serial_code;\
 }
 

#define SERIAL_END(serial_code)\
  if (vstSeed++ == 1)\
  {\
      serial_code;\
  }\
}
 


ideMsgLog mMsgLogForce;


class thrNBS : public idtBaseThread
{
    ULong mCount;
    SInt mType;
public:
    thrNBS() : idtBaseThread(IDT_JOINABLE) {};

    IDE_RC initialize(SInt aType, ULong aCount = ID_ULONG(10000000000));
    IDE_RC destroy();

    virtual void run();
};

#define MAX_SEED (4)

static UInt  Pv1, Pv2;
static ULong gValue;


static ULong Pdata[MAX_SEED] =
{
    ID_ULONG(0x1234567812345678),
    ID_ULONG(0xaaaabbbbccccdddd),
    ID_ULONG(0xffffeeeeddddcccc),
    ID_ULONG(0x1111222233334444)
};

inline void reset(UInt *sSeed1)
{
    *sSeed1 = 0;
}



IDE_RC thrNBS::initialize(SInt aType, ULong aCount)
{
    mType  = aType;
    mCount = aCount;
    
    return IDE_SUCCESS;
}


#define USE_NBS 
void thrNBS::run()
{
    UInt sSeed = 0;
    UInt sPv1, sPv2;
    ULong sValue1;
    ULong sValue2;
    IDE_TEST_RAISE(ideAllocErrorSpace() != IDE_SUCCESS,
                   ide_alloc_error);

    gValue = sValue1 = sValue2 = Pdata[0];
    
    while(mCount-- > 0)
    {
        if (mType == 0) // read
        {
            UInt   i, j;
            idBool sExact = ID_FALSE;
            do
            {
                SERIAL_BEGIN(sPv2   = Pv2; sValue1 = gValue;);
                idlOS::thr_yield();
                SERIAL_END(sValue2 = gValue;sPv1   = Pv1;);
                
            } while (sPv1 != sPv2);
            for (i = 0; i < MAX_SEED; i++)
            {
                if ((sValue1 == Pdata[i]) && (sValue2 == Pdata[i]))
                {
                    sExact = ID_TRUE;
                    break;
                }
            }

            if (sExact != ID_TRUE)
            {
                idlOS::fprintf(stderr, "\n Error!! %ld %ld %lx %lx\n", sPv1, sPv2, sValue1, sValue2);
                idlOS::exit(1);
            }
            idlOS::thr_yield();
        }
        else // write
        {
            SERIAL_BEGIN (Pv1++);
//            idlOS::thr_yield();
            gValue = Pdata[mCount % MAX_SEED];
            SERIAL_END (Pv2++);
        }
    }
    return;
    IDE_EXCEPTION(ide_alloc_error);
    {
        IDE_CALLBACK_FATAL("ide_alloc_error");
    }
    IDE_EXCEPTION_END;
}

#define MAX_READ_THREAD (10)
#define MAX_WRITE_THREAD (1)

thrNBS gReadThr[MAX_READ_THREAD];
thrNBS gWriteThr[MAX_WRITE_THREAD];

int main()
{
    UInt i;

    fprintf(stderr, "start \n");
    
    for (i = 0; i < MAX_WRITE_THREAD; i++)
    {
        IDE_ASSERT(gWriteThr[i].initialize(1) == IDE_SUCCESS);
        IDE_ASSERT(gWriteThr[i].start() == IDE_SUCCESS);
        gWriteThr[i].waitToStart();
    }
    idlOS::sleep(1);
    
    for (i = 0; i < MAX_READ_THREAD; i++)
    {
        IDE_ASSERT(gReadThr[i].initialize(0) == IDE_SUCCESS);
        IDE_ASSERT(gReadThr[i].start() == IDE_SUCCESS);
    }
    
    IDE_ASSERT(gWriteThr[0].join() == IDE_SUCCESS);
}

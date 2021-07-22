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
 

#include <idl.h>
#include <ideErrorMgr.h>
#include <smr.h>
#include <smi.h>

static UInt gCopyMethod;

#define COPY_THREAD_COUNT (1)
#define COPY_FILE_COUNT   (50)

class smuCopyThread : public idtBaseThread
{
//For Operation
public:
    smuCopyThread();
    virtual ~smuCopyThread();

    double  CheckTime(struct timeval *sTime, struct timeval *eTime, int val);
    double  ShowElapsed(int count, struct timeval *start_time, struct timeval *end_time);
        
    IDE_RC initialize(UInt aStart);
    IDE_RC destroy();

    IDE_RC startThread();

    IDE_RC shutdown();
    virtual void run();

//For Member
public:

    UInt    mStart;
    idBool  mFinish;
};


smuCopyThread::smuCopyThread()
        : idtBaseThread(IDT_JOINABLE)
{
}

smuCopyThread::~smuCopyThread()
{
}

IDE_RC smuCopyThread::initialize(UInt aStart)
{
    mStart = aStart;
    mFinish = ID_FALSE;
    
    resetStarted(); 

    return IDE_SUCCESS;
}

IDE_RC smuCopyThread::startThread()
{
    IDE_TEST(start() != IDE_SUCCESS);
    IDE_TEST(waitToStart(0) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smuCopyThread::destroy()
{
    return IDE_SUCCESS;
}

void smuCopyThread::run()
{
    UInt         i;
    UInt         sStart;
    UInt         sEnd;
    iduFile      sSrcFile;
    iduFile      sDestFile;
    struct  stat sStat;
    SChar*       sSrcBase;
    SChar*       sDestBase;
    SChar        sSrcLogFileName[512];
    SChar        sDestLogFileName[512];
    SChar        sSeed = 0;
    double       totalTime = 0.0;
    struct timeval startTime;
    struct timeval endTime;

    IDE_TEST(ideAllocErrorSpace() != IDE_SUCCESS);
    
    sStart = mStart;
    sEnd = sStart + COPY_FILE_COUNT;

    idlOS::printf("copy %d ~ %d\n", sStart, sEnd);
    
    gettimeofday(&startTime, (void *)NULL);

    for (i = sStart ; i < sEnd; i++)
    {
        idlOS::snprintf(sSrcLogFileName, 512, "%s%c%s%"ID_UINT32_FMT"", 
                        smuProperty::getLogDirPath(), 
                        IDL_FILE_SEPARATOR,
                        SMR_LOG_FILE_NAME, 
                        i);
        
        IDE_TEST(sSrcFile.initialize(IDU_MEM_SM_SMU) != IDE_SUCCESS);
        IDE_TEST(sSrcFile.setFileName(sSrcLogFileName) != IDE_SUCCESS);
        IDE_TEST(sSrcFile.open() != IDE_SUCCESS);
        
        IDE_TEST(idlOS::fstat(sSrcFile.getFileHandle(), &sStat) != 0);
        
        sSrcBase = (SChar*)idlOS::mmap(0, sStat.st_size, PROT_READ, 
                                       MAP_SHARED, sSrcFile.getFileHandle(), 0);
        
        idlOS::snprintf(sDestLogFileName, 512, "%s%c%s%"ID_UINT32_FMT"", 
                        smuProperty::getArchiveDir(), 
                        IDL_FILE_SEPARATOR,
                        SMR_LOG_FILE_NAME, 
                        i);
        
        IDE_TEST(sDestFile.initialize(IDU_MEM_SM_SMU) != IDE_SUCCESS);
        IDE_TEST(sDestFile.setFileName(sDestLogFileName) != IDE_SUCCESS);
        
        IDE_TEST(sDestFile.create() != IDE_SUCCESS);
        
        IDE_TEST(sDestFile.open() != IDE_SUCCESS);

        // for performance
        IDE_TEST(sDestFile.write(sStat.st_size - 1, &sSeed, ID_SIZEOF(SChar))
                 != IDE_SUCCESS);

        if (gCopyMethod == 0)
        {
            IDE_TEST(sDestFile.write(0, sSrcBase, sStat.st_size)
                     != IDE_SUCCESS);

            IDE_TEST(sDestFile.sync() != IDE_SUCCESS);
        }
        else
        {
            sDestBase = (SChar*)idlOS::mmap(0, sStat.st_size, PROT_READ | PROT_WRITE, 
                                            MAP_PRIVATE, sDestFile.getFileHandle(), 0);
        
            idlOS::memcpy(sDestBase, sSrcBase, sStat.st_size);

            idlOS::msync((SChar*)sDestBase, sStat.st_size, MS_SYNC);
            
            (void)idlOS::madvise((caddr_t)sDestBase, (size_t)sStat.st_size, MADV_DONTNEED);
           
            IDE_TEST(idlOS::munmap(sDestBase, sStat.st_size) != 0);
         }
        
        IDE_TEST(sDestFile.close() != IDE_SUCCESS);
        IDE_TEST(sDestFile.destroy() != IDE_SUCCESS);
        
        (void)idlOS::madvise((caddr_t)sSrcBase, (size_t)sStat.st_size, MADV_DONTNEED);
            
        IDE_TEST(idlOS::munmap(sSrcBase, sStat.st_size) != 0);
        
        IDE_TEST(sSrcFile.close() != IDE_SUCCESS);
        IDE_TEST(sSrcFile.destroy() != IDE_SUCCESS);
   }

    gettimeofday(&endTime, (void *)NULL);
    totalTime += CheckTime(&startTime, &endTime, i);
    gettimeofday(&startTime, (void *)NULL);               
    
    idlOS::printf("%15.2f seconds, %8.2f file(s) per sec\n",
                  totalTime/1000000.00,
                  (double)((sEnd-sStart)*1000000.0/totalTime));
    
    return;

    IDE_EXCEPTION_END;

    idlOS::printf("ERROR COPY!!\n");
    
    return;
}

IDE_RC smuCopyThread::shutdown()
{
    mFinish = ID_TRUE;

    IDE_TEST(join() != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}


/**********************************************************
** show time
**********************************************************/
double  smuCopyThread::ShowElapsed(int count, struct timeval *start_time, struct timeval *end_time)
{
    struct timeval v_timeval;
    double elapsedtime ;

    v_timeval.tv_sec  = end_time->tv_sec  - start_time->tv_sec;
    v_timeval.tv_usec = end_time->tv_usec - start_time->tv_usec;

    if (v_timeval.tv_usec < 0)
    {
        v_timeval.tv_sec -= 1;
        v_timeval.tv_usec = 999999 - v_timeval.tv_usec * (-1);
    }

    elapsedtime = v_timeval.tv_sec*1000000+v_timeval.tv_usec;

    //idlOS::printf(" [%d]", count);
    //idlOS::printf(" Elapsed Time ==>       %15.2f seconds\n", elapsedtime/1000000.00);
    
    return elapsedtime ;
}

double  smuCopyThread::CheckTime(struct timeval *sTime, struct timeval *eTime, int val)
{
    double elapsedtime;

    elapsedtime = ShowElapsed(val,sTime, eTime);
    return elapsedtime;
}

/*
void* thread_copy(void* aParam)
{
    UInt         i;
    UInt         sStart;
    UInt         sEnd;
    iduFile      sSrcFile;
    iduFile      sDestFile;
    struct  stat sStat;
    SChar*       sSrcBase;
    SChar*       sDestBase;
    SChar        sSrcLogFileName[512];
    SChar        sDestLogFileName[512];
    SChar        sSeed = 0;
    double       totalTime = 0.0;
    struct timeval startTime;
    struct timeval endTime;

    sStart = *(UInt*)aParam;
    sEnd = sStart + 10;

    idlOS::printf("copy %d ~ %d\n", sStart, sEnd);
    
    gettimeofday(&startTime, (void *)NULL);
    
    for (i = sStart ; i < sEnd; i++)
    {
        idlOS::sprintf(sSrcLogFileName, "%s%c%s%"ID_UINT32_FMT"", 
                       smuProperty::getLogDirPath(), 
                       IDL_FILE_SEPARATOR,
                       SMR_LOG_FILE_NAME, 
                       i);
        
        IDE_TEST(sSrcFile.initialize(IDU_MEM_SM_SMU) != IDE_SUCCESS);
        IDE_TEST(sSrcFile.setFileName(sSrcLogFileName) != IDE_SUCCESS);
        IDE_TEST(sSrcFile.open() != IDE_SUCCESS);
        
        IDE_TEST(idlOS::fstat(sSrcFile.getFileHandle(), &sStat) != 0);
        
        sSrcBase = (SChar*)idlOS::mmap(0, sStat.st_size, PROT_READ, 
                                       MAP_SHARED, sSrcFile.getFileHandle(), 0);
        
        idlOS::sprintf(sDestLogFileName, "%s%c%s%"ID_UINT32_FMT"", 
                       smuProperty::getArchiveDir(), 
                       IDL_FILE_SEPARATOR,
                       SMR_LOG_FILE_NAME, 
                       i);
        
        IDE_TEST(sDestFile.initialize(IDU_MEM_SM_SMU) != IDE_SUCCESS);
        IDE_TEST(sDestFile.setFileName(sDestLogFileName) != IDE_SUCCESS);
        
        IDE_TEST(sDestFile.create() != IDE_SUCCESS);
        
        IDE_TEST(sDestFile.open() != IDE_SUCCESS);

        // for performance
        IDE_TEST(sDestFile.write(sStat.st_size - 1, &sSeed, ID_SIZEOF(SChar))
                 != IDE_SUCCESS);

        if (gCopyMethod == 0)
        {
            IDE_TEST(sDestFile.write(0, sSrcBase, sStat.st_size)
                     != IDE_SUCCESS);
            
            IDE_TEST(sDestFile.sync() != IDE_SUCCESS);
        }
        else
        {
            sDestBase = (SChar*)idlOS::mmap(0, sStat.st_size, PROT_READ | PROT_WRITE, 
                                            MAP_PRIVATE, sDestFile.getFileHandle(), 0);
        
            idlOS::memcpy(sDestBase, sSrcBase, sStat.st_size);

            idlOS::msync((SChar*)sDestBase,
                         sStat.st_size,
                         MS_SYNC);
            
           (void)idlOS::madvise((caddr_t)sDestBase, (size_t)sStat.st_size, MADV_DONTNEED);
           
            IDE_TEST(idlOS::munmap(sDestBase, sStat.st_size) != 0);
         }
        
        IDE_TEST(sDestFile.close() != IDE_SUCCESS);
        IDE_TEST(sDestFile.destroy() != IDE_SUCCESS);
        
        (void)idlOS::madvise((caddr_t)sSrcBase, (size_t)sStat.st_size, MADV_DONTNEED);
            
        IDE_TEST(idlOS::munmap(sSrcBase, sStat.st_size) != 0);
        
        IDE_TEST(sSrcFile.close() != IDE_SUCCESS);
        IDE_TEST(sSrcFile.destroy() != IDE_SUCCESS);
   }

    gettimeofday(&endTime, (void *)NULL);
    totalTime += CheckTime(&startTime, &endTime, i);
    gettimeofday(&startTime, (void *)NULL);               
    
    idlOS::printf("%15.2f seconds, %8.2f mps \n",
                  totalTime/1000000.00,
                  (double)((sEnd-sStart)*1000000.0/totalTime));
    
    return NULL;

    IDE_EXCEPTION_END;

    idlOS::printf("ERROR COPY!!\n");
    
    return NULL;
}
*/

int main(int argc, char *argv[])
{
    UInt       i;
    smuCopyThread sThrArray[COPY_THREAD_COUNT];
    
    IDE_TEST(argc != 2);
    
    gCopyMethod = idlOS::atoi(argv[1]);
    
    /* Global Memory Manager initialize */
    IDE_TEST( iduMemMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );
    IDE_TEST( iduMutexMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );

    //fix TASK-3870
    (void)iduLatch::initializeStatic(IDU_CLIENT_TYPE);

    IDE_TEST(iduCond::initializeStatic() != IDE_SUCCESS);

    IDE_TEST(idp::initialize(NULL, NULL) != IDE_SUCCESS);

    IDE_TEST(iduProperty::load() != IDE_SUCCESS);
    IDE_TEST(smuProperty::load() != IDE_SUCCESS);

    for (i = 0; i < COPY_THREAD_COUNT; i++)
    {
        IDE_TEST(sThrArray[i].initialize((i*COPY_FILE_COUNT))
                 != IDE_SUCCESS);
    }
    
    for (i = 0; i < COPY_THREAD_COUNT; i++)
    {
        IDE_TEST(sThrArray[i].startThread() != IDE_SUCCESS);
    }

    for (i = 0; i < COPY_THREAD_COUNT; i++)
    {
        IDE_TEST(sThrArray[i].shutdown() != IDE_SUCCESS);
        IDE_TEST(sThrArray[i].destroy() != IDE_SUCCESS);
    }


    IDE_ASSERT( idp::destroy() == IDE_SUCCESS );
    IDE_ASSERT( iduCond::destroyStatic() == IDE_SUCCESS );
    (void)iduLatch::destroyStatic();
    IDE_ASSERT( iduMutexMgr::destroyStatic() == IDE_SUCCESS );
    IDE_ASSERT( iduMemMgr::destroyStatic() == IDE_SUCCESS );

    return 0;
        
    IDE_EXCEPTION_END;

    return -1;
}

/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduAIOQueue.cpp 66405 2014-08-13 07:15:26Z djin $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <ideLog.h>
#include <idErrorCode.h>
#include <iduAIOQueue.h>

#ifndef VC_WIN32

#define  IDU_AIO_FULL_SLEEP_TIME 1000

/* ------------------------------------------------
 *  AIO Queue Unit
 * ----------------------------------------------*/

IDE_RC iduAIOQueueUnit::initialize()
{
    IDE_TEST(mAIO.initialize() != IDE_SUCCESS);

    mStatus = IDU_AIO_QUEUE_NONE;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduAIOQueueUnit::destroy()
{
    return IDE_SUCCESS;
}


IDE_RC iduAIOQueueUnit::read(PDL_HANDLE aHandle, PDL_OFF_T  aWhere, void  *aBuffer, size_t aSize)
{
    IDE_ASSERT(mStatus != IDU_AIO_QUEUE_DOING);

    mStatus = IDU_AIO_QUEUE_DOING;

    IDE_TEST(mAIO.read(aHandle, aWhere, aBuffer, aSize) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduAIOQueueUnit::write(PDL_HANDLE aHandle, PDL_OFF_T  aWhere, void  *aBuffer, size_t aSize)
{
    IDE_ASSERT(mStatus != IDU_AIO_QUEUE_DOING);

    mStatus = IDU_AIO_QUEUE_DOING;

    IDE_TEST(mAIO.write(aHandle, aWhere, aBuffer, aSize) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

idBool iduAIOQueueUnit::isFinish(SInt *aErrorCode)
{
    return mAIO.isFinish(aErrorCode);
}

IDE_RC iduAIOQueueUnit::waitForFinish(SInt *aErrorCode)
{
    IDE_TEST(mAIO.waitForFinish(aErrorCode) != IDE_SUCCESS);

    mStatus = IDU_AIO_QUEUE_FINISH;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* ------------------------------------------------
 *  AIO Queue Main
 * ----------------------------------------------*/

IDE_RC iduAIOQueue::initialize(UInt aQueueCount)
{
    UInt i;

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_ID_ASYNC_IO_MANAGER,
                               sizeof(iduAIOQueueUnit) * aQueueCount,
                               (void **)&mUnitArray,
                               IDU_MEM_IMMEDIATE) != IDE_SUCCESS);

    mUnitCount = aQueueCount;
    mCurrUnit  = 0;

    for (i = 0; i < aQueueCount; i++)
    {
        IDE_TEST(mUnitArray[i].initialize() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

IDE_RC iduAIOQueue::destroy()
{
    UInt i;

    for (i = 0; i < mUnitCount; i++)
    {
        IDE_ASSERT(mUnitArray[i].getStatus() != IDU_AIO_QUEUE_DOING);

        IDE_TEST(mUnitArray[i].destroy() != IDE_SUCCESS);
    }
    IDE_TEST(iduMemMgr::free(mUnitArray) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


void  iduAIOQueue::refreshUnitArray()
{
    UInt i;

    for (i = 0; i < mUnitCount; i++)
    {
        if (mUnitArray[i].getStatus() == IDU_AIO_QUEUE_DOING)
        {
            SInt sErrorCode = 0;

            if (mUnitArray[i].isFinish(&sErrorCode) == ID_TRUE)
            {
                IDE_ASSERT(mUnitArray[i].waitForFinish(&sErrorCode) == IDE_SUCCESS);

                if (sErrorCode == 0)
                {
                    mUnitArray[i].setStatus(IDU_AIO_QUEUE_FINISH);
                }
                else
                {

                    ideLog::logErrorMsg(IDE_SERVER_0);
                    ideLog::log(IDE_SERVER_0, ID_TRC_AIO_FAILED, sErrorCode);
                    IDE_CALLBACK_FATAL("AIO Error Occurred!!");
                }
            }
        }
    }
}


UInt  iduAIOQueue::findEmptyUnit()
{
    UInt sSavedUnit;
    UInt sFoundUnit;

    sSavedUnit = mCurrUnit;

    refreshUnitArray();

    while(1)
    {
        if (mUnitArray[mCurrUnit].getStatus() != IDU_AIO_QUEUE_DOING)
        {
            sFoundUnit = mCurrUnit;
            mCurrUnit = ( (mCurrUnit + 1) % mUnitCount);
            break;
        }
        else
        {
            mCurrUnit = ( (mCurrUnit + 1) % mUnitCount);

            /* ------------------------------------------------
             * 한바퀴 모두 돈 상태이면 대기
             * ----------------------------------------------*/
            if (mCurrUnit == sSavedUnit)
            {
                PDL_Time_Value sPDL_Time_Value;
                sPDL_Time_Value.initialize( 0, IDU_AIO_FULL_SLEEP_TIME );
                idlOS::sleep(sPDL_Time_Value);
                refreshUnitArray();
            }
        }
    }
    return sFoundUnit;
}



IDE_RC iduAIOQueue::read (PDL_HANDLE aHandle, PDL_OFF_T  aWhere, void  *aBuffer, size_t aSize, UInt *aID)
{
    UInt sID;

    while(1)
    {
        sID = findEmptyUnit();

        if (mUnitArray[sID].read(aHandle, aWhere, aBuffer, aSize) != IDE_SUCCESS)
        {
            IDE_TEST(mUnitArray[sID].getErrno() != EAGAIN);
        }
        else
        {
            break;
        }
    }

    if (aID != NULL)
    {
        *aID = sID;
    }
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduAIOQueue::write(PDL_HANDLE aHandle, PDL_OFF_T  aWhere, void  *aBuffer, size_t aSize, UInt *aID)
{
    UInt sID;

    while(1)
    {
        sID = findEmptyUnit();

        if (mUnitArray[sID].write(aHandle, aWhere, aBuffer, aSize) != IDE_SUCCESS)
        {
            IDE_TEST(mUnitArray[sID].getErrno() != EAGAIN);
        }
        else
        {
            break;
        }
    }

    if (aID != NULL)
    {
        *aID = sID;
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduAIOQueue::waitForFinishAll()
{
    UInt i;
    UInt sEndCount = 0;

    while(sEndCount != mUnitCount)
    {
        refreshUnitArray();
        for (i = 0, sEndCount = 0; i < mUnitCount; i++)
        {
            if (mUnitArray[i].getStatus() != IDU_AIO_QUEUE_DOING)
            {
                sEndCount++;
            }
        }
        PDL_Time_Value sPDL_Time_Value;
        sPDL_Time_Value.initialize( 0, IDU_AIO_FULL_SLEEP_TIME );
        idlOS::sleep(sPDL_Time_Value);
    }

    return IDE_SUCCESS;
/*
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
*/
}

IDE_RC iduAIOQueue::waitForFinish(UInt aID)
{
    SInt sErrorCode;

    IDE_TEST(mUnitArray[aID].waitForFinish(&sErrorCode) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

void iduAIOQueue::dump()
{
    UInt i;
    ideLogEntry sLog(IDE_SERVER_0);

    sLog.append("Dumping Queue\n");
    for (i = 0; i < mUnitCount; i++)
    {
        sLog.appendFormat("   [%u] Stat[%u] \n",
                          i, (UInt)(mUnitArray[i].getStatus()));
    }

    sLog.write();
}
#endif

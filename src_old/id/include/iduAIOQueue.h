/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduAIOQueue.h 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

/***********************************************************************
 * This file is for Asynchronous I/O Class
 **********************************************************************/

#ifndef _O_IDU_AIO_QUEUE_H_
#define _O_IDU_AIO_QUEUE_H_ 1

#include <idu.h>
#include <idl.h>
#include <iduFileAIO.h>

typedef enum iduAIOQueueStatus
{
    IDU_AIO_QUEUE_NONE = 0,
    IDU_AIO_QUEUE_DOING,
    IDU_AIO_QUEUE_FINISH,

    IDU_AIO_QUEUE_MAX

}iduAIOQueueStatus;

class  iduAIOQueueUnit
{
    iduFileAIO        mAIO;
    iduAIOQueueStatus mStatus;

public:
    iduAIOQueueUnit()  {}
    ~iduAIOQueueUnit() {}

    IDE_RC initialize();
    IDE_RC destroy();

    IDE_RC read(PDL_HANDLE, PDL_OFF_T aWhere, void  *aBuffer, size_t aSize);
    IDE_RC write(PDL_HANDLE, PDL_OFF_T aWhere, void  *aBuffer, size_t aSize);
    idBool isFinish(SInt *aErrorCode);
    IDE_RC waitForFinish(SInt *aErrorCode);

    iduAIOQueueStatus getStatus() { return mStatus; }
    void setStatus(iduAIOQueueStatus aStatus) { mStatus = aStatus; }
    SInt getErrno() { return mAIO.getErrno();}


};

class iduAIOQueue
{
    UInt             mUnitCount;
    iduAIOQueueUnit *mUnitArray;
    UInt             mCurrUnit; // 빈 슬롯을 찾기 위해 이전 위치 저장함.

    void  refreshUnitArray();
    UInt  findEmptyUnit();

public:
    iduAIOQueue() {}
    ~iduAIOQueue() {}

    IDE_RC initialize(UInt aQueueCount);
    IDE_RC destroy();

    // invoke request writing
    IDE_RC read(PDL_HANDLE,  PDL_OFF_T aWhere, void  *aBuffer, size_t aSize, UInt *aID = NULL);
    IDE_RC write(PDL_HANDLE, PDL_OFF_T aWhere, void  *aBuffer, size_t aSize, UInt *aID = NULL);
    IDE_RC waitForFinishAll();
    IDE_RC waitForFinish(UInt aID);

    void   dump();

};

#endif  // _O_FILE_H_


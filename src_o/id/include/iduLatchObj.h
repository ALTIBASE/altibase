/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduLatchObj.h 84854 2019-01-31 09:47:48Z yoonhee.kim $
 **********************************************************************/

/* ------------------------------------------------
 *      !!!!!!!!!!!!!WARNING!!!!!!!!!!!!!!
 *
 *  이 헤더화일은 C 소스코드와 호환이 되도록
 *  구현되어야 합니다.
 * ----------------------------------------------*/

#ifndef _O_IDU_LATCH_OBJ_H_
#define _O_IDU_LATCH_OBJ_H_ 1

#include <idl.h>
#include <idv.h>
#include <iduMutex.h>
#include <iduCond.h>

/* BUGBUG : Back Compatibility smiDef.h : 871 line */
#define IDU_LATCH  UInt 
#define IDU_LATCH_MASK     (0x00000001)
#define IDU_LATCH_UNMASK   (0xFFFFFFFE)
#define IDU_LATCH_UNLOCKED (0x00000000)
#define IDU_LATCH_LOCKED   (0x00000001)
#define IDU_LATCH_RETRY_COUNT 10

#define IDU_LATCH_NAME_LENGTH (64)

typedef enum
{
    IDU_LATCH_TYPE_BEGIN = 0,
    IDU_LATCH_TYPE_POSIX = IDU_LATCH_TYPE_BEGIN,
    IDU_LATCH_TYPE_NATIVE,
    IDU_LATCH_TYPE_POSIX2,
    IDU_LATCH_TYPE_NATIVE2,
    IDU_LATCH_TYPE_MAX
} iduLatchType;


typedef enum
{
    IDU_LATCH_NONE = 0,
    IDU_LATCH_READ,
    IDU_LATCH_WRITE
    
} iduLatchMode;

class iduLatchCore
{
public:
    SChar*          mName;
    iduLatchType    mType;

    volatile SInt   mMode;
    ULong           mWriteThreadID; /* write-hold thread id */
    iduLatchCore*   mIdleLink;
    iduLatchCore*   mInfoLink;

    ULong           mGetReadCount;
    ULong           mGetWriteCount;
    ULong           mReadMisses;
    ULong           mWriteMisses;
    ULong           mSleepCount;
    SInt            mSpinCount;

    SChar*          mTypeString;
    SChar*          mModeString;
    SChar           mOwner[32];
    SChar           mPtrString[32];

    static SChar*   mTypes[IDU_LATCH_TYPE_MAX];
    static SChar*   mModes[3];

    inline iduLatchCore* getNextInfo() {return mInfoLink;}
};

class iduLatchObj : public iduLatchCore
{
public:
    virtual ~iduLatchObj() {};
    virtual IDE_RC initialize(SChar *aName)
    {
        mName           = aName;
        mMode           = 0;
        mGetReadCount   = 0;
        mGetWriteCount  = 0;
        mReadMisses     = 0;
        mWriteMisses    = 0;
        mSpinCount      = 0;
        mSleepCount     = 0;
        mWriteThreadID  = 0;

        return IDE_SUCCESS;
    }
    virtual IDE_RC destroy() = 0;
  
    virtual IDE_RC tryLockRead(idBool*  aSuccess, void* aStatSQL = NULL) = 0;
    virtual IDE_RC tryLockWrite(idBool* aSuccess, void* aStatSQL = NULL) = 0;
    virtual IDE_RC lockRead(void*  aStatSQL, void* aWeArgs) = 0;
    virtual IDE_RC lockWrite(void* aStatSQL, void* aWeArgs) = 0;
    virtual IDE_RC unlock( idBool* aIsUnlockedAll = NULL ) = 0;
    virtual IDE_RC unlockWriteAll() = 0;
};

class iduLatchPosix : public iduLatchObj
{
public:
    iduMutex    mMutex;

    virtual IDE_RC initialize(SChar *aName);
    virtual IDE_RC destroy();
  
    virtual IDE_RC tryLockRead(idBool*  aSuccess, void* aStatSQL = NULL);
    virtual IDE_RC tryLockWrite(idBool* aSuccess, void* aStatSQL = NULL);
    virtual IDE_RC lockRead(void*  aStatSQL, void* aWeArgs);
    virtual IDE_RC lockWrite(void* aStatSQL, void* aWeArgs);
    virtual IDE_RC unlock( idBool* aIsUnlockedAll = NULL );
    virtual IDE_RC unlockWriteAll();
};

class iduLatchPosix2 : public iduLatchObj
{
public:
    iduMutex    mMutex;
    SInt        mXPending;

    virtual IDE_RC initialize(SChar *aName);
    virtual IDE_RC destroy();
  
    virtual IDE_RC tryLockRead(idBool*  aSuccess, void* aStatSQL = NULL);
    virtual IDE_RC tryLockWrite(idBool* aSuccess, void* aStatSQL = NULL);
    virtual IDE_RC lockRead(void*  aStatSQL, void* aWeArgs);
    virtual IDE_RC lockWrite(void* aStatSQL, void* aWeArgs);
    virtual IDE_RC unlock( idBool* aIsUnlockedAll = NULL );
    virtual IDE_RC unlockWriteAll();
};

class iduLatchNative : public iduLatchObj
{
public:
    iduNativeMutexObj   mMutex;
    SChar               mLatchName[IDU_LATCH_NAME_LENGTH];

    virtual IDE_RC initialize(SChar *aName);
    virtual IDE_RC destroy();
  
    virtual IDE_RC tryLockRead(idBool*  aSuccess, void* aStatSQL = NULL);
    virtual IDE_RC tryLockWrite(idBool* aSuccess, void* aStatSQL = NULL);
    virtual IDE_RC lockRead(void*  aStatSQL, void* aWeArgs);
    virtual IDE_RC lockWrite(void* aStatSQL, void* aWeArgs);
    virtual IDE_RC unlock( idBool* aIsUnlockedAll = NULL );
    virtual IDE_RC unlockWriteAll();

    void sleepForLatchValueChange(SInt aFlag);
};

class iduLatchNative2 : public iduLatchObj
{
public:
    SChar   mLatchName[IDU_LATCH_NAME_LENGTH];

    virtual IDE_RC initialize(SChar *aName);
    virtual IDE_RC destroy();
  
    virtual IDE_RC tryLockRead(idBool*  aSuccess, void* aStatSQL = NULL);
    virtual IDE_RC tryLockWrite(idBool* aSuccess, void* aStatSQL = NULL);
    virtual IDE_RC lockRead(void*  aStatSQL, void* aWeArgs);
    virtual IDE_RC lockWrite(void* aStatSQL, void* aWeArgs);
    virtual IDE_RC unlock( idBool* aIsUnlockedAll = NULL );
    virtual IDE_RC unlockWriteAll();

    void sleepForLatchValueChange(SInt aFlag);
};

#endif 
    

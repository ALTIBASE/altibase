/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduFixedTable.h 74637 2016-03-07 06:01:43Z donovan.seo $
 **********************************************************************/
#ifndef _O_IDU_FIXED_TABLE_H_
# define _O_IDU_FIXED_TABLE_H_ 1

#include <iduFixedTableDef.h>
#include <iduMemory.h>

class iduFixedTable
{
    static iduFixedTableDesc  mTableHead; // Dummy
    static iduFixedTableDesc *mTableTail;
    static UInt               mTableCount;
    static UInt               mColumnCount;
    static iduFixedTableAllocRecordBuffer mAllocCallback;
    static iduFixedTableBuildRecord       mBuildCallback;
    static iduFixedTableCheckKeyRange     mCheckKeyRangeCallback;

public:
    // BUG-36588
    static void   initialize( void );
    static void   finalize( void );

    static void   registFixedTable(iduFixedTableDesc *);
    static void   setCallback(iduFixedTableAllocRecordBuffer aAlloc,
                              iduFixedTableBuildRecord       aBuild,
                              iduFixedTableCheckKeyRange     aCheckKeyRange);
    static iduFixedTableDesc* getTableDescHeader()
    {
        return mTableHead.mNext;
    }
    static IDE_RC allocRecordBuffer(void *aHeader,
                                    UInt aRecordCount,
                                    void **aBuffer)
    {
        return mAllocCallback(aHeader, aRecordCount, aBuffer);
    }

    static IDE_RC buildRecord(void                *aHeader,
                              iduFixedTableMemory *aMemory,
                              void      *aObj)
    {
        return mBuildCallback(aHeader, aMemory,  aObj);
    }

    static idBool checkKeyRange( iduFixedTableMemory   * aMemory,
                                 iduFixedTableColDesc  * aColDesc,
                                 void                 ** aObj )
    {
        return mCheckKeyRangeCallback( aMemory, aColDesc, aObj );
    }

    // 이 함수는 X$TAB를 생성하기 위해 스스로 만든 build 함수
    static IDE_RC buildRecordForSelfTable( idvSQL      *aStatistics,
                                           void        *aHeader,
                                           void        *aDumpObj,
                                           iduFixedTableMemory *aMemory );

    // 이 함수는 X$COL를 생성하기 위해 스스로 만든 build 함수
    static IDE_RC buildRecordForSelfColumn( idvSQL      *aStatistics,
                                            void        *aHeader,
                                            void        *aDumpObj,
                                            iduFixedTableMemory *aMemory );
};

// 자동으로 Fixed Table을 생성하고, 등록하도록 하는 클래스

/*
 * ===================== WARNING!!!!!!!!!!!!! ====================
 * ObjName에 iduFixedTableDesc의 포인터대신에 객체를 넘겨야 한다.
 * 왜냐하면, 동일화일 내에서 다수로 정의된 iduFixedTableDesc의
 * 전역 객체의 이름을 서로 다른 형태로 생성하기 위해
 * 프리프로세서의 ## 기능을 이용하도록 하였기 때문이다.
 * 자동으로 포인터 타입으로 넘어가도록 하였기 때문에
 * 객체의 이름을 넘겨야 한다.
 * ===================== WARNING!!!!!!!!!!!!! ====================

 EXAMPLE)

 iduFixedTableDesc gSampleTable =
 {
 (SChar *)"SampleFixedTable",
 buildRecordCallback,
 gSampleTableColDesc,
 IDU_STARTUP_INIT,
 0, NULL
 };

 IDU_FIXED_TABLE_DEFINE(gSampleTable);   <== OK

 IDU_FIXED_TABLE_DEFINE(&gSampleTable);  <== Wrong!!!

*/

#define IDU_FIXED_TABLE_LOGGING_POINTER()       \
    {                                           \
        mRollbackBeginRecord  = mBeginRecord;   \
        mRollbackBeforeRecord = mBeforeRecord;  \
        mRollbackCurrRecord   = mCurrRecord;    \
    }

#define IDU_FIXED_TABLE_ROLLBACK_POINTER()      \
    {                                           \
        mBeginRecord  = mRollbackBeginRecord;   \
        mBeforeRecord = mRollbackBeforeRecord;  \
        mCurrRecord   = mRollbackCurrRecord;    \
    }

class iduFixedTableMemory
{
    iduMemory mMemory;
    iduMemory *mMemoryPtr;
    // Current Usage Pointer
    UChar    *mBeginRecord;
    UChar    *mBeforeRecord; // or final record..it depends on operation.
    UChar    *mCurrRecord;    // or final record..it depends on operation.

    // For Rollback in filter-false case.
    UChar    *mRollbackBeginRecord;
    UChar    *mRollbackBeforeRecord;
    UChar    *mRollbackCurrRecord;

    void             *mContext;
    iduMemoryStatus   mMemStatus;

    idBool            mUseExternalMemory;
public:
    iduFixedTableMemory() {}
    ~iduFixedTableMemory() {}

    IDE_RC initialize( iduMemory * aMemory );
    IDE_RC restartInit();
    IDE_RC allocateRecord(UInt aSize, void **aMem);
    IDE_RC initRecord(void **aMem);
    void   freeRecord();
    void   resetRecord();

    IDE_RC destroy();

    UChar *getBeginRecord()            { return mBeginRecord;  }
    UChar *getCurrRecord()             { return mCurrRecord;   }
    UChar *getBeforeRecord()           { return mBeforeRecord; }
    void  *getContext()                { return mContext;      }
    void   setContext(void * aContext) { mContext = aContext;  }
    idBool useExternalMemory()         { return mUseExternalMemory;  }
};


class iduFixedTableRegistBroker
{
public:
    iduFixedTableRegistBroker(iduFixedTableDesc *aDesc);
};

#define IDU_FIXED_TABLE_DEFINE_EXTERN(ObjName)                  \
    extern iduFixedTableDesc ObjName;

#define IDU_FIXED_TABLE_DEFINE(ObjName)                         \
    extern iduFixedTableDesc ObjName;                           \
    static iduFixedTableRegistBroker ObjName##ObjName(&ObjName)

// PROJ-1726
// 동적 모듈에서 <모듈>im::initSystemTable 에서
// 동적으로 fixed table 을 등록하기 위해 사용.

#define IDU_FIXED_TABLE_DEFINE_RUNTIME(ObjName)                 \
    iduFixedTable::registFixedTable(&ObjName);

#endif /* _O_IDU_FIXED_TABLE_H_ */

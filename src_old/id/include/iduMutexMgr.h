/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMutexMgr.h 84854 2019-01-31 09:47:48Z yoonhee.kim $
 **********************************************************************/
#ifndef _O_IDU_MUTEX_MGR_H_
#define _O_IDU_MUTEX_MGR_H_ 1

#include <iduMemDefs.h>
#include <idl.h>
#include <iduMutexEntry.h>

class iduMemSmall;
class iduMemPool;

class iduMutexMgr
{
private:
    /*
     * Project 2379 - Limit max count of mutex
     */
    static IDE_RC makeNewEntry(iduMutexEntry**, iduMutexKind);

    static iduMutexEntry    mPosixEntry;
    static iduMutexEntry    mNativeEntry;
    static iduMutexEntry    mInfoHeadEntry;
    static iduMutexEntry    mInfoTailEntry;
    static ULong            mPoolCount;
    static ULong            mPoolMaxCount;

    //fix PROJ-1749
    static iduMutexOP*      mMutexOpArrayServer[IDU_MUTEX_KIND_MAX];
    static iduMutexOP*      mMutexOpArrayClient[IDU_MUTEX_KIND_MAX];
    
    //fix PROJ-1749
    static iduPeerType      mMutexMgrType;

    /*
     * Project 2408 - Memory allocator renewal
     */
    static IDE_RC           allocEntry(iduMutexEntry**);
    static IDE_RC           freeEntry(iduMutexEntry*);
    static iduMemSmall      mMutexPool;
    static idBool           mUsePool;

    
public:
    static void   lockPosix()       { mPosixEntry.lock(NULL);     }
    static void   unlockPosix()     { mPosixEntry.unlock();       }
    static void   lockNative()      { mNativeEntry.lock(NULL);    }
    static void   unlockNative()    { mNativeEntry.unlock();      }
    static void   lockInfoHead()    { mInfoHeadEntry.lock(NULL);  }
    static void   unlockInfoHead()  { mInfoHeadEntry.unlock();    }
    static void   lockInfoTail()    { mInfoTailEntry.lock(NULL);  }
    static void   unlockInfoTail()  { mInfoTailEntry.unlock();    }

    static IDE_RC initializeStatic(iduPeerType aMutexMgrType);
    static IDE_RC destroyStatic();
    
    static IDE_RC alloc(iduMutexEntry **aEntry, iduMutexKind aKind);
    static IDE_RC free(iduMutexEntry *aEntry);
    static IDE_RC freeIdles();

    static iduMutexEntry* getRoot() { return &mPosixEntry; }
    static iduMutexEntry* getInfo() { return &mInfoHeadEntry; }
    static iduMutexEntry* getInfoTail() { return &mInfoTailEntry; }
    static void setPoolMaxCount( UInt aPoolMaxSizeInMB ) 
           {
               ULong sMaxInByte;

               sMaxInByte= (ULong)aPoolMaxSizeInMB * 1024 * 1024;
               mPoolMaxCount = ( sMaxInByte / ID_SIZEOF(iduMutexEntry) ) + 1;
           }
   
    //fix PROJ-1749 
    static iduMutexOP* getMutexOP( iduMutexKind aKind )
           { 
               if (mMutexMgrType == IDU_SERVER_TYPE) 
               {
                   return mMutexOpArrayServer[aKind];
               }
               else
               {
                   return mMutexOpArrayClient[aKind];
               }
           }

    static void unlockAllMyThread();
};

#endif	// _O_MUTEX_MGR_H_

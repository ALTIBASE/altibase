/***********************************************************************
 * Copyright 2001, AltiBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduSync.h 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

#ifndef _O_IDU_SYNC_H_
# define _O_IDU_SYNC_H_

#define IDU_SYNC_RELEASED (0)
#define IDU_SYNC_REQUIRED (1)
#define IDU_SYNC_ACQUIRED (2)

#include <idl.h>

typedef vULong iduSyncWord;

class iduSync
{
 public:
    static void initialize( iduSyncWord* aWords,
                            UInt         aWordMax );
    
    static void acquire2( UInt         aWordNo,
                          iduSyncWord* aWords );
    
    static void acquire( UInt         aWordNo,
                         iduSyncWord* aWords,
                         UInt         aWordMax );
    
    static void release( UInt         aWordNo,
                         iduSyncWord* aWords );
};

inline void iduSync::initialize( iduSyncWord* aWords,
                                 UInt         aWordMax )
{
    UInt sCount;
    
    for( sCount = 0; sCount < aWordMax; sCount++ )
    {
        aWords[sCount] = IDU_SYNC_RELEASED;
    }
}
    
inline void iduSync::acquire2( UInt         aWordNo,
                               iduSyncWord* aWords )
{
    UInt sCount;

    sCount          = 1 - aWordNo;
    aWords[aWordNo] = IDU_SYNC_REQUIRED;
    IDL_MEM_BARRIER;
    
    while( aWords[sCount] == IDU_SYNC_REQUIRED )
    {
        aWords[aWordNo] = IDU_SYNC_RELEASED;
        idlOS::thr_yield();

        IDL_MEM_BARRIER;
        aWords[aWordNo] = IDU_SYNC_REQUIRED;
    }
    while( aWords[sCount] == IDU_SYNC_ACQUIRED )
    {
        idlOS::thr_yield();
    }
    aWords[aWordNo] = IDU_SYNC_ACQUIRED;
    IDL_MEM_BARRIER;
}

inline void iduSync::acquire( UInt         aWordNo,
                              iduSyncWord* aWords,
                              UInt         aWordMax )
{
    UInt sCount;
    UInt sRequired;
    UInt sAcquired;

    sCount          = 0;
    sRequired       = 0;
    sAcquired       = ID_UINT_MAX;
    aWords[aWordNo] = IDU_SYNC_REQUIRED;
    for( ; sCount < aWordMax; sCount++ )
    {
        switch( aWords[sCount] )
        {
         case IDU_SYNC_REQUIRED:
            sRequired++;
            break;
         case IDU_SYNC_ACQUIRED:
            sAcquired = sCount;
            break;
        }
    }
    while( sRequired != 1 )
    {
        aWords[aWordNo] = IDU_SYNC_RELEASED;
        idlOS::thr_yield();
        sCount          = 0;
        sRequired       = 0;
        sAcquired       = ID_UINT_MAX;
        aWords[aWordNo] = IDU_SYNC_REQUIRED;
        for( ; sCount < aWordMax; sCount++ )
        {
            switch( aWords[sCount] )
            {
             case IDU_SYNC_REQUIRED:
                sRequired++;
                break;
             case IDU_SYNC_ACQUIRED:
                sAcquired = sCount;
                break;
            }
        }
    }
    if( sAcquired != ID_UINT_MAX )
    {
        while( aWords[sAcquired] == IDU_SYNC_ACQUIRED )
        {
            idlOS::thr_yield();
        }
    }
    aWords[aWordNo] = IDU_SYNC_ACQUIRED;
}

inline void iduSync::release( UInt         aWordNo,
                              iduSyncWord* aWords )
{
    IDL_MEM_BARRIER;
    aWords[aWordNo] = IDU_SYNC_RELEASED;
}

#endif /* _O_IDU_SYNC_H_ */

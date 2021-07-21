/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMutex.h 86279 2019-10-15 01:10:42Z hykim $
 **********************************************************************/
#ifndef _O_IDU_MUTEX_H_
#define _O_IDU_MUTEX_H_ 1

#include <idl.h>
#include <iduMutexMgr.h>

class iduMutex
{
public:

    /*
     * mutex의 종류와 측정할 wait event를 명시한다.
     * wait event는 idv.h에 직접 정의하거나 참조한다
     */
    IDE_RC initialize(const SChar *aName,
                      iduMutexKind aKind,
                      idvWaitIndex aWaitEventID );
    
    IDE_RC destroy();
    
    inline IDE_RC trylock(idBool        & bLock );

    /*
     * 초기화시 명시된 wait event의 wait time을 측정하려면
     * Session 통계자료구조인 idvSQL를 인자로 전달해야한다.
     * TIMED_STATISTICS 프로퍼티가 1 이어야 측정된다. 
     */
    inline IDE_RC lock( idvSQL        * aStatSQL );

    inline IDE_RC unlock();

    void  status() {}
    
    inline PDL_thread_mutex_t* getMutexForCond();
    inline iduMutexStat* getMutexStat();
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* Reset statistic info and change name of a Mutex */
    /* ex> mmcMutexPool use these functions for init. a mutex */
    inline void resetStat();
    inline void setName( SChar *aName );
    
    // BUGBUG : BUG-14263
    // for make POD class type, remove copy operator.
    //// Prohibit assign : To Fix PR-4073
    // iduMutex& operator=(const iduMutex& ) { return  *this;};

    static inline IDE_RC unlock( void *aMutex, UInt, void * );
    static inline idBool isMutexSame( void *aLhs, void *aRhs );
    static IDE_RC dump( void * ) { return IDE_SUCCESS; }

private:
    // BUGBUG : BUG-14263
    // for make POD class type, remove copy operator.
    ////Prohibit automatic compile generation of these member function
    //iduMutex(const iduMutex& );
    
public:  /* POD class type should make non-static data members as public */
    iduMutexEntry    *mEntry;
};

iduMutexStat* iduMutex::getMutexStat()
{
#if !defined(SMALL_FOOTPRINT) || defined(WRS_VXWORKS)
    return &(mEntry->mStat);
#else
    return NULL;
#endif
}

PDL_thread_mutex_t* iduMutex::getMutexForCond()
{
    return (PDL_thread_mutex_t *)(mEntry->getMutexForCondWait());
}

inline IDE_RC iduMutex::trylock( idBool  & bLock )
{
    mEntry->trylock( &bLock );

    return IDE_SUCCESS;
}

inline IDE_RC iduMutex::lock( idvSQL * aStatSQL )
{
    mEntry->lock( aStatSQL );

    return IDE_SUCCESS;
}

inline IDE_RC iduMutex::unlock()
{
    mEntry->unlock();

    return IDE_SUCCESS;
}

/* --------------------------------------------------------------------
 * mtx commit 시에 latch를 풀어주기 위한 함수
 * sdbBufferMgr과 i/f를 맞추기 위해 2개의 인자가 추가되었으나
 * 사용되지 않는다.
 * ----------------------------------------------------------------- */
inline IDE_RC iduMutex::unlock( void *aMutex, UInt, void * )
{
    IDE_DASSERT( aMutex != NULL );

    return ( ((iduMutex *)aMutex)->unlock() );
}

/* --------------------------------------------------------------------
 * 2개의 latch가 같은 지 비교한다.
 * sdrMiniTrans에서 스택에 있는 특정 아이템을 찾을 때 사용된다.
 * 단순히 포인터가 같은지 비교한다. -> 비교할 아이템을 찾지 못했음.
 * ----------------------------------------------------------------- */
inline idBool iduMutex::isMutexSame( void *aLhs, void *aRhs )
{
    IDE_DASSERT( aLhs != NULL );
    IDE_DASSERT( aRhs != NULL );

    return ( aLhs == aRhs ? ID_TRUE : ID_FALSE );
}

/* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
inline void iduMutex::resetStat()
{
    mEntry->resetStat();
}

inline void iduMutex::setName(SChar *aName)
{
    mEntry->setName( aName );
}

#endif	// _O_MUTEX_H_

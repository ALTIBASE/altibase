/***********************************************************************
 * Copyright 1999-2000, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduSema.h 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/


/* ------------------------------------------------
 *  Process간의 Semaphore연산을
 *  Simulation 하는 코드임. excerpted from OS.h
 * ----------------------------------------------*/

#ifndef _O_IDU_SEMA_H_
#define _O_IDU_SEMA_H_   1

#include <idl.h>


struct iduSema_t
{
  PDL_mutex_t lock_;
  PDL_cond_t count_nonzero_;
  UInt count_;
  UInt waiters_;
};

class iduSema
{
public:
    static IDE_RC initialize(iduSema_t *, UInt aCount);
    static IDE_RC destroy(iduSema_t *);
    
    static IDE_RC post    (iduSema_t *);
    static IDE_RC post    (iduSema_t *, size_t aReleaseCount);
    static IDE_RC trywait (iduSema_t *, idBool *aSuccess);
    static IDE_RC wait    (iduSema_t *);
    static IDE_RC wait    (iduSema_t *, PDL_Time_Value *);
};

#endif

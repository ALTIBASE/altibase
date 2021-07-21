/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idmDef.h 67796 2014-12-03 08:39:33Z donlet $
 **********************************************************************/

#ifndef _O_IDM_DEF_H_
# define _O_IDM_DEF_H_ 1

# include <idTypes.h>

/* idmModule.type                            */
# define IDM_TYPE_NONE             (0x00000000)
# define IDM_TYPE_INTEGER          (0x00000002)
# define IDM_TYPE_STRING           (0x00000004)

/* idmModule.flag                            */
# define IDM_FLAG_HAVE_CHILD_MASK  (0x00000001)
# define IDM_FLAG_HAVE_CHILD_TRUE  (0x00000001)
# define IDM_FLAG_HAVE_CHILD_FALSE (0x00000000)

typedef struct idmId     idmId;
typedef struct idmModule idmModule;

typedef IDE_RC (*idmInitFunc)( idmModule* aModule );

typedef IDE_RC (*idmFinalFunc)( idmModule* aModule );

typedef IDE_RC (*idmGetNextIdFunc)( const idmModule* aModule,
                                    const idmId*     aPreviousId,
                                    idmId*           aId,
                                    UInt             aIdMaximum );

typedef IDE_RC (*idmGetFunc)( const idmModule* aModule,
                              const idmId*     aId,
                              UInt*            aType,
                              void*            aValue,
                              UInt*            aLength,
                              UInt             aMaximum );

typedef IDE_RC (*idmSetFunc)( idmModule*   aModule,
                              const idmId* aId,
                              UInt         aType,
                              const void*  aValue,
                              UInt         aLength );

/* 
 * PROJ-2473 SNMP 지원
 *
 * net-snmp의 id는 u_long 타입이다. 
 * net-snmp에서 사용하는 oid 타입을 선언하자.
 */
typedef vULong oid;

struct idmId {
    UInt length;
    oid  id[1];
};

struct idmModule {
    UChar*           name;
    idmModule*       parent;
    idmModule*       child;
    idmModule*       brother;
    idmModule*       next;
    void*            data;
    oid              id;
    UInt             type;
    UInt             flag;
    UInt             depth;
    idmInitFunc      init;
    idmFinalFunc     final;
    idmGetNextIdFunc getNextId;
    idmGetFunc       get;
    idmSetFunc       set;
};

#endif /* _O_IDM_DEF_H_ */

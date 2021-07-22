/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/***********************************************************************
 * $Id: idkAtomic.h,v 1.7 2006/06/02 08:43:52 alex Exp $
 **********************************************************************/

#ifndef _IDK_ATOMIC_
#define _IDK_ATOMIC_   1


IDL_EXTERN_C_BEGIN

/* value inccrement */
UChar   idkAtomicINC1( void *aPtr);
UShort  idkAtomicINC2( void *aPtr);
UInt    idkAtomicINC4( void *aPtr);
ULong   idkAtomicINC8( void *aPtr);

/* value decriment */
UChar   idkAtomicDEC1( void *aPtr);
UShort  idkAtomicDEC2( void *aPtr);
UInt    idkAtomicDEC4( void *aPtr);
ULong   idkAtomicDEC8( void *aPtr);

/* value add */
UChar   idkAtomicADD1( void *aPtr, UChar  aDelta);
UShort  idkAtomicADD2( void *aPtr, UShort aDelta);
UInt    idkAtomicADD4( void *aPtr, UInt   aDelta);
ULong   idkAtomicADD8( void *aPtr, ULong  aDelta);

/* Compare And Swap CAS/CMPXCHG operations for a 1/2/4/8 bytes *
 * If *arg1 == arg2, set *arg1 = arg3; return old value        */
UChar  idkAtomicCAS1( void *, UChar    , UChar );
UShort idkAtomicCAS2( void *, UShort   , UShort);
UInt   idkAtomicCAS4( void *, UInt     , UInt  );
ULong  idkAtomicCAS8( void *, ULong    , ULong );

#define  idkAtomicRead2(x)  idkAtomicCAS2( &x, 0, 0)
#define  idkAtomicRead4(x)  idkAtomicCAS4( &x, 0, 0)
#define  idkAtomicRead8(x)  idkAtomicCAS8( &x, 0, 0)


#define  idkAtomicWrite2(aVal, aNew)                            \
    {                                                           \
        UShort sOld = aVal;                                     \
        while( sOld != idkAtomicCAS2( &aVal, sOld, aNew) )      \
        {                                                       \
            idlOS::thr_yield();                                 \
            sOld = aVal;                                        \
        }                                                       \
    }

#define  idkAtomicWrite4(aVal, aNew)                            \
    {                                                           \
        UInt sOld = aVal;                                       \
        while( sOld != idkAtomicCAS4( &aVal, sOld, aNew) )      \
        {                                                       \
            idlOS::thr_yield();                                 \
            sOld = aVal;                                        \
        }                                                       \
    }

#define  idkAtomicWrite8(aVal, aNew)                            \
    {                                                           \
        ULong sOld = aVal;                                      \
        while( sOld != idkAtomicCAS8( &aVal, sOld, aNew) )      \
        {                                                       \
            idlOS::thr_yield();                                 \
            sOld = aVal;                                        \
        }                                                       \
    }


#if defined(COMPILE_64BIT)
# define idkAtomicCASP( aPtr, aOld, aNew) (void*)idkAtomicCAS8( (aPtr), (ULong)(aOld), (ULong)(aNew))
#else
# define idkAtomicCASP( aPtr, aOld, aNew) (void*)idkAtomicCAS4( (aPtr), (UInt)(aOld), (UInt)(aNew))
#endif

IDL_EXTERN_C_END


#endif

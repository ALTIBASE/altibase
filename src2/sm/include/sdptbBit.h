/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: sdptbBit.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * 비트맵을 효율적으로 관리하는 유틸리티 함수들이다.
 ***********************************************************************/

#ifndef _O_SDPTB_BIT_H_
#define _O_SDPTB_BIT_H_ 1

#include <sdptb.h>
#include <sdp.h>

/* BUG-47666 little endian, big endian 에 구분되지 않고
 *           사용 할 수 있도록 다음과 같이 합니다. 
 *           길이를 줄이고 바로 이해하기 쉽도록
 *           #define을 사용하지 않고 숫자를 사용합니다.
 *           32 : Int 의 bit size
 *           8  : char의 bit size
 *
 * UInt  [] : [         ][          ][          ]
 * UChar [] :            [ ][ ][V][ ]        : aIndex % 32 / 8
 *                               = 00100000  : 1 << ( aIndex % 8 ) */
#define  SDPTB_SET_BIT_FOR_INT( aBit, aIndex )  \
      (aBit) = 0;                              \
      ((UChar*)&(aBit))[(((aIndex) % 32)/ 8)] = (1 << ( (aIndex) % 8 ));
     
class sdptbBit {
public:
    static IDE_RC initialize(){ return IDE_SUCCESS; }
    static IDE_RC destroy(){ return IDE_SUCCESS; }

    /*
     * 특정 주소값으로 부터 aIndex번째의 비트를 1로 만든다
     */
    inline static void setBit( void * aAddr, UInt aIndex)
    {
        UInt sNBytes = aIndex / SDPTB_BITS_PER_BYTE ;
        UInt sRest = aIndex % SDPTB_BITS_PER_BYTE;

        *((UChar *)aAddr + sNBytes) |= (1 << sRest);
    }

    /* BUG-47666 atomic 으로 동시성을 제어하는 set bit
     * int 로 처리하므로 aAddr 은 반드시 4Byte Align 이 맞아야 한다. */
    inline static void atomicSetBit32( UInt * aAddr, UInt aIndex )
    {
        UInt  sBefore ;
        UInt  sBit;

        IDE_DASSERT( ( (ULong)aAddr & 0x0000000000000003 ) == 0 );

        SDPTB_SET_BIT_FOR_INT( sBit, aIndex );

        aAddr += ( aIndex / SDPTB_BITS_PER_UINT );

        do{
            sBefore = idCore::acpAtomicGet32( aAddr );

            if ( ( sBefore & sBit ) == sBit )
            {
                /* 이미 Set 되어 있으면 취소 */
                break;
            }

        }while ( (UInt)idCore::acpAtomicCas32( aAddr,          // position
                                               sBefore | sBit, // set
                                               sBefore )       // compage
                 != sBefore );
    }
    /*
     * 특정 주소값으로 부터 aIndex번째의 비트를 0으로 만든다.
     */
    inline static void clearBit( void * aAddr, UInt aIndex )
    {
        UInt sNBytes = aIndex / SDPTB_BITS_PER_BYTE ;
        UInt sRest = aIndex % SDPTB_BITS_PER_BYTE;

        *((UChar *)aAddr + sNBytes) &= ~(1 << sRest);
    }

    /* BUG-47666 atomic 으로 동시성을 제어하는 set bit
     * int 로 처리하므로 aAddr 은 반드시 4Byte Align 이 맞아야 한다. */
    inline static void atomicClearBit32( UInt * aAddr, UInt aIndex )
    {
        UInt  sBefore ;
        UInt  sBit;

        IDE_DASSERT( ( (ULong)aAddr & 0x0000000000000003 ) == 0 );

        SDPTB_SET_BIT_FOR_INT( sBit, aIndex );

        aAddr += ( aIndex / SDPTB_BITS_PER_UINT );

        do{
            sBefore = idCore::acpAtomicGet32( aAddr );

            if ( ( sBefore & sBit ) == 0 )
            {
                /* 이미 Clear 되어 있으면 취소 */
                break;
            }

        }while ( (UInt)idCore::acpAtomicCas32( aAddr,             // position
                                               sBefore & (~sBit), // clear
                                               sBefore )          // compage
                 != sBefore );
    }

    /*
     * 특정 주소값으로 부터 aIndex번째의 비트가 0인지 1인지를 알아낸다.
     */
    inline static idBool getBit( void * aAddr, UInt  aIndex )

    {
        UInt    sNBytes = aIndex / SDPTB_BITS_PER_BYTE ;
        UInt    sRest = aIndex % SDPTB_BITS_PER_BYTE;
        idBool  sVal;

        IDL_MEM_BARRIER; // BUG-47666

        if( *((UChar *)aAddr + sNBytes) & (1 << sRest))
        {
            sVal = ID_TRUE;
        }
        else
        {
            sVal = ID_FALSE;
        }

        return sVal;
    }

    static void findZeroBit( void * aAddr,
                             UInt   aCount,
                             UInt * aBitIdx);

    static void findBit( void * aAddr,
                         UInt   aCount,
                         UInt * aBitIdx);

    static void findBitFromHint( void *  aAddr,
                                 UInt    aCount,
                                 UInt    aHint,
                                 UInt *  aBitIdx);

    static void findBitFromHintRotate( void *  aAddr,
                                       UInt    aCount,
                                       UInt    aHint,
                                       UInt *  aBitIdx);

    static void findZeroBitFromHint( void *  aAddr,
                                     UInt    aCount,
                                     UInt    aHint,
                                     UInt *  aBitIdx);

    /*특정주소값으로부터  aCount개의 비트를 대상으로 1인 비트의 합을 구한다*/
    static void sumOfZeroBit( void * aAddr,
                              UInt   aCount,
                              UInt * aRet);
};

#endif //_O_SDPTB_BIT_H_

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
 * $Id: sdptbBit.cpp 27228 2008-07-23 17:36:52Z newdaily $
 **********************************************************************/

#include <sdptb.h>

//255는 안들어옴.
static UChar gZeroBitIdx[256] =
{
    0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,4 ,
    0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,5 ,
    0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,4 ,
    0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,6 ,
    0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,4 ,
    0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,5 ,
    0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,4 ,
    0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,7 ,
    0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,4 ,
    0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,5 ,
    0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,4 ,
    0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,6 ,
    0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,4 ,
    0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,5 ,
    0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,4 ,
    0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,8
};

//0은 안들어옴.
static UChar gBitIdx[256] =
{
    8 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,
    4 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,
    5 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,
    4 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,
    6 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,
    4 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,
    5 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,
    4 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,
    7 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,
    4 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,
    5 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,
    4 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,
    6 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,
    4 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,
    5 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,
    4 ,0 ,1 ,0 ,2 ,0 ,1 ,0 ,3 ,0 ,1 ,0 ,2 ,0 ,1 ,0
};

static UChar zeroBitCntInOneByte[256] =
{
    8 ,7 ,7 ,6 ,7 ,6 ,6 ,5 ,7 ,6 ,6 ,5 ,6 ,5 ,5 ,4 ,
    7 ,6 ,6 ,5 ,6 ,5 ,5 ,4 ,6 ,5 ,5 ,4 ,5 ,4 ,4 ,3 ,
    7 ,6 ,6 ,5 ,6 ,5 ,5 ,4 ,6 ,5 ,5 ,4 ,5 ,4 ,4 ,3 ,
    6 ,5 ,5 ,4 ,5 ,4 ,4 ,3 ,5 ,4 ,4 ,3 ,4 ,3 ,3 ,2 ,
    7 ,6 ,6 ,5 ,6 ,5 ,5 ,4 ,6 ,5 ,5 ,4 ,5 ,4 ,4 ,3 ,
    6 ,5 ,5 ,4 ,5 ,4 ,4 ,3 ,5 ,4 ,4 ,3 ,4 ,3 ,3 ,2 ,
    6 ,5 ,5 ,4 ,5 ,4 ,4 ,3 ,5 ,4 ,4 ,3 ,4 ,3 ,3 ,2 ,
    5 ,4 ,4 ,3 ,4 ,3 ,3 ,2 ,4 ,3 ,3 ,2 ,3 ,2 ,2 ,1 ,
    7 ,6 ,6 ,5 ,6 ,5 ,5 ,4 ,6 ,5 ,5 ,4 ,5 ,4 ,4 ,3 ,
    6 ,5 ,5 ,4 ,5 ,4 ,4 ,3 ,5 ,4 ,4 ,3 ,4 ,3 ,3 ,2 ,
    6 ,5 ,5 ,4 ,5 ,4 ,4 ,3 ,5 ,4 ,4 ,3 ,4 ,3 ,3 ,2 ,
    5 ,4 ,4 ,3 ,4 ,3 ,3 ,2 ,4 ,3 ,3 ,2 ,3 ,2 ,2 ,1 ,
    6 ,5 ,5 ,4 ,5 ,4 ,4 ,3 ,5 ,4 ,4 ,3 ,4 ,3 ,3 ,2 ,
    5 ,4 ,4 ,3 ,4 ,3 ,3 ,2 ,4 ,3 ,3 ,2 ,3 ,2 ,2 ,1 ,
    5 ,4 ,4 ,3 ,4 ,3 ,3 ,2 ,4 ,3 ,3 ,2 ,3 ,2 ,2 ,1 ,
    4 ,3 ,3 ,2 ,3 ,2 ,2 ,1 ,3 ,2 ,2 ,1 ,2 ,1 ,1 ,0
};

/*
 * 0인 bit을 찾을때는 count값을 검사하지 않는다.
 */
#define SDPTB_RESULT_BIT_VAL_gZeroBitIdx( result, count )            (result)

/*
 * 1인 bit을 찾을때는 count값을 검사한다.
 * 어차피 1을 찾는 루틴은 그리 길지 않기 때문에 count를 검사해도 문제가 없다.
 */
#define SDPTB_RESULT_BIT_VAL_gBitIdx( result, count )                         \
               ( (result >= count) ?  SDPTB_BIT_NOT_FOUND : result )

/*
 * aAddr주소값으로부터 바이트 단위로 검색을 시작하여 compare_val과 틀린 값을
 * 만나면 array를 사용해서 그 비트의 인덱스값을 얻어낸다.
 * 주의: 한바이트에서만 검색을 하는것이 아니라 다음바이트까지 계속검색한다.
 */
#define SDPTB_BIT_FIND_PER_BYTES( aAddr,compare_val ,aBitIdx,                  \
                                  sIdx ,array ,aCount, aMask)                  \
   {                                                                           \
       if (  *(UChar*)aAddr != compare_val )                                   \
       {                                                                       \
           *aBitIdx =                                                          \
            SDPTB_RESULT_BIT_VAL_##array( sIdx + array[*(UChar*)aAddr & aMask],\
                                          aCount);                             \
            IDE_RAISE( return_anyway );                                        \
       }                                                                       \
       else                                                                    \
       {                                                                       \
           (*(UChar**)&aAddr)++;                                               \
           sIdx += SDPTB_BITS_PER_BYTE;                                        \
           if( sIdx >= aCount )                                                \
           {                                                                   \
                *aBitIdx = SDPTB_BIT_NOT_FOUND;                                \
                IDE_RAISE( return_anyway );                                    \
           }                                                                   \
       }                                                                       \
   }

/***********************************************************************
 * Description :
 *  aAddr주소값으로부터 aCount개의 비트를 대상으로 1인 비트를 찾아서
 *  aBitIdx에 그 인덱스번호를 넘겨준다.  aHint에 주어진 비트 인덱스번호를
 *  시작지점으로 aAddr로부터 해당비트를 검색한다.
 *
 * aAddr            - [IN] 검색을 시작할 주소값
 * aCount           - [IN] 검색할 비트갯수
 * aHint            - [IN] 검색에대한 힌트(검색을 시작할 "비트의 index번호")
 * aBitIdx          - [OUT] 검색된 비트의 index번호
 *
 ***********************************************************************/
void sdptbBit::findBitFromHint( void *  aAddr,
                                UInt    aCount,
                                UInt    aHint,
                                UInt *  aBitIdx )
{
    UInt    sIdx=0;
    UInt    sLoop;
    UInt    sTemp;
    UChar   sMask = 0; //힌트이후의 값을 얻기위한 용도로 사용된다.
    UInt    sHintInBytes; //바이트단위의 힌트값을 유지한다.

    IDE_ASSERT( aHint < aCount );

    IDL_MEM_BARRIER;  // BUG-47666

    sHintInBytes = aHint / SDPTB_BITS_PER_BYTE;

    if( aHint >= SDPTB_BITS_PER_BYTE )
    {
        sIdx = sHintInBytes * SDPTB_BITS_PER_BYTE;
        *(UChar**)&aAddr += sHintInBytes;
    }

    sMask = 0xFF << (aHint % SDPTB_BITS_PER_BYTE);

    /* 
     * BUG-22182  bitmap tbs에서 파일에 free공간이 있는데
     *            찾지 못하는 경우가 있습니다.
     */
    // best case
    if( ((*(UChar*)aAddr & sMask) != 0x00)   ||
        (((aHint % SDPTB_BITS_PER_BYTE) + (aCount - aHint)) 
                                  <= SDPTB_BITS_PER_BYTE) )
    {
        if( (sIdx >> 3) == sHintInBytes) //힌트가 포함된 바이트라면
        {
            *aBitIdx = SDPTB_RESULT_BIT_VAL_gBitIdx( 
                                    sIdx + gBitIdx[(*(UChar*)aAddr) & sMask],
                                    aCount );
        }
        else
        {
            *aBitIdx = SDPTB_RESULT_BIT_VAL_gBitIdx( 
                                    sIdx + gBitIdx[(*(UChar*)aAddr) & 0xFF],
                                    aCount );
        }
        IDE_CONT( return_anyway );
    }

    //best case가 아니라면 지금 보고있는 바이트는 건너뛰어야만 한다.
    (*(UChar**)&aAddr)++;
    sIdx += SDPTB_BITS_PER_BYTE;

    /*
     * 8바이트로 정렬되지 않았다면 정렬될때까지 바이트 단위로 읽는다.
     */
    sTemp = (UInt)((vULong)aAddr % SDPTB_BITS_PER_BYTE);
    if( sTemp != 0 )
    {
        sLoop = SDPTB_BITS_PER_BYTE - sTemp;

        while( sLoop-- ) 
        {
           /* 
            * BUG-22182 bitmap tbs에서 파일에 free공간이 있는데 찾지 못하는
            *           경우가 있습니다.
            */
            if( (sIdx >> 3) == sHintInBytes ) //힌트가 포함된 바이트라면
            {                                 
                SDPTB_BIT_FIND_PER_BYTES( aAddr,
                                          0x00,
                                          aBitIdx,
                                          sIdx,
                                          gBitIdx, 
                                          aCount,
                                          sMask );
            }
            else
            {
                SDPTB_BIT_FIND_PER_BYTES( aAddr,
                                          0x00,
                                          aBitIdx,
                                          sIdx,
                                          gBitIdx, 
                                          aCount,
                                          0xFF );
            }
        }

    }

    while ( *(ULong*)aAddr == 0 )
    {
       (*(ULong**)&aAddr)++;
       sIdx +=  SDPTB_BITS_PER_ULONG;
    }

    while( 1 ) 
    {
           /* 
            * BUG-22182 bitmap tbs에서 파일에 free공간이 있는데 찾지 못하는
            *           경우가 있습니다.
            */
            if( (sIdx >> 3) == sHintInBytes ) //힌트가 포함된 바이트라면
            {
                SDPTB_BIT_FIND_PER_BYTES( aAddr,
                                          0x00,
                                          aBitIdx,
                                          sIdx,
                                          gBitIdx, 
                                          aCount,
                                          sMask );
            }
            else
            {
                SDPTB_BIT_FIND_PER_BYTES( aAddr,
                                          0x00,
                                          aBitIdx,
                                          sIdx,
                                          gBitIdx, 
                                          aCount,
                                          0xFF );
            }
    }
    
    IDE_EXCEPTION_CONT( return_anyway );

    /*
     * BUG-22363   IDE_ASSERT(sIdx <= aCache->mMaxGGID)[sdptbExtent.cpp:432], 
     *             errno=[16] 로 서버 비정상종료
     *
     * 위에서 체크를 해서 교정하지만 다시한번 확실하게 더블체크한다.
     */
    if( (*aBitIdx != SDPTB_BIT_NOT_FOUND) &&
		(*aBitIdx >= aCount) )
    {
       *aBitIdx = SDPTB_BIT_NOT_FOUND;

    }
    return ; 
}


/***********************************************************************
 * Description :
 *  aAddr주소값으로부터 aCount개의 비트를 대상으로 1인 비트를 찾아서
 *  aBitIdx에 그 인덱스번호를 넘겨준다.
 *
 * aAddr            - [IN] 검색을 시작할 주소값
 * aCount           - [IN] 검색할 비트갯수
 * aBitIdx          - [OUT] 검색된 비트의 index번호
 *
 ***********************************************************************/
void sdptbBit::findBit( void * aAddr,
                        UInt   aCount,
                        UInt * aBitIdx )
{
    findBitFromHint( aAddr,
                     aCount,
                     0,   // 시작지점의 hint,즉 첫비트부터검색한다.
                     aBitIdx);
}


/***********************************************************************
 * Description :
 *  aAddr주소값으로부터 aCount개의 비트를 대상으로 0인 비트를 찾아서
 *  aBitIdx에 그 인덱스번호를 넘겨준다.  aHint에 주어진 비트 인덱스번호를
 *  시작지점으로 aAddr로부터 해당비트를 검색한다.
 *
 * aAddr            - [IN] 검색을 시작할 주소값
 * aCount           - [IN] 검색할 비트갯수
 * aHint            - [IN] 검색에대한 힌트(검색을 시작할 "비트의 index번호")
 * aBitIdx          - [OUT] 검색된 비트의 index번호
 ***********************************************************************/
void sdptbBit::findZeroBitFromHint( void *  aAddr,
                                    UInt    aCount,
                                    UInt    aHint,
                                    UInt *  aBitIdx )
{
    UInt    sIdx=0;
    UInt    sLoop;
    UInt    sTemp;
    UInt    sNBytes;
    
    IDE_ASSERT( aAddr != NULL );
    IDE_ASSERT( aBitIdx != NULL );
    IDE_ASSERT( aHint < aCount );
    
    if( aHint >= SDPTB_BITS_PER_BYTE )
    {
        sNBytes = aHint / SDPTB_BITS_PER_BYTE;
        sIdx = sNBytes*SDPTB_BITS_PER_BYTE;

        *(UChar**)&aAddr += sNBytes;
    }

    // best case
    if( *(UChar*)aAddr != 0xFF )
    {
        *aBitIdx = 
          SDPTB_RESULT_BIT_VAL_gZeroBitIdx( sIdx + gZeroBitIdx[*(UChar*)aAddr],
                                            aCount);
        IDE_CONT( return_anyway );
    }

    /*
     * 8바이트로 정렬되지 않았다면 정렬될때까지 바이트 단위로 읽는다.
     */
    sTemp = (UInt)((vULong)aAddr % SDPTB_BITS_PER_BYTE);
    if( sTemp != 0 )
    {
        sLoop = SDPTB_BITS_PER_BYTE - sTemp;

        while( sLoop-- ) 
        {
            SDPTB_BIT_FIND_PER_BYTES( aAddr,
                                      0xFF,
                                      aBitIdx,
                                      sIdx,
                                      gZeroBitIdx, 
                                      aCount,
                                      0xFF);   //space holder 
        }

    }
    IDE_ASSERT( ((vULong)aAddr % SDPTB_BITS_PER_BYTE) == 0 );

    while( *(ULong*)aAddr == ID_ULONG_MAX ) 
    {
       (*(ULong**)&aAddr)++;
       sIdx +=  SDPTB_BITS_PER_ULONG;
    }

    while( 1 ) 
    {
       SDPTB_BIT_FIND_PER_BYTES( aAddr,
                                 0xFF,
                                 aBitIdx,
                                 sIdx,
                                 gZeroBitIdx, 
                                 aCount,
                                 0xFF); //space holder
    }
    
    IDE_EXCEPTION_CONT( return_anyway );

    return ; 
}

#if 0 //not used
/***********************************************************************
 * Description :
 *  aAddr주소값으로부터 aCount개의 비트를 대상으로 0인 비트를 찾아서
 *  aBitIdx에 그 인덱스번호를 넘겨준다.
 *
 * aAddr            - [IN] 검색을 시작할 주소값
 * aCount           - [IN] 검색할 비트갯수
 * aBitIdx          - [OUT] 검색된 비트의 index번호
 *
 ***********************************************************************/
void sdptbBit::findZeroBit( void * aAddr,
                            UInt   aCount,
                            UInt * aBitIdx )
{
    findZeroBitFromHint( aAddr,
                         aCount,
                         0,   // 시작지점의 hint,즉 첫비트부터검색한다.
                         aBitIdx );
}
#endif

/***********************************************************************
 * Description :
 *  aAddr주소값으로부터 aCount개의 비트를 대상으로 1인 비트를 찾아서
 *  aBitIdx에 그 인덱스번호를 넘겨준다.  aHint에 주어진 비트 인덱스번호를
 *  시작지점으로 aAddr로부터 해당비트를 검색한다.
 *  aCount의 비트까지 검색했는데 찾지 못햇다면 다시 처음부터 검색한다.(rotate)
 *
 * aAddr            - [IN] 검색을 시작할 주소값
 * aCount           - [IN] 검색할 비트갯수
 * aHint            - [IN] 검색에대한 힌트(검색을 시작할 "비트의 index번호")
 * aBitIdx          - [OUT] 검색된 비트의 index번호
 *
 ***********************************************************************/
void sdptbBit::findBitFromHintRotate( void *  aAddr,
                                      UInt    aCount,
                                      UInt    aHint,
                                      UInt *  aBitIdx )
{
    void * sAddr = aAddr;
    findBitFromHint( aAddr,
                     aCount, 
                     aHint,
                     aBitIdx );

    if ( (*aBitIdx == SDPTB_BIT_NOT_FOUND) && (aHint > 0) )
    {
        aAddr = sAddr;
        findBitFromHint( aAddr,
                         aHint,
                         0,           //첫비트부터 검색
                         aBitIdx );
    }
    else
    {
        /* nothing  to do */
    }
}

/***********************************************************************
 * Description :
 *  aAddr주소값으로부터 aCount개의 비트를 대상으로 1인 비트갯수의 합을 구한다.
 *
 * aAddr            - [IN] 시작 주소값
 * aCount           - [IN] 대상이되는 비트갯수
 * aRet             - [OUT] 1인 비트의 갯수의 합
 *
 ***********************************************************************/
void sdptbBit::sumOfZeroBit( void * aAddr,
                             UInt   aCount,
                             UInt * aRet )
{
    UInt    sSum = 0 ;
    UChar * sP = (UChar *)aAddr;
    UChar   sTemp = 0x01;
    UInt    sLoop = aCount / SDPTB_BITS_PER_BYTE;
    UInt    sRest = aCount % SDPTB_BITS_PER_BYTE;

    while( sLoop-- )
    {
        sSum += zeroBitCntInOneByte[ *sP++ ];
    }

    while( sRest-- )
    {
        if( ( *sP & sTemp ) == 0 ) // 0이 세트된 비트 발견!
        {
            sSum++;
        }

        sTemp <<= 1;
    }

    *aRet = sSum;
}



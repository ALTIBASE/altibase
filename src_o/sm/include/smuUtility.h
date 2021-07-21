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
 * $Id: smuUtility.h 85333 2019-04-26 02:34:41Z et16 $
 **********************************************************************/

#ifndef _O_SMU_UTILITY_H_
#define _O_SMU_UTILITY_H_ 1

#include <idl.h>
#include <ide.h>

#include <smDef.h>

/* ------------------------------------------------
 * for A4
 * checksum 계산을 위한 random-mask
 * ----------------------------------------------*/
#define SMU_CHECKSUM_RANDOM_MASK1    (1463735687)
#define SMU_CHECKSUM_RANDOM_MASK2    (1653893711)

typedef IDE_RC (*smuWriteErrFunc)(UInt         aChkFlag,
                                  ideLogModule aModule,
                                  UInt         aLevel,
                                  const SChar *aFormat,
                                  ... );

typedef void (*smuDumpFunc)( void  * aTarget,
                             SChar * aOutBuf, 
                             UInt    aOutSize );

// PROJ-2118 BUG Reporting
typedef struct smuAlignBuf
{
    UChar * mAlignPtr;
    void  * mFreePtr;
} smuAlignBuf;

class smuUtility
{
public:
    static IDE_RC loadErrorMsb(SChar *root_dir, SChar *idn);
    static IDE_RC outputUtilityHeader(const SChar *aUtilName);
    
    static SInt outputMsg(const SChar *aFmt, ...);
    static SInt outputErr(const SChar *aFmt, ...);

    /* 데이타 베이스 name creation */

//      static IDE_RC makeDatabaseFileName(SChar  *aDBName,
//                                         SChar **aDBDir);
    
//      static IDE_RC makeDBDirForCreate(SChar  *aDBName,
//                                       SChar **aDBDir );
    
    //  static IDE_RC getDBDir( SChar **aDBDir);
    

    /* ------------------------------------------------
     * for A4
     * checksum 계산을 위한 접기 함수들..
     * ----------------------------------------------*/
    static inline UInt foldBinary(UChar* aBuffer,
                                  UInt   aLength); 
    
    static inline UInt foldUIntPair(UInt aNum1, 
                                    UInt aNum2);

    // 문자가 알파벳이나 숫자인 경우 true 반환
    static inline idBool isAlNum( SChar aChar );
    // 문자가 숫자인 경우 true를 반환
    static inline idBool isDigit( SChar aChar );
    // 문자열이 모두 숫자로 이루어져 있는지 판단하는 함수
    static inline idBool isDigitForString( SChar * aChar, UInt aLength );

    /* 크거나 같은 2^n 값을 반환 */
    static inline UInt getPowerofTwo( UInt aValue );

    static inline IDE_RC allocAlignedBuf( iduMemoryClientIndex aAllocIndex,
                                          UInt                 aAllocSize,
                                          UInt                 aAlignSize,
                                          smuAlignBuf*         aAllocPtr );

    static inline IDE_RC freeAlignedBuf( smuAlignBuf* aAllocPtr );

    /*************************************************************************
     * PROJ-2201 Innovation in sorting and hashing(temp)
     *************************************************************************/
    static void getTimeString( UInt    aTimeValue, 
                               UInt    aBufferLength,
                               SChar * aBuffer );

    /* Dump용 함수 */
    static void dumpFuncWithBuffer( UInt           aChkFlag, 
                                    ideLogModule   aModule, 
                                    UInt           aLevel, 
                                    smuDumpFunc    aDumpFunc,
                                    void         * aTarget);
    static void printFuncWithBuffer( smuDumpFunc    aDumpFunc,
                                     void         * aTarget);

    /* 공용 변수들에 대한 DumpFunction */
    static void dumpGRID( void  * aTarget,
                          SChar * aOutBuf, 
                          UInt    aOutSize );
    static void dumpUInt( void  * aTarget,
                          SChar * aOutBuf, 
                          UInt    aOutSize );
    static void dumpExtDesc( void  * aTarget,
                             SChar * aOutBuf, 
                             UInt    aOutSize );
public:
    // unit test를 위한 변수
    // verify는 ideLog::log을 사용하나
    // unit은 idlOS::printf로 보여주기 위해 unit에서만 이를
    // 다른 함수로 대치하여 사용한다.
    static smuWriteErrFunc mWriteError;
};

/* ------------------------------------------------
 * 바이너리 스트링 조합 
 * ----------------------------------------------*/
inline UInt smuUtility::foldBinary(UChar* aBuffer, 
                                   UInt   aLength)
{

    UInt i;
    UInt sFold;
    
    IDE_DASSERT(aBuffer != NULL);
    IDE_DASSERT(aLength != 0);

    for(i = 0, sFold = 0; i < aLength; i++, aBuffer++)
    {
        sFold = smuUtility::foldUIntPair(sFold,
                                         (UInt)(*aBuffer));
    }

    return (sFold);
    
}

/* ------------------------------------------------
 * 두개의 UInt 값 조합
 * ----------------------------------------------*/
inline UInt smuUtility::foldUIntPair(UInt aNum1,
                                     UInt aNum2)
{

    return (((((aNum1 ^ aNum2 ^ SMU_CHECKSUM_RANDOM_MASK2) << 8) + aNum1)
             ^ SMU_CHECKSUM_RANDOM_MASK1) + aNum2);
    
}

/***********************************************************************
 * Description : 영문자와 숫자를 판단하는
 * alphavet 이나 digit이면 true 그렇지않으면 false
 **********************************************************************/
inline idBool smuUtility::isAlNum( SChar aChar )
{
    if ( ( ( 'a' <= aChar ) && ( aChar <= 'z' ) ) ||
         ( ( 'A' <= aChar ) && ( aChar <= 'Z' ) ) ||
         ( isDigit( aChar ) == ID_TRUE ) )
    {
        return ID_TRUE;
    }

    return ID_FALSE;
}

/***********************************************************************
 * Description : 숫자를 판단하는 함수 
 * digit이면 true 그렇지않으면 false
 **********************************************************************/
inline idBool smuUtility::isDigit( SChar aChar )
{
    if ( ( '0' <= aChar ) && ( aChar <= '9' ) )
    {
        return ID_TRUE;
    }
    return ID_FALSE;

}

/***********************************************************************
 * Description : 문자열이 모두 숫자로 이루어져 있는지 판단하는 함수
 * digit이면 true 그렇지않으면 false
 **********************************************************************/
inline idBool smuUtility::isDigitForString( SChar * aChar, UInt aLength )
{
    UInt i;

    IDE_ASSERT( aChar != NULL );
    
    for( i = 0; i < aLength; i++ )
    {
        if( isDigit( *( aChar + i ) ) == ID_FALSE )
        {
            return ID_FALSE;
        }
    }

    return ID_TRUE;
}

/***********************************************************************
 * Description : UInt 값을 입력받아 크거나 같은 2^n 값을 반환
 **********************************************************************/
inline UInt smuUtility::getPowerofTwo( UInt aValue )
{
    SInt i;
    UInt sResult    = 0x00000001;
    UInt sInput     = aValue ;
 
    for ( i = 0; i < 32; i++ )
    {
        aValue  >>= 1;
        sResult <<= 1;
 
        if ( (aValue == 0) || (sInput == sResult) )
        {
            break;
        }
        else
        {
            /* do nothing */
        }
    }

    return sResult;
}

/* --------------------------------------------------------------------
 * PROJ-2118 BUG Reporting
 *
 * Description : 주어진 align에 맞추어 memory를 alloc 한다.
 *
 * ----------------------------------------------------------------- */
IDE_RC smuUtility::allocAlignedBuf( iduMemoryClientIndex aAllocIndex,
                                    UInt                 aAllocSize,
                                    UInt                 aAlignSize,
                                    smuAlignBuf*         aAllocPtr )
{
    void  * sAllocPtr;
    UInt    sState  = 0;

    IDE_ERROR( aAllocPtr != NULL );

    aAllocPtr->mAlignPtr = NULL;
    aAllocPtr->mFreePtr  = NULL;

    IDE_TEST( iduMemMgr::calloc( aAllocIndex, 1,
                                 aAlignSize + aAllocSize,
                                 &sAllocPtr )
             != IDE_SUCCESS );
    sState = 1;

    aAllocPtr->mFreePtr  = sAllocPtr;
    aAllocPtr->mAlignPtr = (UChar*)idlOS::align( (UChar*)sAllocPtr,
                                                 aAlignSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sAllocPtr ) == IDE_SUCCESS );
            sAllocPtr = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

/* --------------------------------------------------------------------
 * PROJ-2118 BUG Reporting
 *
 * Description : allocAlignedBuf 에서 Alloc 한 메모리를 해제한다.
 *
 * ----------------------------------------------------------------- */
IDE_RC smuUtility::freeAlignedBuf( smuAlignBuf* aAllocPtr )
{
    IDE_ERROR( aAllocPtr != NULL );

    (void)iduMemMgr::free( aAllocPtr->mFreePtr );

    aAllocPtr->mAlignPtr = NULL;
    aAllocPtr->mFreePtr  = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#endif  // _O_SMU_UTILITY_H_
    



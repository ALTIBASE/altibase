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
 * $Id: mtc.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtl.h>
#include <mtv.h>
#include <mtk.h>
#include <mtz.h>
#if defined (CYGWIN32)
# include <pthread.h>
#endif

#include <idErrorCode.h>
#include <mtuProperty.h>
#include <iduCheckLicense.h>

extern mtdModule mtdDouble;
extern mtdModule mtdNumeric;
extern mtdModule mtdFloat;
extern mtlModule mtlAscii;
extern mtlModule mtlUTF16;

/* PROJ-1517 */
SDouble gDoublePow[64];

/********************************************************
 * BUG-41194 double to numeric 변환 성능개선
 * 2진수를 10진수로 변경시 사용할 변환테이블을 정의한다.
 *
 * 2^-1075 = 0.247033 E-323
 * 2^-1074 = 0.494066 E-323
 * ...
 * 2^-1    = 0.500000 E0
 * 2^0     = 0.100000 E1
 * 2^1     = 0.200000 E1
 * 2^2     = 0.400000 E1
 * ...
 * 2^1074  = 0.202402 E324
 * 2^1075  = 0.404805 E324
 *           ~~~~~~~~  ~~~
 *           실수부   지수부
 *
 ********************************************************/
SDouble   gConvMantissaBuf[1075+1+1075];  // 실수부
SInt      gConvExponentBuf[1075+1+1075];  // 지수부
SDouble * gConvMantissa;
SInt    * gConvExponent;
idBool    gIEEE754Double;

const UChar mtc::hashPermut[256] = {
    1, 87, 49, 12,176,178,102,166,121,193,  6, 84,249,230, 44,163,
    14,197,213,181,161, 85,218, 80, 64,239, 24,226,236,142, 38,200,
    110,177,104,103,141,253,255, 50, 77,101, 81, 18, 45, 96, 31,222,
    25,107,190, 70, 86,237,240, 34, 72,242, 20,214,244,227,149,235,
    97,234, 57, 22, 60,250, 82,175,208,  5,127,199,111, 62,135,248,
    174,169,211, 58, 66,154,106,195,245,171, 17,187,182,179,  0,243,
    132, 56,148, 75,128,133,158,100,130,126, 91, 13,153,246,216,219,
    119, 68,223, 78, 83, 88,201, 99,122, 11, 92, 32,136,114, 52, 10,
    138, 30, 48,183,156, 35, 61, 26,143, 74,251, 94,129,162, 63,152,
    170,  7,115,167,241,206,  3,150, 55, 59,151,220, 90, 53, 23,131,
    125,173, 15,238, 79, 95, 89, 16,105,137,225,224,217,160, 37,123,
    118, 73,  2,157, 46,116,  9,145,134,228,207,212,202,215, 69,229,
    27,188, 67,124,168,252, 42,  4, 29,108, 21,247, 19,205, 39,203,
    233, 40,186,147,198,192,155, 33,164,191, 98,204,165,180,117, 76,
    140, 36,210,172, 41, 54,159,  8,185,232,113,196,231, 47,146,120,
    51, 65, 28,144,254,221, 93,189,194,139,112, 43, 71,109,184,209
};

const UInt mtc::hashInitialValue = 0x01020304;

mtcGetColumnFunc            mtc::getColumn;
mtcOpenLobCursorWithRow     mtc::openLobCursorWithRow;
mtcOpenLobCursorWithGRID    mtc::openLobCursorWithGRID;
mtcReadLob                  mtc::readLob;
mtcGetLobLengthWithLocator  mtc::getLobLengthWithLocator;
mtcIsNullLobColumnWithRow   mtc::isNullLobColumnWithRow;
mtcGetCompressionColumnFunc mtc::getCompressionColumn;
/* PROJ-2446 ONE SOURCE */
mtcGetStatistics            mtc::getStatistics;

// PROJ-1579 NCHAR
mtcGetDBCharSet            mtc::getDBCharSet;
mtcGetNationalCharSet      mtc::getNationalCharSet;

static IDE_RC             gMtcResult      = IDE_SUCCESS;
static PDL_thread_mutex_t gMtcMutex;

UInt mtc::mExtTypeModuleGroupCnt = 0;
UInt mtc::mExtCvtModuleGroupCnt = 0;
UInt mtc::mExtFuncModuleGroupCnt = 0;
UInt mtc::mExtRangeFuncGroupCnt = 0;
UInt mtc::mExtCompareFuncCnt = 0;

mtdModule ** mtc::mExtTypeModuleGroup[MTC_MAXIMUM_EXTERNAL_GROUP_CNT];
mtvModule ** mtc::mExtCvtModuleGroup[MTC_MAXIMUM_EXTERNAL_GROUP_CNT];
mtfModule ** mtc::mExtFuncModuleGroup[MTC_MAXIMUM_EXTERNAL_GROUP_CNT];
smiCallBackFunc * mtc::mExtRangeFuncGroup[MTC_MAXIMUM_EXTERNAL_GROUP_CNT];
mtdCompareFunc  * mtc::mExtCompareFuncGroup[MTC_MAXIMUM_EXTERNAL_GROUP_CNT];

/* BUG-39237 add sys_guid() function */
ULong     gInitialValue;
SChar     gHostID[MTF_SYS_HOSTID_LENGTH + 1];
UInt      gTimeSec;

extern "C" void mtcInitializeMutex( void )
{
    if( idlOS::thread_mutex_init( &gMtcMutex ) != 0 )
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrMutexInit));
        gMtcResult = IDE_FAILURE;
    }
}

const void* mtc::getColumnDefault( const void*,
                                   const smiColumn*,
                                   UInt* )
{
    return NULL;
}

IDE_RC
mtc::addExtTypeModule( mtdModule **  aExtTypeModules )
{
    IDE_ASSERT( aExtTypeModules != NULL );
    IDE_ASSERT( mExtTypeModuleGroupCnt < MTC_MAXIMUM_EXTERNAL_GROUP_CNT );

    mExtTypeModuleGroup[mExtTypeModuleGroupCnt] = aExtTypeModules;
    mExtTypeModuleGroupCnt++;

    return IDE_SUCCESS;
}

IDE_RC
mtc::addExtCvtModule( mtvModule **  aExtCvtModules )
{
    IDE_ASSERT( aExtCvtModules != NULL );
    IDE_ASSERT( mExtCvtModuleGroupCnt < MTC_MAXIMUM_EXTERNAL_GROUP_CNT );

    mExtCvtModuleGroup[mExtCvtModuleGroupCnt] = aExtCvtModules;
    mExtCvtModuleGroupCnt++;

    return IDE_SUCCESS;
}

IDE_RC
mtc::addExtFuncModule( mtfModule **  aExtFuncModules )
{
    IDE_ASSERT( aExtFuncModules != NULL );
    IDE_ASSERT( mExtFuncModuleGroupCnt < MTC_MAXIMUM_EXTERNAL_GROUP_CNT );

    mExtFuncModuleGroup[mExtFuncModuleGroupCnt] = aExtFuncModules;
    mExtFuncModuleGroupCnt++;

    return IDE_SUCCESS;
}

IDE_RC
mtc::addExtRangeFunc( smiCallBackFunc  * aExtRangeFuncs )
{
    IDE_ASSERT( aExtRangeFuncs != NULL );
    IDE_ASSERT( mExtRangeFuncGroupCnt < MTC_MAXIMUM_EXTERNAL_GROUP_CNT );

    mExtRangeFuncGroup[mExtRangeFuncGroupCnt] = aExtRangeFuncs;
    mExtRangeFuncGroupCnt++;

    return IDE_SUCCESS;
}

IDE_RC
mtc::addExtCompareFunc( mtdCompareFunc * aExtCompareFuncs )
{
    IDE_ASSERT( aExtCompareFuncs != NULL );
    IDE_ASSERT( mExtCompareFuncCnt < MTC_MAXIMUM_EXTERNAL_GROUP_CNT );

    mExtCompareFuncGroup[mExtCompareFuncCnt] = aExtCompareFuncs;
    mExtCompareFuncCnt++;

    return IDE_SUCCESS;
}

IDE_RC mtc::initialize( mtcExtCallback  * aCallBacks )
{
    SDouble   sDoubleValue = 1.0;
    SDouble   sDoubleProbe = -100.0;
    UChar   * sDoubleProbes = (UChar*) &sDoubleProbe;
    SChar     sHostString[IDU_SYSTEM_INFO_LENGTH];
    UInt      i;
    SInt      j;

    /* BUG-40387 trace log의 양을 줄이기 위해
     * mt초기화시 발생하는 에러는 출력하지 않는다. */
    
    if( aCallBacks == NULL )
    {
        getColumn               = getColumnDefault;
        openLobCursorWithRow    = NULL;
        openLobCursorWithGRID   = NULL;
        readLob                 = NULL;
        getLobLengthWithLocator = NULL;
        isNullLobColumnWithRow  = NULL;

        getDBCharSet            = NULL;
        getNationalCharSet      = NULL;

        getCompressionColumn    = NULL;
        /* PROJ-2336 ONE SOURCE */
        getStatistics           = NULL;
    }
    else
    {
        getColumn               = aCallBacks->getColumn;
        openLobCursorWithRow    = aCallBacks->openLobCursorWithRow;
        openLobCursorWithGRID   = aCallBacks->openLobCursorWithGRID;
        readLob                 = aCallBacks->readLob;
        getLobLengthWithLocator = aCallBacks->getLobLengthWithLocator;
        isNullLobColumnWithRow  = aCallBacks->isNullLobColumnWithRow;

        getDBCharSet            = aCallBacks->getDBCharSet;
        getNationalCharSet      = aCallBacks->getNationalCharSet;

        getCompressionColumn    = aCallBacks->getCompressionColumn;
        /* PROJ-2336 ONE SOURCE */
        getStatistics           = aCallBacks->getStatistics;
    }

    IDE_TEST( mtuProperty::initProperty( NULL ) != IDE_SUCCESS );

    /* PROJ-1517 */
    for ( i = 0; i < ( ID_SIZEOF(gDoublePow) / ID_SIZEOF(gDoublePow[0]) ); i++)
    {
        gDoublePow[i] = sDoubleValue;
        sDoubleValue  = sDoubleValue * 100.0;
    }

    // BUG-41194
    // IEEE754에서 64bit double로 -100은 다음과 같이 표현한다.
    // 11000000 01011001 00000000 00000000 00000000 00000000 00000000 00000000
    // C0       59       00       00       00       00       00       00
#if defined(ENDIAN_IS_BIG_ENDIAN)
    if ( ( sDoubleProbes[0] == 0xC0 ) && ( sDoubleProbes[1] == 0x59 ) &&
         ( sDoubleProbes[2] == 0x00 ) && ( sDoubleProbes[3] == 0x00 ) &&
         ( sDoubleProbes[4] == 0x00 ) && ( sDoubleProbes[5] == 0x00 ) &&
         ( sDoubleProbes[6] == 0x00 ) && ( sDoubleProbes[7] == 0x00 ) )
#else
    if ( ( sDoubleProbes[7] == 0xC0 ) && ( sDoubleProbes[6] == 0x59 ) &&
         ( sDoubleProbes[5] == 0x00 ) && ( sDoubleProbes[4] == 0x00 ) &&
         ( sDoubleProbes[3] == 0x00 ) && ( sDoubleProbes[2] == 0x00 ) &&
         ( sDoubleProbes[1] == 0x00 ) && ( sDoubleProbes[0] == 0x00 ) )
#endif
    {
        // IEEE754형식의 double이다.
        gIEEE754Double = ID_TRUE;
        
        // 2진수 10진수 변환테이블을 생성한다.
        gConvMantissa = gConvMantissaBuf + 1075;
        gConvExponent = gConvExponentBuf + 1075;
    
        gConvMantissa[0] = 0.1;
        gConvExponent[0] = 1;
        for ( j = 1; j <= 1075; j++ )
        {
            gConvMantissa[j] = gConvMantissa[j - 1] * 2.0;
            gConvExponent[j] = gConvExponent[j - 1];
            if ( gConvMantissa[j] > 1 )
            {
                gConvMantissa[j] /= 10.0;
                gConvExponent[j]++;
            }
            else
            {
                // Nothing to do.
            }

            gConvMantissa[-j] = gConvMantissa[-j + 1] / 2.0;
            gConvExponent[-j] = gConvExponent[-j + 1];
            if ( gConvMantissa[-j] < 0.1 )
            {
                gConvMantissa[-j] *= 10.0;
                gConvExponent[-j]--;
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // IEEE754형식의 double이 아니다.
        gIEEE754Double = ID_FALSE;
    }
  
    /* BUG-39237 adding sys_guid() function that returning byte(16) type. */ 
    idlOS::srand(idlOS::time());

    gInitialValue = ( (ULong) idlOS::rand() << 32 ) + (ULong) idlOS::rand() ;
    gTimeSec      = (UInt) idlOS::gettimeofday().sec();

    // MT 모듈 초기화 시에는 프로퍼티로 초기화 하지만
    // 추후 클라이언트 연결 시에는 해당 클라이언트의 ALTIBASE_NLS_USE로
    // 다시 defModule을 구성하게 된다.
    IDE_TEST( mtl::initialize( (SChar*)"US7ASCII", ID_FALSE )
              != IDE_SUCCESS );
    IDE_TEST( mtd::initialize( mExtTypeModuleGroup, mExtTypeModuleGroupCnt )
              != IDE_SUCCESS );
    IDE_TEST( mtv::initialize( mExtCvtModuleGroup, mExtCvtModuleGroupCnt )
              != IDE_SUCCESS );
    IDE_TEST( mtf::initialize( mExtFuncModuleGroup, mExtFuncModuleGroupCnt )
              != IDE_SUCCESS );
    IDE_TEST( mtk::initialize( mExtRangeFuncGroup, mExtRangeFuncGroupCnt,
                               mExtCompareFuncGroup, mExtCompareFuncCnt )
              != IDE_SUCCESS );
    IDE_TEST( mtz::initialize() != IDE_SUCCESS );

    IDE_TEST( iduCheckLicense::getHostUniqueString( sHostString, ID_SIZEOF(sHostString))
              != IDE_SUCCESS );

    if ( idlOS::strlen( sHostString ) < MTF_SYS_HOSTID_LENGTH )
    {
        idlOS::snprintf( gHostID, MTF_SYS_HOSTID_LENGTH + 1,
                         MTF_SYS_HOSTID_FORMAT""ID_XINT32_FMT,
                         idlOS::rand() );
    }
    else
    {
        idlOS::strncpy( gHostID, sHostString, MTF_SYS_HOSTID_LENGTH);
        idlOS::strUpper( gHostID, MTF_SYS_HOSTID_LENGTH );
        gHostID[MTF_SYS_HOSTID_LENGTH] = '\0';
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC mtc::finalize( idvSQL * aStatistics )
{
    IDE_TEST( mtf::finalize() != IDE_SUCCESS );
    IDE_TEST( mtv::finalize() != IDE_SUCCESS );
    IDE_TEST( mtd::finalize() != IDE_SUCCESS );
    IDE_TEST( mtl::finalize() != IDE_SUCCESS );
    IDE_TEST( mtz::finalize() != IDE_SUCCESS );
    IDE_TEST( mtuProperty::finalProperty( aStatistics )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt mtc::isSameType( const mtcColumn* aColumn1,
                      const mtcColumn* aColumn2 )
{
    UInt sRes;
    if( aColumn1->type.dataTypeId == aColumn2->type.dataTypeId  &&
        aColumn1->type.languageId == aColumn2->type.languageId  &&
        ( aColumn1->flag & MTC_COLUMN_ARGUMENT_COUNT_MASK ) ==
        ( aColumn2->flag & MTC_COLUMN_ARGUMENT_COUNT_MASK ) &&
        aColumn1->precision     == aColumn2->precision      &&
        aColumn1->scale         == aColumn2->scale           )
    {
        sRes = 1;
    }
    else
    {
        sRes = 0;
    }

    return sRes;
}

void mtc::copyColumn( mtcColumn*       aDestination,
                      const mtcColumn* aSource )
{
    *aDestination = *aSource;

    aDestination->column.flag &= ~SMI_COLUMN_TYPE_MASK;
    aDestination->column.flag |= SMI_COLUMN_TYPE_FIXED;

    // BUG-38494 Compressed Column
    aDestination->column.flag &= ~SMI_COLUMN_COMPRESSION_MASK;
    aDestination->column.flag |= SMI_COLUMN_COMPRESSION_FALSE;
}

/********************************************************************
 * Description
 *    이 함수는 상위 레이어를 위한 함수이다.
 *    Variable 컬럼에 대해 NULL이 반환되면
 *    이 함수는 mtdModule의 NULL 값으로 래핑한 후 반환한다.
 *    그리고 SM에서 callback으로 이 함수를 호출되서는 안된다.
 *    왜냐하면, aColumn의 module을 참조하므로 smiColumn을
 *    인자로 넘기면 안된다.
 *
 ********************************************************************/
const void* mtc::value( const mtcColumn* aColumn,
                        const void*      aRow,
                        UInt             aFlag )
{
    // Proj-1391 Removing variable NULL Project
    const void* sValue =
        mtd::valueForModule( (smiColumn*)aColumn, aRow, aFlag,
                             aColumn->module->staticNull );

    return sValue;
}

#ifdef ENDIAN_IS_BIG_ENDIAN
UInt mtc::hash( UInt         aHash,
                const UChar* aValue,
                UInt         aLength )
{
    UChar        sHash[4];
    const UChar* sFence;

    sFence = aValue + aLength;

    sHash[3] = aHash;
    aHash >>= 8;
    sHash[2] = aHash;
    aHash >>= 8;
    sHash[1] = aHash;
    aHash >>= 8;
    sHash[0] = aHash;

    for( ; aValue < sFence; aValue++ )
    {
        sHash[0] = hashPermut[sHash[0]^*aValue];
        sHash[1] = hashPermut[sHash[1]^*aValue];
        sHash[2] = hashPermut[sHash[2]^*aValue];
        sHash[3] = hashPermut[sHash[3]^*aValue];
    }

    return sHash[0]<<24|sHash[1]<<16|sHash[2]<<8|sHash[3];
}
#else
UInt mtc::hash(UInt         aHash,
               const UChar* aValue,
               UInt         aLength )
{
    UChar        sHash[4];
    const UChar* sFence;

    sFence = aValue + aLength;

    sHash[3] = aHash;
    aHash >>= 8;
    sHash[2] = aHash;
    aHash >>= 8;
    sHash[1] = aHash;
    aHash >>= 8;
    sHash[0] = aHash;
    sFence--;
    // little endian이기 때문에 value를 역순으로 한다.
    for(; sFence >= aValue; sFence--)
    {
        sHash[0] = hashPermut[sHash[0]^*sFence];
        sHash[1] = hashPermut[sHash[1]^*sFence];
        sHash[2] = hashPermut[sHash[2]^*sFence];
        sHash[3] = hashPermut[sHash[3]^*sFence];
    }
    return sHash[0]<<24|sHash[1]<<16|sHash[2]<<8|sHash[3];
}
#endif

// fix BUG-9496
// float, numeric data type에 대한 hash 함수
// (mtc::hash 함수는 endian에 따른 구분처리)
// mtc::hashWithExponent는 endian에 따른 구분 처리를 하지 않는다.
// float, numeric data type은 mtdNumericType 구조체를 사용하므로
// endian에 따른 구분 처리를 하지 않아도 됨.
UInt mtc::hashWithExponent( UInt         aHash,
                            const UChar* aValue,
                            UInt         aLength )
{
    UChar        sHash[4];
    const UChar* sFence;

    sFence = aValue + aLength;

    sHash[3] = aHash;
    aHash >>= 8;
    sHash[2] = aHash;
    aHash >>= 8;
    sHash[1] = aHash;
    aHash >>= 8;
    sHash[0] = aHash;

    for( ; aValue < sFence; aValue++ )
    {
        sHash[0] = hashPermut[sHash[0]^*aValue];
        sHash[1] = hashPermut[sHash[1]^*aValue];
        sHash[2] = hashPermut[sHash[2]^*aValue];
        sHash[3] = hashPermut[sHash[3]^*aValue];
    }

    return sHash[0]<<24|sHash[1]<<16|sHash[2]<<8|sHash[3];
}

UInt mtc::hashSkip( UInt         aHash,
                    const UChar* aValue,
                    UInt         aLength )
{
    UChar        sHash[4];
    UInt         sSkip;
    const UChar* sFence;
    const UChar* sSubFence;

    sSkip   = 2;
    sFence  = aValue + aLength;

    sHash[3] = aHash;
    aHash >>= 8;
    sHash[2] = aHash;
    aHash >>= 8;
    sHash[1] = aHash;
    aHash >>= 8;
    sHash[0] = aHash;

    while( aValue < sFence )
    {
        sSubFence = aValue + 2;
        if( sSubFence > sFence )
        {
            sSubFence = sFence;
        }
        for( ; aValue < sSubFence; aValue ++ )
        {
            sHash[0] = hashPermut[sHash[0]^*aValue];
            sHash[1] = hashPermut[sHash[1]^*aValue];
            sHash[2] = hashPermut[sHash[2]^*aValue];
            sHash[3] = hashPermut[sHash[3]^*aValue];
        }
        aValue += sSkip;
        sSkip <<= 1;
    }

    return (sHash[0]<<24|sHash[1]<<16|sHash[2]<<8|sHash[3]);
}

UInt mtc::hashSkipWithoutZero( UInt         aHash,
                               const UChar* aValue,
                               UInt         aLength )
{
    UChar        sHash[4];
    UInt         sSkip;
    const UChar* sFence;
    const UChar* sSubFence;

    sSkip   = 2;
    sFence  = aValue + aLength;

    sHash[3] = aHash;
    aHash >>= 8;
    sHash[2] = aHash;
    aHash >>= 8;
    sHash[1] = aHash;
    aHash >>= 8;
    sHash[0] = aHash;

    while ( aValue < sFence )
    {
        sSubFence = aValue + 2;
        if ( sSubFence > sFence )
        {
            sSubFence = sFence;
        }
        else
        {
            // Nothing to do.
        }
        for ( ; aValue < sSubFence; aValue++ )
        {
            if ( *aValue != 0x00 )
            {
                sHash[0] = hashPermut[sHash[0]^*aValue];
                sHash[1] = hashPermut[sHash[1]^*aValue];
                sHash[2] = hashPermut[sHash[2]^*aValue];
                sHash[3] = hashPermut[sHash[3]^*aValue];
            }
            else
            {
                // Nothing to do.
            }
        }
        aValue += sSkip;
        sSkip <<= 1;
    }

    return (sHash[0]<<24|sHash[1]<<16|sHash[2]<<8|sHash[3]);
}

UInt mtc::hashWithoutSpace( UInt         aHash,
                            const UChar* aValue,
                            UInt         aLength )
{
    // To Fix PR-11941
    // 숫자형 String인 경우 성능 저하가 매우 심각함.
    // 따라서, Space를 제외한 모든 String을 비교하도록 함.

    UChar        sHash[4];
    const UChar* sFence;

    sFence  = aValue + aLength;

    sHash[3] = aHash;
    aHash >>= 8;
    sHash[2] = aHash;
    aHash >>= 8;
    sHash[1] = aHash;
    aHash >>= 8;
    sHash[0] = aHash;

    for( ; aValue < sFence; aValue ++ )
    {
        if ( ( *aValue != ' ' ) && ( *aValue != 0x00 ) )
        {
            sHash[0] = hashPermut[sHash[0]^*aValue];
            sHash[1] = hashPermut[sHash[1]^*aValue];
            sHash[2] = hashPermut[sHash[2]^*aValue];
            sHash[3] = hashPermut[sHash[3]^*aValue];
        }
    }

    // To Fix PR-11941
    /*
      UChar        sHash[4];
      UInt         sSkip;
      const UChar* sFence;
      const UChar* sSubFence;

      sSkip   = 2;
      sFence  = aValue + aLength;

      sHash[3] = aHash;
      aHash >>= 8;
      sHash[2] = aHash;
      aHash >>= 8;
      sHash[1] = aHash;
      aHash >>= 8;
      sHash[0] = aHash;

      while( aValue < sFence )
      {
      sSubFence = aValue + 2;
      if( sSubFence > sFence )
      {
      sSubFence = sFence;
      }
      for( ; aValue < sSubFence; aValue ++ )
      {
      if( *aValue != ' ' )
      {
      sHash[0] = hashPermut[sHash[0]^*aValue];
      sHash[1] = hashPermut[sHash[1]^*aValue];
      sHash[2] = hashPermut[sHash[2]^*aValue];
      sHash[3] = hashPermut[sHash[3]^*aValue];
      }
      }
      aValue += sSkip;
      sSkip <<= 1;
      }
    */

    return (sHash[0]<<24|sHash[1]<<16|sHash[2]<<8|sHash[3]);
}

IDE_RC mtc::makeNumeric( mtdNumericType* aNumeric,
                         UInt            aMaximumMantissa,
                         const UChar*    aString,
                         UInt            aLength )
{
    const UChar* sMantissaStart;
    const UChar* sMantissaEnd;
    const UChar* sPointPosition;
    UInt         sOffset;
    SInt         sSign;
    SInt         sExponentSign;
    SInt         sExponent;
    SInt         sMantissaIterator;

    if( (aString == NULL) || (aLength == 0) )
    {
        aNumeric->length = 0;
    }
    else
    {
        sSign  = 1;
        for(sOffset = 0; sOffset < aLength; sOffset++ )
        {
            if( aString[sOffset] != ' ' && aString[sOffset] != '\t' )
            {
                break;
            }
        }
        IDE_TEST_RAISE( sOffset >= aLength, ERR_INVALID_LITERAL );
        switch( aString[sOffset] )
        {
            case '-':
                sSign = -1;
            case '+':
                sOffset++;
                IDE_TEST_RAISE( sOffset >= aLength, ERR_INVALID_LITERAL );
        }
        if( aString[sOffset] == '.' )
        {
            sMantissaStart = &(aString[sOffset]);
            sPointPosition = &(aString[sOffset]);
            sOffset++;
            IDE_TEST_RAISE( sOffset >= aLength, ERR_INVALID_LITERAL );
            IDE_TEST_RAISE( aString[sOffset] < '0' || aString[sOffset] > '9',
                            ERR_INVALID_LITERAL );
            sOffset++;
            for( ; sOffset < aLength; sOffset++ )
            {
                if( aString[sOffset] < '0' || aString[sOffset] > '9' )
                {
                    break;
                }
            }
            sMantissaEnd = &(aString[sOffset - 1]);
        }
        else
        {
            IDE_TEST_RAISE( aString[sOffset] < '0' || aString[sOffset] > '9',
                            ERR_INVALID_LITERAL );
            sMantissaStart = &(aString[sOffset]);
            sPointPosition = NULL;
            sOffset++;
            for( ; sOffset < aLength; sOffset++ )
            {
                if( aString[sOffset] == '.' )
                {
                    sPointPosition = &(aString[sOffset]);
                    sOffset++;
                    break;
                }
                if( aString[sOffset] < '0' || aString[sOffset] > '9' )
                {
                    break;
                }
            }
            for( ; sOffset < aLength; sOffset++ )
            {
                if( aString[sOffset] < '0' || aString[sOffset] > '9' )
                {
                    break;
                }
            }
            sMantissaEnd = &(aString[sOffset - 1]);
        }
        sExponent = 0;
        if( sOffset < aLength )
        {
            if( aString[sOffset] == 'E' || aString[sOffset] == 'e' )
            {
                sExponentSign = 1;
                sOffset++;
                IDE_TEST_RAISE( sOffset >= aLength, ERR_INVALID_LITERAL );
                switch( aString[sOffset] )
                {
                    case '-':
                        sExponentSign = -1;
                    case '+':
                        sOffset++;
                        IDE_TEST_RAISE( sOffset >= aLength, ERR_INVALID_LITERAL );
                }
                if( sExponentSign > 0 )
                {
                    IDE_TEST_RAISE( aString[sOffset] < '0' || aString[sOffset] > '9',
                                    ERR_INVALID_LITERAL );
                    for( ; sOffset < aLength; sOffset++ )
                    {
                        if( aString[sOffset] < '0' || aString[sOffset] > '9' )
                        {
                            break;
                        }
                        sExponent = sExponent * 10 + aString[sOffset] - '0';
                        IDE_TEST_RAISE( sExponent > 100000000,
                                        ERR_VALUE_OVERFLOW );
                    }
                    sExponent *= sExponentSign;
                }
                else
                {
                    IDE_TEST_RAISE( aString[sOffset] < '0' || aString[sOffset] > '9',
                                    ERR_INVALID_LITERAL );
                    for( ; sOffset < aLength; sOffset++ )
                    {
                        if( aString[sOffset] < '0' || aString[sOffset] > '9' )
                        {
                            break;
                        }
                        if( sExponent < 10000000 )
                        {
                            sExponent = sExponent * 10 + aString[sOffset] - '0';
                        }
                    }
                    sExponent *= sExponentSign;
                }
            }
        }
        for( ; sOffset < aLength; sOffset++ )
        {
            if( aString[sOffset] != ' ' && aString[sOffset] != '\t' )
            {
                break;
            }
        }
        IDE_TEST_RAISE( sOffset < aLength, ERR_INVALID_LITERAL );
        if( sPointPosition == NULL )
        {
            sPointPosition = sMantissaEnd + 1;
        }
        for( ; sMantissaStart <= sMantissaEnd; sMantissaStart++ )
        {
            if( *sMantissaStart != '0' && *sMantissaStart != '.' )
            {
                break;
            }
        }
        for( ; sMantissaEnd >= sMantissaStart; sMantissaEnd-- )
        {
            if( *sMantissaEnd != '0' && *sMantissaEnd != '.' )
            {
                break;
            }
        }
        if( sMantissaStart > sMantissaEnd )
        {
            /* Zero */
            aNumeric->length       = 1;
            aNumeric->signExponent = 0x80;
        }
        else
        {
            if( sMantissaStart < sPointPosition )
            {
                sExponent += sPointPosition - sMantissaStart;
            }
            else
            {
                sExponent -= sMantissaStart - sPointPosition - 1;
            }
            sMantissaIterator = 0;
            if( sSign > 0 )
            {
                aNumeric->length = 1;
                if( sExponent & 0x00000001 )
                {
                    aNumeric->mantissa[sMantissaIterator] =
                        *sMantissaStart - '0';
                    sMantissaStart++;
                    sMantissaIterator++;
                }
                sExponent = ( ( sExponent + 200000001 ) >> 1 ) - 100000000;
                for( ;
                     sMantissaIterator < (SInt)aMaximumMantissa;
                     sMantissaIterator++ )
                {
                    if( sMantissaEnd < sMantissaStart )
                    {
                        break;
                    }
                    if( *sMantissaStart == '.' )
                    {
                        sMantissaStart++;
                    }
                    if( sMantissaEnd - sMantissaStart < 1 )
                    {
                        break;
                    }
                    if( sMantissaStart[1] == '.' )
                    {
                        if( sMantissaEnd - sMantissaStart < 2 )
                        {
                            break;
                        }
                        aNumeric->mantissa[sMantissaIterator] =
                            ( sMantissaStart[0] - '0' ) * 10
                            + ( sMantissaStart[2] - '0' );
                        sMantissaStart += 3;
                    }
                    else
                    {
                        aNumeric->mantissa[sMantissaIterator] =
                            ( sMantissaStart[0] - '0' ) * 10
                            + ( sMantissaStart[1] - '0' );
                        sMantissaStart += 2;
                    }
                }
                if( sMantissaStart <= sMantissaEnd )
                {
                    if( *sMantissaStart == '.' )
                    {
                        sMantissaStart++;
                    }
                }
                if( sMantissaIterator < (SInt)aMaximumMantissa &&
                    sMantissaStart   == sMantissaEnd )
                {
                    aNumeric->mantissa[sMantissaIterator] =
                        ( sMantissaStart[0] - '0' ) * 10;
                    sMantissaStart++;
                    sMantissaIterator++;
                }
                if( sMantissaStart <= sMantissaEnd )
                {
                    if( sMantissaStart[0] >= '5' )
                    {
                        for( sMantissaIterator--;
                             sMantissaIterator >= 0;
                             sMantissaIterator-- )
                        {
                            aNumeric->mantissa[sMantissaIterator]++;
                            if( aNumeric->mantissa[sMantissaIterator] < 100 )
                            {
                                break;
                            }
                            aNumeric->mantissa[sMantissaIterator] = 0;
                        }
                        if( sMantissaIterator < 0 )
                        {
                            aNumeric->mantissa[0] = 1;
                            sExponent++;
                            sMantissaIterator++;
                        }
                        sMantissaIterator++;
                    }
                }

                IDE_TEST_RAISE( sExponent > 63, ERR_VALUE_OVERFLOW );
                if( sExponent >= -63 )
                {
                    aNumeric->length       = sMantissaIterator + 1;
                    aNumeric->signExponent = 0x80 | ( sExponent + 64 );
                }
                else
                {
                    aNumeric->length       = 1;
                    aNumeric->signExponent = 0x80;
                }
            }
            else
            {
                aNumeric->length = 1;
                if( sExponent & 0x00000001 )
                {
                    aNumeric->mantissa[sMantissaIterator] =
                        99 - ( *sMantissaStart - '0' );
                    sMantissaStart++;
                    sMantissaIterator++;
                }
                sExponent = ( ( sExponent + 200000001 ) >> 1 ) - 100000000;
                for( ;
                     sMantissaIterator < (SInt)aMaximumMantissa;
                     sMantissaIterator++ )
                {
                    if( sMantissaEnd < sMantissaStart )
                    {
                        break;
                    }
                    if( *sMantissaStart == '.' )
                    {
                        sMantissaStart++;
                    }
                    if( sMantissaEnd - sMantissaStart < 1 )
                    {
                        break;
                    }
                    if( sMantissaStart[1] == '.' )
                    {
                        if( sMantissaEnd - sMantissaStart < 2 )
                        {
                            break;
                        }
                        aNumeric->mantissa[sMantissaIterator] =
                            99 - ( ( sMantissaStart[0] - '0' ) * 10
                                   + ( sMantissaStart[2] - '0' ) );
                        sMantissaStart += 3;
                    }
                    else
                    {
                        aNumeric->mantissa[sMantissaIterator] =
                            99 - ( ( sMantissaStart[0] - '0' ) * 10
                                   + ( sMantissaStart[1] - '0' ) );
                        sMantissaStart += 2;
                    }
                }
                if( sMantissaStart <= sMantissaEnd )
                {
                    if( *sMantissaStart == '.' )
                    {
                        sMantissaStart++;
                    }
                }
                if( sMantissaIterator < (SInt)aMaximumMantissa &&
                    sMantissaStart == sMantissaEnd )
                {
                    aNumeric->mantissa[sMantissaIterator] =
                        99 - ( ( sMantissaStart[0] - '0' ) * 10 );
                    sMantissaStart++;
                    sMantissaIterator++;
                }
                if( sMantissaStart <= sMantissaEnd )
                {
                    if( sMantissaStart[0] >= '5' )
                    {
                        for( sMantissaIterator--;
                             sMantissaIterator >= 0;
                             sMantissaIterator-- )
                        {
                            if( aNumeric->mantissa[sMantissaIterator] == 0 )
                            {
                                aNumeric->mantissa[sMantissaIterator] = 99;
                            }
                            else
                            {
                                aNumeric->mantissa[sMantissaIterator]--;
                                break;
                            }
                        }
                        if( sMantissaIterator < 0 )
                        {
                            aNumeric->mantissa[0] = 98;
                            sExponent++;
                            sMantissaIterator++;
                        }
                        sMantissaIterator++;
                    }
                }

                IDE_TEST_RAISE( sExponent > 63, ERR_VALUE_OVERFLOW );
                if( sExponent >= -63 )
                {
                    aNumeric->length       = sMantissaIterator + 1;
                    aNumeric->signExponent = 64 - sExponent;
                }
                else
                {
                    aNumeric->length       = 1;
                    aNumeric->signExponent = 0x80;
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::numericCanonize( mtdNumericType *aValue,
                             mtdNumericType *aCanonizedValue,
                             SInt            aCanonPrecision,
                             SInt            aCanonScale,
                             idBool         *aCanonized )
{
    SInt sMaximumExponent;
    SInt sMinimumExponent;
    SInt sExponent;
    SInt sExponentEnd;
    SInt sCount;

    *aCanonized = ID_TRUE;

    if( aValue->length > 1 )
    {
        sMaximumExponent =
            ( ( ( aCanonPrecision - aCanonScale ) + 2001 ) >> 1 ) - 1000;
        sMinimumExponent =
            ( ( 2002 - aCanonScale ) >> 1 ) - 1000;
        if( aValue->signExponent & 0x80 )
        {
            sExponent = ( aValue->signExponent & 0x7F ) - 64;
            IDE_TEST_RAISE( sExponent > sMaximumExponent,
                            ERR_VALUE_OVERFLOW );
            if( sExponent == sMaximumExponent )
            {
                if( ( aCanonPrecision - aCanonScale ) & 0x01 )
                {
                    IDE_TEST_RAISE( aValue->mantissa[0] >= 10,
                                    ERR_VALUE_OVERFLOW );
                }
            }
            sExponentEnd = sExponent - aValue->length + 2;
            if( sExponent >= sMinimumExponent )
            {
                if( ( sExponentEnd < sMinimumExponent )   ||
                    ( sExponentEnd == sMinimumExponent  &&
                      ( aCanonScale & 0x01 ) == 1     &&
                      aValue->mantissa[sExponent-sExponentEnd] % 10 != 0 ) )
                {
                    if( aCanonScale & 0x01 )
                    {
                        sCount = sExponent - sMinimumExponent;
                        aCanonizedValue->signExponent = aValue->signExponent;
                        aCanonizedValue->length       = sCount + 2;
                        if( aValue->mantissa[sCount] % 10 >= 5 )
                        {
                            aCanonizedValue->mantissa[sCount] =
                                aValue->mantissa[sCount] - aValue->mantissa[sCount] % 10 + 10;
                            sCount--;
                        }
                        else
                        {
                            aCanonizedValue->mantissa[sCount] =
                                aValue->mantissa[sCount] - aValue->mantissa[sCount] % 10;
                            sCount--;
                            if( aCanonizedValue->mantissa[sCount+1] == 0 )
                            {
                                aCanonizedValue->length--;
                                for( ; sCount >= 0; sCount-- )
                                {
                                    aCanonizedValue->mantissa[sCount] =
                                        aValue->mantissa[sCount];
                                    if( aCanonizedValue->mantissa[sCount] != 0 )
                                    {
                                        break;
                                    }
                                    aCanonizedValue->length--;
                                }
                            }
                        }
                        if( aCanonizedValue->mantissa[sCount+1] > 99 )
                        {
                            aCanonizedValue->length--;
                            for( ; sCount >= 0; sCount-- )
                            {
                                aCanonizedValue->mantissa[sCount] =
                                    aValue->mantissa[sCount] + 1;
                                if( aCanonizedValue->mantissa[sCount] < 100 )
                                {
                                    sCount--;
                                    break;
                                }
                                aCanonizedValue->length--;
                            }
                        }
                    }
                    else
                    {
                        sCount = sExponent - sMinimumExponent;
                        aCanonizedValue->signExponent = aValue->signExponent;
                        aCanonizedValue->length       = sCount + 2;
                        if( aValue->mantissa[sCount+1] >= 50 )
                        {
                            for( ; sCount >= 0; sCount-- )
                            {
                                aCanonizedValue->mantissa[sCount] =
                                    aValue->mantissa[sCount] + 1;
                                if( aCanonizedValue->mantissa[sCount] < 100 )
                                {
                                    sCount--;
                                    break;
                                }
                                aCanonizedValue->length--;
                            }
                        }
                    }
                    for( ; sCount >= 0; sCount-- )
                    {
                        aCanonizedValue->mantissa[sCount] =
                            aValue->mantissa[sCount];
                    }

                    //------------------------------------------------
                    // To Fix BUG-13225
                    // ex) Numeric : 1  byte( length ) +
                    //               1  byte( sign & exponent ) +
                    //               20 byte( mantissa )
                    //
                    //     Numeric( 6 )일 때, 100000.01111은 다음과 같이
                    //     설정되어야 함
                    //     100000.0111 => length : 3
                    //                    sign & exponent : 197
                    //                    mantissa : 1
                    //     그러나 length가 5로 설정되는 문제가 있었음
                    //     원인 : mantissa값이 0인 경우, length를
                    //            줄여야 함
                    //------------------------------------------------

                    for( sCount=aCanonizedValue->length-2; sCount>0; sCount-- )
                    {
                        if( aCanonizedValue->mantissa[sCount] == 0 )
                        {
                            aCanonizedValue->length--;
                        }
                        else
                        {
                            break;
                        }
                    }

                    if( aCanonizedValue->mantissa[0] == 0 )
                    {
                        aCanonizedValue->length       = 1;
                        aCanonizedValue->signExponent = 0x80;
                    }
                    else
                    {
                        if( aCanonizedValue->mantissa[0] > 99 )
                        {
                            IDE_TEST_RAISE( aCanonizedValue->signExponent
                                            == 0xFF,
                                            ERR_VALUE_OVERFLOW );
                            aCanonizedValue->signExponent++;
                            aCanonizedValue->length      = 2;
                            aCanonizedValue->mantissa[0] = 1;
                        }
                        sExponent =
                            ( aCanonizedValue->signExponent & 0x7F ) - 64;
                        IDE_TEST_RAISE( sExponent > sMaximumExponent,
                                        ERR_VALUE_OVERFLOW );
                        if( sExponent == sMaximumExponent )
                        {
                            if( ( aCanonPrecision - aCanonScale )
                                & 0x01 )
                            {
                                IDE_TEST_RAISE( aCanonizedValue->mantissa[0]
                                                >= 10,
                                                ERR_VALUE_OVERFLOW );
                            }
                        }
                    }
                }
                else
                {
                    *aCanonized = ID_FALSE;
                }
            }
            else if ( sExponent == sMinimumExponent - 1 &&
                      ( aCanonScale & 0x01 ) == 0 )
            {
                if( aValue->mantissa[0] >= 50 )
                {
                    IDE_TEST_RAISE( aValue->signExponent == 0xFF,
                                    ERR_VALUE_OVERFLOW );
                    aCanonizedValue->length       = 2;
                    aCanonizedValue->signExponent = aValue->signExponent + 1;
                    aCanonizedValue->mantissa[0]  = 1;
                }
                else
                {
                    aCanonizedValue->length       = 1;
                    aCanonizedValue->signExponent = 0x80;
                }
                sExponent = ( aCanonizedValue->signExponent & 0x7F ) - 64;
                IDE_TEST_RAISE( sExponent > sMaximumExponent,
                                ERR_VALUE_OVERFLOW );
                if( sExponent == sMaximumExponent )
                {
                    if( ( aCanonPrecision - aCanonScale ) & 0x01 )
                    {
                        IDE_TEST_RAISE( aCanonizedValue->mantissa[0] >= 10,
                                        ERR_VALUE_OVERFLOW );
                    }
                }
            }
            else
            {
                aCanonizedValue->length       = 1;
                aCanonizedValue->signExponent = 0x80;
            }
        }
        else
        {
            sExponent = 64 - ( aValue->signExponent & 0x7F );
            IDE_TEST_RAISE( sExponent > sMaximumExponent,
                            ERR_VALUE_OVERFLOW );
            if( sExponent == sMaximumExponent )
            {
                if( ( aCanonPrecision - aCanonScale ) & 0x01 )
                {
                    IDE_TEST_RAISE( aValue->mantissa[0] <= 89,
                                    ERR_VALUE_OVERFLOW );
                }
            }
            sExponentEnd = sExponent - aValue->length + 2;
            if( sExponent >= sMinimumExponent )
            {
                if( sExponentEnd < sMinimumExponent ||
                    ( sExponentEnd == sMinimumExponent &&
                      ( aCanonScale & 0x01 ) == 1   &&
                      aValue->mantissa[sExponent-sExponentEnd] % 10 != 9 ) )
                {
                    if( aCanonScale & 0x01 )
                    {
                        sCount = sExponent - sMinimumExponent;
                        aCanonizedValue->signExponent = aValue->signExponent;
                        aCanonizedValue->length       = sCount + 2;
                        if( aValue->mantissa[sCount] % 10 <= 4 )
                        {
                            aCanonizedValue->mantissa[sCount] =
                                aValue->mantissa[sCount] - aValue->mantissa[sCount] % 10 - 1;
                            sCount--;
                        }
                        else
                        {
                            aCanonizedValue->mantissa[sCount] =
                                aValue->mantissa[sCount] - aValue->mantissa[sCount] % 10 + 9;
                            sCount--;
                            if( aCanonizedValue->mantissa[sCount+1] == 99 )
                            {
                                aCanonizedValue->length--;
                                for( ; sCount >= 0; sCount-- )
                                {
                                    aCanonizedValue->mantissa[sCount] =
                                        aValue->mantissa[sCount];
                                    if( aCanonizedValue->mantissa[sCount]
                                        != 99 )
                                    {
                                        break;
                                    }
                                    aCanonizedValue->length--;
                                }
                            }
                        }
                        if( aCanonizedValue->mantissa[sCount+1] > 99 )
                        {
                            aCanonizedValue->length--;
                            for( ; sCount >= 0; sCount-- )
                            {
                                aCanonizedValue->mantissa[sCount] =
                                    aValue->mantissa[sCount] - 1;
                                if( aCanonizedValue->mantissa[sCount] < 100 )
                                {
                                    sCount--;
                                    break;
                                }
                                aCanonizedValue->length--;
                            }
                        }
                    }
                    else
                    {
                        sCount = sExponent - sMinimumExponent;
                        aCanonizedValue->signExponent = aValue->signExponent;
                        aCanonizedValue->length       = sCount + 2;
                        if( aValue->mantissa[sCount+1] <= 49 )
                        {
                            for( ; sCount >= 0; sCount-- )
                            {
                                aCanonizedValue->mantissa[sCount] =
                                    aValue->mantissa[sCount] - 1;
                                if( aCanonizedValue->mantissa[sCount] <= 99 )
                                {
                                    sCount--;
                                    break;
                                }
                                aCanonizedValue->length--;
                            }
                        }
                    }
                    for( ; sCount >= 0; sCount-- )
                    {
                        aCanonizedValue->mantissa[sCount] =
                            aValue->mantissa[sCount];
                    }

                    // To Fix BUG-13225
                    for( sCount=aCanonizedValue->length-2; sCount>0; sCount-- )
                    {
                        if( aCanonizedValue->mantissa[sCount] == 99 )
                        {
                            aCanonizedValue->length--;
                        }
                        else
                        {
                            break;
                        }
                    }

                    if( aCanonizedValue->mantissa[0] == 99 )
                    {
                        aCanonizedValue->length       = 1;
                        aCanonizedValue->signExponent = 0x80;
                    }
                    else
                    {
                        if( aCanonizedValue->mantissa[0] > 99 )
                        {
                            IDE_TEST_RAISE( aCanonizedValue->signExponent
                                            == 0x01,
                                            ERR_VALUE_OVERFLOW );
                            aCanonizedValue->signExponent--;
                            aCanonizedValue->length      = 2;
                            aCanonizedValue->mantissa[0] = 98;
                        }
                        sExponent =
                            64 - ( aCanonizedValue->signExponent & 0x7F );
                        IDE_TEST_RAISE( sExponent > sMaximumExponent,
                                        ERR_VALUE_OVERFLOW );
                        if( sExponent == sMaximumExponent )
                        {
                            if( ( aCanonPrecision - aCanonScale )
                                & 0x01 )
                            {
                                IDE_TEST_RAISE( aCanonizedValue->mantissa[0]
                                                <= 89,
                                                ERR_VALUE_OVERFLOW );
                            }
                        }
                    }
                }
                else
                {
                    *aCanonized = ID_FALSE;
                }
            }
            else if ( sExponent == sMinimumExponent - 1 &&
                      ( aCanonScale & 0x01 ) == 0 )
            {
                if( aValue->mantissa[0] <= 49 )
                {
                    IDE_TEST_RAISE( aValue->signExponent == 0x01,
                                    ERR_VALUE_OVERFLOW );
                    aCanonizedValue->length       = 2;
                    aCanonizedValue->signExponent = aValue->signExponent - 1;
                    aCanonizedValue->mantissa[0]  = 98;
                }
                else
                {
                    aCanonizedValue->length       = 1;
                    aCanonizedValue->signExponent = 0x80;
                }
                sExponent = 64 - aCanonizedValue->signExponent;
                IDE_TEST_RAISE( sExponent > sMaximumExponent,
                                ERR_VALUE_OVERFLOW );
                if( sExponent == sMaximumExponent )
                {
                    if( ( aCanonPrecision - aCanonScale ) & 0x01 )
                    {
                        IDE_TEST_RAISE( aCanonizedValue->mantissa[0] <= 89,
                                        ERR_VALUE_OVERFLOW );
                    }
                }
            }
            else
            {
                aCanonizedValue->length       = 1;
                aCanonizedValue->signExponent = 0x80;
            }
        }
    }
    else
    {
        *aCanonized = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC mtc::floatCanonize( mtdNumericType *aValue,
                           mtdNumericType *aCanonizedValue,
                           SInt            aCanonPrecision,
                           idBool         *aCanonized )
{
    SInt sPrecision;
    SInt sCount;
    SInt sFence;

    *aCanonized = ID_TRUE;

    if( aValue->length > 1 )
    {
        sFence = aValue->length - 2;
        sPrecision = ( aValue->length - 1 ) << 1;
        if( aValue->signExponent & 0x80 )
        {
            if( aValue->mantissa[0] < 10 )
            {
                sPrecision--;
                if( aValue->mantissa[sFence] % 10 == 0 )
                {
                    sPrecision--;
                }
                if( sPrecision > aCanonPrecision )
                {
                    aCanonizedValue->signExponent = aValue->signExponent;
                    if( aCanonPrecision & 0x01 )
                    {
                        sCount = aCanonPrecision >> 1;
                        aCanonizedValue->length = sCount + 2;
                        if( aValue->mantissa[sCount+1] >= 50 )
                        {
                            for( ; sCount >= 0; sCount-- )
                            {
                                aCanonizedValue->mantissa[sCount] =
                                    aValue->mantissa[sCount] + 1;
                                if( aCanonizedValue->mantissa[sCount] < 100 )
                                {
                                    sCount--;
                                    break;
                                }
                                aCanonizedValue->length--;
                            }
                        }
                    }
                    else
                    {
                        sCount = aCanonPrecision >> 1;
                        aCanonizedValue->length = sCount + 2;
                        if( aValue->mantissa[sCount] % 10 >= 5 )
                        {
                            aCanonizedValue->mantissa[sCount] =
                                aValue->mantissa[sCount] - aValue->mantissa[sCount] % 10 + 10;
                        }
                        else
                        {
                            aCanonizedValue->mantissa[sCount] =
                                aValue->mantissa[sCount] - aValue->mantissa[sCount] % 10;
                        }
                        sCount--;
                        if( aCanonizedValue->mantissa[sCount+1] > 99 )
                        {
                            aCanonizedValue->length--;
                            for( ; sCount >= 0; sCount-- )
                            {
                                aCanonizedValue->mantissa[sCount] =
                                    aValue->mantissa[sCount] + 1;
                                if( aCanonizedValue->mantissa[sCount] < 100 )
                                {
                                    sCount--;
                                    break;
                                }
                                aCanonizedValue->length--;
                            }
                        }
                    }
                    for( ; sCount >= 0; sCount-- )
                    {
                        aCanonizedValue->mantissa[sCount] =
                            aValue->mantissa[sCount];
                    }
                }
                else
                {
                    *aCanonized = ID_FALSE;
                }
            }
            else
            {
                if( aValue->mantissa[sFence] % 10 == 0 )
                {
                    sPrecision--;
                }
                if( sPrecision > aCanonPrecision )
                {
                    aCanonizedValue->signExponent = aValue->signExponent;
                    if( aCanonPrecision & 0x01 )
                    {
                        sCount = ( aCanonPrecision - 1 ) >> 1;
                        aCanonizedValue->length = sCount + 2;
                        if( aValue->mantissa[sCount] % 10 >= 5 )
                        {
                            aCanonizedValue->mantissa[sCount] =
                                aValue->mantissa[sCount] - aValue->mantissa[sCount] % 10 + 10;
                        }
                        else
                        {
                            aCanonizedValue->mantissa[sCount] =
                                aValue->mantissa[sCount] - aValue->mantissa[sCount] % 10;
                        }
                        sCount--;
                        if( aCanonizedValue->mantissa[sCount+1] > 99 )
                        {
                            aCanonizedValue->length--;
                            for( ; sCount >= 0; sCount-- )
                            {
                                aCanonizedValue->mantissa[sCount] =
                                    aValue->mantissa[sCount] + 1;
                                if( aCanonizedValue->mantissa[sCount] < 100 )
                                {
                                    sCount--;
                                    break;
                                }
                                aCanonizedValue->length--;
                            }
                        }
                    }
                    else
                    {
                        sCount = ( aCanonPrecision - 1 ) >> 1;
                        aCanonizedValue->length = sCount + 2;
                        if( aValue->mantissa[sCount+1] >= 50 )
                        {
                            for( ; sCount >= 0; sCount-- )
                            {
                                aCanonizedValue->mantissa[sCount] =
                                    aValue->mantissa[sCount] + 1;
                                if( aCanonizedValue->mantissa[sCount] < 100 )
                                {
                                    sCount--;
                                    break;
                                }
                                aCanonizedValue->length--;
                            }
                        }
                    }
                    for( ; sCount >= 0; sCount-- )
                    {
                        aCanonizedValue->mantissa[sCount] =
                            aValue->mantissa[sCount];
                    }
                    if( aCanonizedValue->mantissa[0] > 99 )
                    {
                        IDE_TEST_RAISE( aCanonizedValue->signExponent == 0xFF,
                                        ERR_VALUE_OVERFLOW );
                        aCanonizedValue->signExponent++;
                        aCanonizedValue->length      = 2;
                        aCanonizedValue->mantissa[0] = 1;
                    }
                }
                else
                {
                    *aCanonized = ID_FALSE;
                }
            }
        }
        else
        {
            if( aValue->mantissa[0] > 89 )
            {
                sPrecision--;
                if( aValue->mantissa[sFence] % 10 == 9 )
                {
                    sPrecision--;
                }
                if( sPrecision > aCanonPrecision )
                {
                    aCanonizedValue->signExponent = aValue->signExponent;
                    if( aCanonPrecision & 0x01 )
                    {
                        sCount = aCanonPrecision >> 1;
                        aCanonizedValue->length = sCount + 2;
                        if( aValue->mantissa[sCount+1] <= 49 )
                        {
                            for( ; sCount >= 0; sCount-- )
                            {
                                aCanonizedValue->mantissa[sCount] =
                                    aValue->mantissa[sCount] - 1;
                                if( aCanonizedValue->mantissa[sCount] <= 99 )
                                {
                                    sCount--;
                                    break;
                                }
                                aCanonizedValue->length--;
                            }
                        }
                    }
                    else
                    {
                        sCount = aCanonPrecision >> 1;
                        aCanonizedValue->length = sCount + 2;
                        if( aValue->mantissa[sCount] % 10 <= 4 )
                        {
                            aCanonizedValue->mantissa[sCount] =
                                aValue->mantissa[sCount] - aValue->mantissa[sCount] % 10 - 1;
                        }
                        else
                        {
                            aCanonizedValue->mantissa[sCount] =
                                aValue->mantissa[sCount] - aValue->mantissa[sCount] % 10 + 9;
                        }
                        sCount--;
                        if( aCanonizedValue->mantissa[sCount+1] > 99 )
                        {
                            aCanonizedValue->length--;
                            for( ; sCount >= 0; sCount-- )
                            {
                                aCanonizedValue->mantissa[sCount] =
                                    aValue->mantissa[sCount] - 1;
                                if( aCanonizedValue->mantissa[sCount] < 100 )
                                {
                                    sCount--;
                                    break;
                                }
                                aCanonizedValue->length--;
                            }
                        }
                    }
                    for( ; sCount >= 0; sCount-- )
                    {
                        aCanonizedValue->mantissa[sCount] =
                            aValue->mantissa[sCount];
                    }
                }
                else
                {
                    *aCanonized = ID_FALSE;
                }
            }
            else
            {
                if( aValue->mantissa[sFence] % 10 == 9 )
                {
                    sPrecision--;
                }
                if( sPrecision > aCanonPrecision )
                {
                    aCanonizedValue->signExponent = aValue->signExponent;
                    if( aCanonPrecision & 0x01 )
                    {
                        sCount = ( aCanonPrecision - 1 ) >> 1;
                        aCanonizedValue->length = sCount + 2;
                        if( aValue->mantissa[sCount] % 10 <= 4 )
                        {
                            aCanonizedValue->mantissa[sCount] =
                                aValue->mantissa[sCount] - aValue->mantissa[sCount] % 10 - 1;
                        }
                        else
                        {
                            aCanonizedValue->mantissa[sCount] =
                                aValue->mantissa[sCount] - aValue->mantissa[sCount] % 10 + 9;
                        }
                        sCount--;
                        if( aCanonizedValue->mantissa[sCount+1] > 99 )
                        {
                            aCanonizedValue->length--;
                            for( ; sCount >= 0; sCount-- )
                            {
                                aCanonizedValue->mantissa[sCount] =
                                    aValue->mantissa[sCount] - 1;
                                if( aCanonizedValue->mantissa[sCount] < 100 )
                                {
                                    sCount--;
                                    break;
                                }
                                aCanonizedValue->length--;
                            }
                        }
                    }
                    else
                    {
                        sCount = ( aCanonPrecision - 1 ) >> 1;
                        aCanonizedValue->length = sCount + 2;
                        if( aValue->mantissa[sCount+1] <= 49 )
                        {
                            for( ; sCount >= 0; sCount-- )
                            {
                                aCanonizedValue->mantissa[sCount] =
                                    aValue->mantissa[sCount] - 1;
                                if( aCanonizedValue->mantissa[sCount] < 100 )
                                {
                                    sCount--;
                                    break;
                                }
                                aCanonizedValue->length--;
                            }
                        }
                    }
                    for( ; sCount >= 0; sCount-- )
                    {
                        aCanonizedValue->mantissa[sCount] =
                            aValue->mantissa[sCount];
                    }
                    if( aCanonizedValue->mantissa[0] > 99 )
                    {
                        IDE_TEST_RAISE( aCanonizedValue->signExponent == 0x01,
                                        ERR_VALUE_OVERFLOW );
                        aCanonizedValue->signExponent--;
                        aCanonizedValue->length      = 2;
                        // 음수이므로 carry발생후 값은 -1 즉, 98이 되어야 한다.
                        aCanonizedValue->mantissa[0] = 98;
                    }
                }
                else
                {
                    *aCanonized = ID_FALSE;
                }
            }
        }
    }
    else
    {
        *aCanonized = ID_FALSE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*
 * PROJ-1517
 * converts SLong to mtdNumericType
 */
void mtc::makeNumeric( mtdNumericType *aNumeric,
                       const SLong     aNumber )
{
    UInt   sCount;
    ULong  sULongValue;

    SShort i;
    SShort sExponent;
    UChar  sMantissa[MTD_FLOAT_MANTISSA_MAXIMUM];

    if ( aNumber == 0 )
    {
        aNumeric->length       = 1;
        aNumeric->signExponent = 0x80;
    }
    /* 양수 */
    else if ( aNumber > 0 )
    {
        sULongValue = aNumber;

        for ( i = 9; i >= 0; i-- )
        {
            sMantissa[i] = (UChar)(sULongValue % 100);
            sULongValue /= 100;
        }

        for ( sExponent = 0; sExponent < 10; sExponent++ )
        {
            if ( sMantissa[sExponent] != 0 )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
        for ( i = 0; i + sExponent < 10; i++ )
        {
            sMantissa[i] = sMantissa[i + sExponent];
        }

        for ( ; i < MTD_FLOAT_MANTISSA_MAXIMUM; i++ )
        {
            sMantissa[i] = 0;
        }
        sExponent = 10 - sExponent;

        aNumeric->signExponent = sExponent + 192;

        /* length 설정 */
        for ( sCount = 0, aNumeric->length = 1;
             sCount < ID_SIZEOF(sMantissa) - 1;
             sCount++ )
        {
            if ( sMantissa[sCount] != 0 )
            {
                aNumeric->length = sCount + 2;
            }
            else
            {
                /* Nothing to do */
            }
        }

        for ( sCount = 0;
             sCount < (UInt)( aNumeric->length - 1 );
             sCount++ )
        {
            aNumeric->mantissa[sCount] = sMantissa[sCount];
        }

    }
    /* 음수 */
    else
    {
        if ( aNumber == -ID_LONG(9223372036854775807) - ID_LONG(1) )
        {
            sULongValue = (ULong)(ID_ULONG(9223372036854775807) + ID_ULONG(1));
        }
        else
        {
            sULongValue = (ULong)(-aNumber);
        }

        for ( i = 9; i >= 0; i-- )
        {
            sMantissa[i] = (UChar)(sULongValue % 100);
            sULongValue /= 100;
        }

        for ( sExponent = 0; sExponent < 10; sExponent++ )
        {
            if ( sMantissa[sExponent] != 0 )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
        for ( i = 0; i + sExponent < 10; i++ )
        {
            sMantissa[i] = sMantissa[i + sExponent];
        }

        for ( ; i < MTD_FLOAT_MANTISSA_MAXIMUM; i++ )
        {
            sMantissa[i] = 0;
        }
        sExponent = 10 - sExponent;

        aNumeric->signExponent = 64 - sExponent;

        /* length 설정 */
        for ( sCount = 0, aNumeric->length = 1;
             sCount < ID_SIZEOF(sMantissa) - 1;
             sCount++ )
        {
            if ( sMantissa[sCount] != 0 )
            {
                aNumeric->length = sCount + 2;
            }
            else
            {
                /* Nothing to do */
            }
        }

        /* 음수 보수 변환 */
        for( sCount = 0;
             sCount < (UInt)( aNumeric->length - 1 );
             sCount++ )
        {
            aNumeric->mantissa[sCount] = 99 - sMantissa[sCount];
        }
    }
}

IDE_RC mtc::makeNumeric( mtdNumericType *aNumeric,
                         const SDouble   aNumber )
{
    SChar   sBuffer[32];

    if ( ( MTU_DOUBLE_TO_NUMERIC_FAST_CONV == 0 ) ||
         ( gIEEE754Double == ID_FALSE ) )
    {
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "%"ID_DOUBLE_G_FMT"",
                         aNumber );
    
        IDE_TEST( makeNumeric( aNumeric,
                               MTD_FLOAT_MANTISSA_MAXIMUM,
                               (const UChar*)sBuffer,
                               idlOS::strlen(sBuffer) )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( double2Numeric( aNumeric, aNumber )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtc::findCompare( const smiColumn* aColumn,
                         UInt             aFlag,
                         smiCompareFunc*  aCompare )
{
    const mtcColumn* sColumn;
    const mtdModule* sModule;
    UInt             sCompareType;

    sColumn = (const mtcColumn*)aColumn;

    IDE_TEST( mtd::moduleById( &sModule, sColumn->type.dataTypeId )
              != IDE_SUCCESS );

    sCompareType = MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL;

    if ( (aColumn->flag & SMI_COLUMN_COMPRESSION_MASK) != SMI_COLUMN_COMPRESSION_TRUE )
    {
        if ( ( aFlag & SMI_COLUMN_COMPARE_TYPE_MASK ) == SMI_COLUMN_COMPARE_NORMAL )
        {
            if ( (aColumn->flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_FIXED )
            {
                sCompareType = MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL;
            }
            else
            {
                sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;
            }
        }
        else if ( ( aFlag & SMI_COLUMN_COMPARE_TYPE_MASK ) == SMI_COLUMN_COMPARE_KEY_AND_VROW )
        {
            sCompareType = MTD_COMPARE_STOREDVAL_MTDVAL;
        }
        else if ( ( aFlag & SMI_COLUMN_COMPARE_TYPE_MASK ) == SMI_COLUMN_COMPARE_KEY_AND_KEY )
        {
            sCompareType = MTD_COMPARE_STOREDVAL_STOREDVAL;
        }
        else
        {
            /* PROJ-2433
             * Direct Key용 compare 함수 사용위해 type 세팅 */
            if ( ( aFlag & SMI_COLUMN_COMPARE_TYPE_MASK ) == SMI_COLUMN_COMPARE_DIRECT_KEY )
            {
                if ( (aColumn->flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_FIXED )
                {
                    sCompareType = MTD_COMPARE_INDEX_KEY_FIXED_MTDVAL;
                }
                else
                {
                    sCompareType = MTD_COMPARE_INDEX_KEY_MTDVAL;
                }
            }
            else
            {
                ideLog::log( IDE_ERR_0,
                             "aFlag : %"ID_UINT32_FMT"\n"
                             "sColumn->type.dataTypeId : %"ID_UINT32_FMT"\n",
                             aFlag,
                             sColumn->type.dataTypeId );

                IDE_ASSERT( 0 );
            }
        }
    }
    else
    {
        /* PROJ-2433
         * Direct Key Index는 compressed column을 지원하지않는다 */
        IDE_DASSERT( ( aFlag & SMI_COLUMN_COMPARE_TYPE_MASK ) !=
                     SMI_COLUMN_COMPARE_DIRECT_KEY );

        if ((aColumn->flag & SMI_COLUMN_STORAGE_MASK) == SMI_COLUMN_STORAGE_MEMORY) 
        {
            sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;
        }
        else if ((aColumn->flag & SMI_COLUMN_STORAGE_MASK) == SMI_COLUMN_STORAGE_DISK)
        {
            if ( ( aFlag & SMI_COLUMN_COMPARE_TYPE_MASK ) == SMI_COLUMN_COMPARE_NORMAL )
            {
                if ( (aColumn->flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_FIXED )
                {
                    sCompareType = MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL;
                }
                else
                {
                    sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;
                }
            }
            else if ( ( aFlag & SMI_COLUMN_COMPARE_TYPE_MASK ) == SMI_COLUMN_COMPARE_KEY_AND_VROW )
            {
                sCompareType = MTD_COMPARE_STOREDVAL_MTDVAL;
            }
            else if ( ( aFlag & SMI_COLUMN_COMPARE_TYPE_MASK ) == SMI_COLUMN_COMPARE_KEY_AND_KEY )
            {
                sCompareType = MTD_COMPARE_STOREDVAL_STOREDVAL;
            }
            else
            {
                ideLog::log( IDE_ERR_0,
                             "aFlag : %"ID_UINT32_FMT"\n"
                             "sColumn->type.dataTypeId : %"ID_UINT32_FMT"\n",
                             aFlag,
                             sColumn->type.dataTypeId );
 
                IDE_ASSERT( 0 );
            }
        }
        else
        {
            IDE_ASSERT( 0 );
        }
    }

    if( ( aFlag & SMI_COLUMN_ORDER_MASK ) == SMI_COLUMN_ORDER_ASCENDING )
    {
        *aCompare = (smiCompareFunc)
            sModule->keyCompare[sCompareType][MTD_COMPARE_ASCENDING];
    }
    else
    {
        *aCompare = (smiCompareFunc)
            sModule->keyCompare[sCompareType][MTD_COMPARE_DESCENDING];
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::findKey2String( const smiColumn   * aColumn,
                            UInt                /* aFlag */,
                            smiKey2StringFunc * aKey2String )
{
    const mtcColumn* sColumn;
    const mtdModule* sModule;

    sColumn = (const mtcColumn*)aColumn;

    IDE_TEST( mtd::moduleById( &sModule, sColumn->type.dataTypeId )
              != IDE_SUCCESS );

    *aKey2String = (smiKey2StringFunc) sModule->encode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::findNull( const smiColumn   * aColumn,
                      UInt                /* aFlag */,
                      smiNullFunc       * aNull )
{
    const mtcColumn* sColumn;
    const mtdModule* sModule;

    sColumn = (const mtcColumn*)aColumn;

    IDE_TEST( mtd::moduleById( &sModule, sColumn->type.dataTypeId )
              != IDE_SUCCESS );

    *aNull = (smiNullFunc)sModule->null;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::findIsNull( const smiColumn * aColumn,
                        UInt             /* aFlag */,
                        smiIsNullFunc   * aIsNull )
{
    const mtcColumn* sColumn;
    const mtdModule* sModule;

    sColumn = (const mtcColumn*)aColumn;

    IDE_TEST( mtd::moduleById( &sModule, sColumn->type.dataTypeId )
              != IDE_SUCCESS );

    *aIsNull = (smiIsNullFunc)sModule->isNull;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
mtc::findHashKeyFunc( const smiColumn * aColumn,
                      smiHashKeyFunc * aHashKeyFunc )
{
/***********************************************************************
 *
 * Description :
 *    Column의 Data Type에 부합하는 Hash Key 추출 함수를 얻는다.
 *    Data Type을 알 수 없는 하위 layer(SM)에서 hash key 추출 함수를
 *    얻기 위해 사용한다.
 *
 * Implementation :
 *
 *    Column의 데이터 Type에 해당하는 모듈을 획득하고,
 *    이에 대한 Hash Key 추출 함수를 Return한다.
 *
 ***********************************************************************/
    const mtcColumn* sColumn;
    const mtdModule* sModule;

    sColumn = (const mtcColumn*)aColumn;

    IDE_TEST( mtd::moduleById( &sModule, sColumn->type.dataTypeId )
              != IDE_SUCCESS );

    *aHashKeyFunc = (smiHashKeyFunc) sModule->hash;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
mtc::findStoredValue2MtdValue( const smiColumn             * aColumn,
                               smiCopyDiskColumnValueFunc  * aStoredValue2MtdValueFunc )
{
/***********************************************************************
 *
 * Description :
 *    Column의 Data Type에 부합하는 column value를 복사하는 함수를 얻는다.
 *    Data Type을 알 수 없는 하위 layer(SM)에서 colum value를 복사하는
 *    함수를 얻기 위해 사용한다.
 *
 *    PROJ-2429 Dictionary based data compress for on-disk DB
 *    dictionary compression colunm일 경우
 *    mtd::mtdStoredValue2MtdValue4CompressColumn함수를 반환하고
 *    그렇지 않을경우 데이터타입에 맞는 디스크테이블컬럼의
 *    데이타를 복사하는 함수를 반환 한다.
 *
 * Implementation :
 *
 *    Column의 데이터 Type에 해당하는 모듈을 획득하고,
 *    이에 대한 column value를 복사하는 함수를 리턴한다.
 ***********************************************************************/

    const mtcColumn* sColumn;
    const mtdModule* sModule;
    UInt             sFunctionIdx;

    sColumn = (const mtcColumn*)aColumn;

    IDE_TEST( mtd::moduleById( &sModule, sColumn->type.dataTypeId )
              != IDE_SUCCESS );

    if ( (aColumn->flag & SMI_COLUMN_COMPRESSION_MASK) 
        != SMI_COLUMN_COMPRESSION_TRUE )
    {
        sFunctionIdx = MTD_COLUMN_COPY_FUNC_NORMAL;
    }
    else
    {
        sFunctionIdx = MTD_COLUMN_COPY_FUNC_COMPRESSION;
    }

    *aStoredValue2MtdValueFunc =
        (smiCopyDiskColumnValueFunc)sModule->storedValue2MtdValue[sFunctionIdx];

    IDE_ASSERT( *aStoredValue2MtdValueFunc != NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
mtc::findStoredValue2MtdValue4DataType( const smiColumn             * aColumn,
                                        smiCopyDiskColumnValueFunc  * aStoredValue2MtdValueFunc )
{
/***********************************************************************
 *
 * Description :
 *    Column의 Data Type에 부합하는 column value를 복사하는 함수를 얻는다.
 *    Data Type을 알 수 없는 하위 layer(SM)에서 colum value를 복사하는
 *    함수를 얻기 위해 사용한다.
 *
 *    PROJ-2429 Dictionary based data compress for on-disk DB 
 *    dictionary compression colunm에 상관없이
 *    데이터타입에 맞는 디스크테이블컬럼의  데이타를 복사하는
 *    함수를 반환 한다.
 *
 *
 * Implementation :
 *
 *    Column의 데이터 Type에 해당하는 모듈을 획득하고,
 *    이에 대한 column value를 복사하는 함수를 리턴한다.
 ***********************************************************************/

    const mtcColumn* sColumn;
    const mtdModule* sModule;

    sColumn = (const mtcColumn*)aColumn;

    IDE_TEST( mtd::moduleById( &sModule, sColumn->type.dataTypeId )
              != IDE_SUCCESS );

    *aStoredValue2MtdValueFunc =
        (smiCopyDiskColumnValueFunc)sModule->storedValue2MtdValue[MTD_COLUMN_COPY_FUNC_NORMAL];

    IDE_ASSERT( *aStoredValue2MtdValueFunc != NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC mtc::findActualSize( const smiColumn   * aColumn,
                            smiActualSizeFunc * aActualSizeFunc )
{
    const mtcColumn* sColumn;
    const mtdModule* sModule;

    sColumn = (const mtcColumn*)aColumn;

    IDE_TEST( mtd::moduleById( &sModule, sColumn->type.dataTypeId )
              != IDE_SUCCESS );

    *aActualSizeFunc = (smiActualSizeFunc)sModule->actualSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


// Added For compacting Disk Index Slots by xcom73
IDE_RC mtc::getAlignValue( const smiColumn * aColumn,
                           UInt *            aAlignValue )
{
    const mtcColumn* sColumn;
    const mtdModule* sModule;

    sColumn = (const mtcColumn*)aColumn;

    IDE_TEST( mtd::moduleById( &sModule, sColumn->type.dataTypeId )
              != IDE_SUCCESS );

    *aAlignValue = sModule->align;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//--------------------------------------------------
// PROJ-1705
// sm에서 테이블레코드로부터 인덱스레코드를 구성하기 위해
// 해당 컬럼데이타의 size와 value ptr을 구한다.
// SM에서 인덱스레코드를 구성하기 위해서 아래의 절차로 수행되며,
//  (1) SM 디스크레코드를 읽어서 ->
//  (2) QP 영역 처리 가능한 레코드 구성  ->
//  (3) SM 인덱스레코드 구성
// 이 함수는 (2)->(3)의 수행시 호출된다.
//
// RP에서도 이 함수가 호출되는데,
// log를 읽어 mtdType의 value를 구성하고,
// 그 value의 mtdType의 length를 구할때 호출된다.
//--------------------------------------------------
IDE_RC mtc::getValueLengthFromFetchBuffer( idvSQL          * /*aStatistic*/,
                                           const smiColumn * aColumn,
                                           const void      * aRow,
                                           UInt            * aColumnSize,
                                           idBool          * aIsNullValue )
{
    const mtcColumn * sColumn;

    sColumn = (const mtcColumn*)aColumn;

    *aColumnSize = sColumn->module->actualSize( sColumn, aRow );

    if( aIsNullValue != NULL )
    {
        *aIsNullValue = sColumn->module->isNull( sColumn, aRow );
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

//    IDE_EXCEPTION_END;

//    return IDE_FAILURE;
}

//--------------------------------------------------
// PROJ-1705
// 디스크테이블컬럼의 데이타를
// qp 레코드처리영역의 해당 컬럼위치에 복사
//--------------------------------------------------
void mtc::storedValue2MtdValue( const smiColumn* aColumn,
                                void           * aDestValue,
                                UInt             aDestValueOffset,
                                UInt             aLength,
                                const void     * aValue )
{
    const mtcColumn * sColumn;
    UInt              sFunctionIdx;

    sColumn = (const mtcColumn*)aColumn;

    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인지 확인하고 dictionary column copy함수를
    // 전달한다. Dictionary compression column이 아닌경우 data type에 맞는 함수를
    // 전달 한다.
    if ( (aColumn->flag & SMI_COLUMN_COMPRESSION_MASK) 
        != SMI_COLUMN_COMPRESSION_TRUE )
    {
        sFunctionIdx = MTD_COLUMN_COPY_FUNC_NORMAL;
    }
    else
    {
        sFunctionIdx = MTD_COLUMN_COPY_FUNC_COMPRESSION;
    }

    sColumn->module->storedValue2MtdValue[sFunctionIdx]( sColumn->column.size,
                                                         aDestValue,
                                                         aDestValueOffset,
                                                         aLength,
                                                         aValue );
}


IDE_RC mtc::cloneTuple( iduMemory   * aMemory,
                        mtcTemplate * aSrcTemplate,
                        UShort        aSrcTupleID,
                        mtcTemplate * aDstTemplate,
                        UShort        aDstTupleID )
{
    ULong sFlag;
    SInt  i;

    aDstTemplate->rows[aDstTupleID].modify
        = aSrcTemplate->rows[aSrcTupleID].modify ;
    aDstTemplate->rows[aDstTupleID].lflag
        = aSrcTemplate->rows[aSrcTupleID].lflag ;
    aDstTemplate->rows[aDstTupleID].columnCount
        = aSrcTemplate->rows[aSrcTupleID].columnCount ;
    // 실제 컬럼 개수만 복사해야 한다.
    aDstTemplate->rows[aDstTupleID].columnMaximum
        = aSrcTemplate->rows[aSrcTupleID].columnCount;
    aDstTemplate->rows[aDstTupleID].rowOffset
        = aSrcTemplate->rows[aSrcTupleID].rowOffset ;
    aDstTemplate->rows[aDstTupleID].rowMaximum
        = aSrcTemplate->rows[aSrcTupleID].rowMaximum ;
    aDstTemplate->rows[aDstTupleID].columns = NULL ;
    aDstTemplate->rows[aDstTupleID].execute = NULL ;
    aDstTemplate->rows[aDstTupleID].row = NULL ;

    // PROJ-1502 PARTITIONED DISK TABLE
    aDstTemplate->rows[aDstTupleID].partitionTupleID = aDstTupleID;

    // PROJ-1502 PARTITIONED DISK TABLE
    aDstTemplate->rows[aDstTupleID].tableHandle = aSrcTemplate->rows[aSrcTupleID].tableHandle;

    // PROJ-2205 rownum in DML
    aDstTemplate->rows[aDstTupleID].cursorInfo = NULL;

    sFlag = aSrcTemplate->rows[aSrcTupleID].lflag;


    /* clone mtcTuple.columns */
    if ( sFlag & MTC_TUPLE_COLUMN_SET_TRUE )
    {
        aDstTemplate->rows[aDstTupleID].columns
            = aSrcTemplate->rows[aSrcTupleID].columns;
    }

    if ( sFlag & MTC_TUPLE_COLUMN_ALLOCATE_TRUE )
    {
        IDE_TEST(aMemory->alloc( ID_SIZEOF(mtcColumn) *
                                 aDstTemplate->rows[aDstTupleID].columnMaximum,
                                 (void**)
                                 &(aDstTemplate->rows[aDstTupleID].columns))
                 != IDE_SUCCESS);
    }

    if ( sFlag & MTC_TUPLE_COLUMN_COPY_TRUE )
    {
        for( i = 0 ; i < aDstTemplate->rows[aDstTupleID].columnMaximum; i++ )
        {
            /* BUG-44382 clone tuple 성능개선 */
            if ( ( sFlag & MTC_TUPLE_TYPE_MASK ) == MTC_TUPLE_TYPE_TABLE )
            {
                aDstTemplate->rows[aDstTupleID].columns[i] =
                    aSrcTemplate->rows[aSrcTupleID].columns[i];
            }
            else
            {
                // intermediate tuple
                MTC_COPY_COLUMN_DESC( &(aDstTemplate->rows[aDstTupleID].columns[i]),
                                      &(aSrcTemplate->rows[aSrcTupleID].columns[i]) );
            }
        }
        /*
          idlOS::memcpy( aDstTemplate->rows[aDstTupleID].columns,
          aSrcTemplate->rows[aSrcTupleID].columns,
          ID_SIZEOF(mtcColumn)
          * aDstTemplate->rows[aDstTupleID].columnMaximum );
          */
    }

    /* clone mtcTuple.execute */
    if ( sFlag & MTC_TUPLE_EXECUTE_SET_TRUE )
    {
        aDstTemplate->rows[aDstTupleID].execute
            = aSrcTemplate->rows[aSrcTupleID].execute;
    }

    if ( sFlag & MTC_TUPLE_EXECUTE_ALLOCATE_TRUE )
    {
        IDE_TEST(aMemory->alloc( ID_SIZEOF(mtcExecute) *
                                 aDstTemplate->rows[aDstTupleID].columnMaximum,
                                 (void**)
                                 &(aDstTemplate->rows[aDstTupleID].execute))
                 != IDE_SUCCESS);
    }

    if ( sFlag & MTC_TUPLE_EXECUTE_COPY_TRUE )
    {
        if (aSrcTemplate->rows[aSrcTupleID].execute != NULL)
        {
            for( i = 0; i < aDstTemplate->rows[aDstTupleID].columnMaximum; i++ )
            {
                aDstTemplate->rows[aDstTupleID].execute[i] =
                    aSrcTemplate->rows[aSrcTupleID].execute[i];
            }
            /*
              idlOS::memcpy( aDstTemplate->rows[aDstTupleID].execute,
              aSrcTemplate->rows[aSrcTupleID].execute,
              ID_SIZEOF(mtcExecute)
              * aDstTemplate->rows[aDstTupleID].columnMaximum );
              */
        }
    }

    /* PROJ-1789 PROWID */
    aDstTemplate->rows[aDstTupleID].ridExecute =
        aSrcTemplate->rows[aSrcTupleID].ridExecute;

    /* clone mtcTuple.row */
    if ( sFlag & MTC_TUPLE_ROW_SET_TRUE )
    {
        aDstTemplate->rows[aDstTupleID].row
            = aSrcTemplate->rows[aSrcTupleID].row;
    }

    if ( sFlag & MTC_TUPLE_ROW_ALLOCATE_TRUE )
    {
        if ( aDstTemplate->rows[aDstTupleID].rowMaximum > 0 )
        {
            /* BUG-44382 clone tuple 성능개선 */
            if ( sFlag & MTC_TUPLE_ROW_MEMSET_TRUE )
            {
                // BUG-15548
                IDE_TEST( aMemory->cralloc(
                              ID_SIZEOF(SChar) * aDstTemplate->rows[aDstTupleID].rowMaximum,
                              (void**)&(aDstTemplate->rows[aDstTupleID].row))
                          != IDE_SUCCESS);
            }
            else
            {
                IDE_TEST( aMemory->alloc(
                              ID_SIZEOF(SChar) * aDstTemplate->rows[aDstTupleID].rowMaximum,
                              (void**)&(aDstTemplate->rows[aDstTupleID].row))
                          != IDE_SUCCESS);
            }
        }
        else
        {
            aDstTemplate->rows[aDstTupleID].row = NULL;
        }
    }

    if ( sFlag & MTC_TUPLE_ROW_COPY_TRUE )
    {
        idlOS::memcpy( aDstTemplate->rows[aDstTupleID].row,
                       aSrcTemplate->rows[aSrcTupleID].row,
                       ID_SIZEOF(SChar)
                       * aDstTemplate->rows[aDstTupleID].rowMaximum );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::cloneTuple( iduVarMemList * aMemory,
                        mtcTemplate   * aSrcTemplate,
                        UShort          aSrcTupleID,
                        mtcTemplate   * aDstTemplate,
                        UShort          aDstTupleID )
{
    ULong sFlag;
    SInt  i;

    aDstTemplate->rows[aDstTupleID].modify
        = aSrcTemplate->rows[aSrcTupleID].modify ;
    aDstTemplate->rows[aDstTupleID].lflag
        = aSrcTemplate->rows[aSrcTupleID].lflag ;
    aDstTemplate->rows[aDstTupleID].columnCount
        = aSrcTemplate->rows[aSrcTupleID].columnCount ;
    // 실제 컬럼 개수만 복사해야 한다.
    aDstTemplate->rows[aDstTupleID].columnMaximum
        = aSrcTemplate->rows[aSrcTupleID].columnCount;
    aDstTemplate->rows[aDstTupleID].rowOffset
        = aSrcTemplate->rows[aSrcTupleID].rowOffset ;
    aDstTemplate->rows[aDstTupleID].rowMaximum
        = aSrcTemplate->rows[aSrcTupleID].rowMaximum ;
    aDstTemplate->rows[aDstTupleID].columns = NULL ;
    aDstTemplate->rows[aDstTupleID].execute = NULL ;
    aDstTemplate->rows[aDstTupleID].row = NULL ;

    // PROJ-1502 PARTITIONED DISK TABLE
    aDstTemplate->rows[aDstTupleID].partitionTupleID = aDstTupleID;

    // PROJ-1502 PARTITIONED DISK TABLE
    aDstTemplate->rows[aDstTupleID].tableHandle = aSrcTemplate->rows[aSrcTupleID].tableHandle;

    // PROJ-2205 rownum in DML
    aDstTemplate->rows[aDstTupleID].cursorInfo = NULL;

    sFlag = aSrcTemplate->rows[aSrcTupleID].lflag;


    /* clone mtcTuple.columns */
    if ( sFlag & MTC_TUPLE_COLUMN_SET_TRUE )
    {
        aDstTemplate->rows[aDstTupleID].columns
            = aSrcTemplate->rows[aSrcTupleID].columns;
    }

    if ( sFlag & MTC_TUPLE_COLUMN_ALLOCATE_TRUE )
    {
        IDE_TEST(aMemory->alloc( ID_SIZEOF(mtcColumn) *
                                 aDstTemplate->rows[aDstTupleID].columnMaximum,
                                 (void**)
                                 &(aDstTemplate->rows[aDstTupleID].columns))
                 != IDE_SUCCESS);
    }

    if ( sFlag & MTC_TUPLE_COLUMN_COPY_TRUE )
    {
        for( i = 0 ; i < aDstTemplate->rows[aDstTupleID].columnMaximum; i++ )
        {
            /* BUG-44382 clone tuple 성능개선 */
            if ( ( sFlag & MTC_TUPLE_TYPE_MASK ) == MTC_TUPLE_TYPE_TABLE )
            {
                aDstTemplate->rows[aDstTupleID].columns[i] =
                    aSrcTemplate->rows[aSrcTupleID].columns[i];
            }
            else
            {
                // intermediate tuple
                MTC_COPY_COLUMN_DESC( &(aDstTemplate->rows[aDstTupleID].columns[i]),
                                      &(aSrcTemplate->rows[aSrcTupleID].columns[i]) );
            }
        }
        /*
          idlOS::memcpy( aDstTemplate->rows[aDstTupleID].columns,
          aSrcTemplate->rows[aSrcTupleID].columns,
          ID_SIZEOF(mtcColumn)
          * aDstTemplate->rows[aDstTupleID].columnMaximum );
          */
    }

    /* clone mtcTuple.execute */
    if ( sFlag & MTC_TUPLE_EXECUTE_SET_TRUE )
    {
        aDstTemplate->rows[aDstTupleID].execute
            = aSrcTemplate->rows[aSrcTupleID].execute;
    }

    if ( sFlag & MTC_TUPLE_EXECUTE_ALLOCATE_TRUE )
    {
        IDE_TEST(aMemory->alloc( ID_SIZEOF(mtcExecute) *
                                 aDstTemplate->rows[aDstTupleID].columnMaximum,
                                 (void**)
                                 &(aDstTemplate->rows[aDstTupleID].execute))
                 != IDE_SUCCESS);
    }

    if ( sFlag & MTC_TUPLE_EXECUTE_COPY_TRUE )
    {
        if (aSrcTemplate->rows[aSrcTupleID].execute != NULL)
        {
            for( i = 0; i < aDstTemplate->rows[aDstTupleID].columnMaximum; i++ )
            {
                aDstTemplate->rows[aDstTupleID].execute[i] =
                    aSrcTemplate->rows[aSrcTupleID].execute[i];
            }
            /*
              idlOS::memcpy( aDstTemplate->rows[aDstTupleID].execute,
              aSrcTemplate->rows[aSrcTupleID].execute,
              ID_SIZEOF(mtcExecute)
              * aDstTemplate->rows[aDstTupleID].columnMaximum );
              */
        }
    }

    /* PROJ-1789 PROWID */
    aDstTemplate->rows[aDstTupleID].ridExecute=
        aSrcTemplate->rows[aSrcTupleID].ridExecute;

    /* clone mtcTuple.row */
    if ( sFlag & MTC_TUPLE_ROW_SET_TRUE )
    {
        aDstTemplate->rows[aDstTupleID].row
            = aSrcTemplate->rows[aSrcTupleID].row;
    }

    if ( sFlag & MTC_TUPLE_ROW_ALLOCATE_TRUE )
    {
        if ( aDstTemplate->rows[aDstTupleID].rowMaximum > 0 )
        {
            /* BUG-44382 clone tuple 성능개선 */
            if ( sFlag & MTC_TUPLE_ROW_MEMSET_TRUE )
            {
                // BUG-15548
                IDE_TEST( aMemory->cralloc(
                              ID_SIZEOF(SChar) * aDstTemplate->rows[aDstTupleID].rowMaximum,
                              (void**)&(aDstTemplate->rows[aDstTupleID].row))
                          != IDE_SUCCESS);
            }
            else
            {
                IDE_TEST( aMemory->alloc(
                              ID_SIZEOF(SChar) * aDstTemplate->rows[aDstTupleID].rowMaximum,
                              (void**)&(aDstTemplate->rows[aDstTupleID].row))
                          != IDE_SUCCESS);
            }
        }
        else
        {
            aDstTemplate->rows[aDstTupleID].row = NULL;
        }
    }

    if ( sFlag & MTC_TUPLE_ROW_COPY_TRUE )
    {
        idlOS::memcpy( aDstTemplate->rows[aDstTupleID].row,
                       aSrcTemplate->rows[aSrcTupleID].row,
                       ID_SIZEOF(SChar)
                       * aDstTemplate->rows[aDstTupleID].rowMaximum );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************
 *
 * 세션 정보중에 dateFormat이 추가되었는데
 * 이 정보는 cloneTemplate의 source, target이 공유하게 되어 있다.
 * 따라서 이 함수는 같은 세션 내에서만 사용되어야 한다.
 * 혹은 다른 세션의 template을 복사하는 경우에는
 * 반드시 dateFormat을 따로 assign 해 주어야 한다.
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * By kumdory, 2005-04-08
 *
 ******************************************************************/
IDE_RC mtc::cloneTemplate( iduMemory    * aMemory,
                           mtcTemplate  * aSource,
                           mtcTemplate  * aDestination )
{
    UShort sTupleIndex;

    aDestination->stackCount      = aSource->stackCount;
    aDestination->dataSize        = aSource->dataSize;

    // To Fix PR-9600
    // To Fix PR-9600
    // mtcTemplate::execInfo, mtcTemplate::execInfoCnt 멤버의 추가로
    // 해당 정보에 대한 복사를 해주어야 함.
    aDestination->execInfoCnt     = aSource->execInfoCnt;

    aDestination->initSubquery    = aSource->initSubquery;
    aDestination->fetchSubquery   = aSource->fetchSubquery;
    aDestination->finiSubquery    = aSource->finiSubquery;
    aDestination->setCalcSubquery = aSource->setCalcSubquery;
    aDestination->getOpenedCursor = aSource->getOpenedCursor;
    aDestination->addOpenedLobCursor = aSource->addOpenedLobCursor;
    aDestination->isBaseTable     = aSource->isBaseTable;
    aDestination->closeLobLocator = aSource->closeLobLocator;
    aDestination->getSTObjBufSize = aSource->getSTObjBufSize;
    aDestination->rowCount        = aSource->rowCount;
    aDestination->rowCountBeforeBinding  = aSource->rowCountBeforeBinding;
    aDestination->variableRow     = aSource->variableRow;

    // PROJ-2002 Column Security
    aDestination->encrypt          = aSource->encrypt;
    aDestination->decrypt          = aSource->decrypt;
    aDestination->encodeECC        = aSource->encodeECC;
    aDestination->getDecryptInfo   = aSource->getDecryptInfo;
    aDestination->getECCInfo       = aSource->getECCInfo;
    aDestination->getECCSize       = aSource->getECCSize;

    // BUG-11192관련
    // template에 dateFormat이 추가되면서 cloneTemplate에서
    // 복사를 해주어야 한다.
    // cloneTemplate은 같은 세션내에서만 사용되므로
    // 포인터만 assign하면 된다. (malloc해서 카피할 필요는 없다)
    aDestination->dateFormat      = aSource->dateFormat;
    aDestination->dateFormatRef   = aSource->dateFormatRef;

    /* PROJ-2208 Multi Currency */
    aDestination->nlsCurrency     = aSource->nlsCurrency;
    aDestination->nlsCurrencyRef  = aSource->nlsCurrencyRef;

    // BUG-37247
    aDestination->groupConcatPrecisionRef = aSource->groupConcatPrecisionRef;

    // BUG-41944
    aDestination->arithmeticOpMode    = aSource->arithmeticOpMode;
    aDestination->arithmeticOpModeRef = aSource->arithmeticOpModeRef;

    idlOS::memcpy(aDestination->currentRow,
                  aSource->currentRow,
                  MTC_TUPLE_TYPE_MAXIMUM * ID_SIZEOF(UShort) );

    // stack buffer is set on execution phase. PR2475
    aDestination->stackCount  = 0;
    aDestination->stackBuffer = NULL;
    aDestination->stackRemain = 0;
    aDestination->stack       = NULL;

    if ( aDestination->dataSize > 0 )
    {
        IDE_TEST(aMemory->alloc( ID_SIZEOF (UChar) *
                                 aDestination->dataSize,
                                 (void**)&(aDestination->data))
                 != IDE_SUCCESS);
    }
    else
    {
        aDestination->data = NULL ;
    }

    // To Fix PR-9600
    // mtcTemplate::execInfo, mtcTemplate::execInfoCnt 멤버의 추가로
    // 해당 정보에 대한 복사를 해주어야 함.
    if ( aDestination->execInfoCnt > 0 )
    {
        IDE_TEST(aMemory->alloc( aDestination->execInfoCnt *
                                 ID_SIZEOF(UInt),
                                 (void**)&(aDestination->execInfo))
                 != IDE_SUCCESS);
    }
    else
    {
        aDestination->execInfo = NULL;
    }

    // PROJ-2527 WITHIN GROUP AGGR
    aDestination->funcDataCnt = aSource->funcDataCnt;
        
    if ( aDestination->funcDataCnt > 0 )
    {
        IDU_FIT_POINT( "mtc::cloneTemplate::alloc::funcData",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( aMemory->alloc( aDestination->funcDataCnt *
                                  ID_SIZEOF(mtfFuncDataBasicInfo*),
                                  (void**)&(aDestination->funcData) )
                  != IDE_SUCCESS);
    }
    else
    {
        aDestination->funcData = NULL;
    }
    
    for ( sTupleIndex = 0                                   ;
          sTupleIndex < aDestination->rowCountBeforeBinding ;
          sTupleIndex ++ )
    {
        IDE_TEST( mtc::cloneTuple( aMemory,
                                   aSource,
                                   sTupleIndex,
                                   aDestination,
                                   sTupleIndex )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::cloneTemplate( iduVarMemList * aMemory,
                           mtcTemplate   * aSource,
                           mtcTemplate   * aDestination )
{
    UShort sTupleIndex;

    aDestination->stackCount      = aSource->stackCount;
    aDestination->dataSize        = aSource->dataSize;

    // To Fix PR-9600
    // To Fix PR-9600
    // mtcTemplate::execInfo, mtcTemplate::execInfoCnt 멤버의 추가로
    // 해당 정보에 대한 복사를 해주어야 함.
    aDestination->execInfoCnt     = aSource->execInfoCnt;

    aDestination->initSubquery    = aSource->initSubquery;
    aDestination->fetchSubquery   = aSource->fetchSubquery;
    aDestination->finiSubquery    = aSource->finiSubquery;
    aDestination->setCalcSubquery = aSource->setCalcSubquery;
    aDestination->getOpenedCursor = aSource->getOpenedCursor;
    aDestination->addOpenedLobCursor = aSource->addOpenedLobCursor;
    aDestination->isBaseTable     = aSource->isBaseTable;
    aDestination->closeLobLocator = aSource->closeLobLocator;
    aDestination->getSTObjBufSize = aSource->getSTObjBufSize;
    aDestination->rowCount        = aSource->rowCount;
    aDestination->rowCountBeforeBinding  = aSource->rowCountBeforeBinding;
    aDestination->variableRow     = aSource->variableRow;

    // PROJ-2002 Column Security
    aDestination->encrypt          = aSource->encrypt;
    aDestination->decrypt          = aSource->decrypt;
    aDestination->encodeECC        = aSource->encodeECC;
    aDestination->getDecryptInfo   = aSource->getDecryptInfo;
    aDestination->getECCInfo       = aSource->getECCInfo;
    aDestination->getECCSize       = aSource->getECCSize;

    // BUG-11192관련
    // template에 dateFormat이 추가되면서 cloneTemplate에서
    // 복사를 해주어야 한다.
    // cloneTemplate은 같은 세션내에서만 사용되므로
    // 포인터만 assign하면 된다. (malloc해서 카피할 필요는 없다)
    aDestination->dateFormat      = aSource->dateFormat;
    aDestination->dateFormatRef   = aSource->dateFormatRef;

    /* PROJ-2208 Multi Currency */
    aDestination->nlsCurrency     = aSource->nlsCurrency;
    aDestination->nlsCurrencyRef  = aSource->nlsCurrencyRef;

    // BUG-37247
    aDestination->groupConcatPrecisionRef = aSource->groupConcatPrecisionRef;

    // BUG-41944
    aDestination->arithmeticOpMode    = aSource->arithmeticOpMode;
    aDestination->arithmeticOpModeRef = aSource->arithmeticOpModeRef;
    
    idlOS::memcpy(aDestination->currentRow,
                  aSource->currentRow,
                  MTC_TUPLE_TYPE_MAXIMUM * ID_SIZEOF(UShort) );

    // stack buffer is set on execution phase. PR2475
    aDestination->stackCount  = 0;
    aDestination->stackBuffer = NULL;
    aDestination->stackRemain = 0;
    aDestination->stack       = NULL;

    if ( aDestination->dataSize > 0 )
    {
        IDE_TEST(aMemory->alloc( ID_SIZEOF (UChar) *
                                 aDestination->dataSize,
                                 (void**)&(aDestination->data))
                 != IDE_SUCCESS);
    }
    else
    {
        aDestination->data = NULL ;
    }

    // To Fix PR-9600
    // mtcTemplate::execInfo, mtcTemplate::execInfoCnt 멤버의 추가로
    // 해당 정보에 대한 복사를 해주어야 함.
    if ( aDestination->execInfoCnt > 0 )
    {
        IDE_TEST(aMemory->alloc( aDestination->execInfoCnt *
                                 ID_SIZEOF(UInt),
                                 (void**)&(aDestination->execInfo))
                 != IDE_SUCCESS);
    }
    else
    {
        aDestination->execInfo = NULL;
    }

    // PROJ-2527 WITHIN GROUP AGGR
    aDestination->funcDataCnt = aSource->funcDataCnt;
        
    if ( aDestination->funcDataCnt > 0 )
    {
        IDU_FIT_POINT( "mtc::cloneTemplate::alloc::funcData",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( aMemory->alloc( aDestination->funcDataCnt *
                                  ID_SIZEOF(mtfFuncDataBasicInfo*),
                                  (void**)&(aDestination->funcData) )
                  != IDE_SUCCESS);
    }
    else
    {
        aDestination->funcData = NULL;
    }
    
    for ( sTupleIndex = 0                                   ;
          sTupleIndex < aDestination->rowCountBeforeBinding ;
          sTupleIndex ++ )
    {
        IDE_TEST( mtc::cloneTuple( aMemory,
                                   aSource,
                                   sTupleIndex,
                                   aDestination,
                                   sTupleIndex )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************
 *
 * PROJ-2527 WITHIN GROUP AGGR
 *
 * cloneTemplate시에 생성한 객체들에 대해 finalize한다.
 *
 ******************************************************************/
void mtc::finiTemplate( mtcTemplate  * aTemplate )
{
    /* BUG-46892 */
    idvSQL * sStatistics = NULL;
    ULong    sTotalSize  = (ULong)0;
    UInt     i;

    sStatistics = mtc::getStatistics( aTemplate );

    if ( aTemplate->funcData != NULL )
    {
        for ( i = 0; i < aTemplate->funcDataCnt; i++ )
        {
            if ( aTemplate->funcData[i] != NULL )
            {
                sTotalSize += aTemplate->funcData[i]->memoryMgr->getSize(); /* BUG-46892 */

                mtf::freeFuncDataMemory( aTemplate->funcData[i]->memoryMgr );

                aTemplate->funcData[i] = NULL;
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    /* BUG-46892 */
    if ( sStatistics != NULL )
    {
        IDV_SQL_SET( sStatistics, mMathTempMem, sTotalSize );
    }
    else
    {
        /* Nothing to do */
    }
}

/*-----------------------------------------------------------------------------
 * Name :
 *    IDE_RC mtc::addFloat()
 *
 * Argument :
 *    aValue - point of value , 계산 되어지는 값
 *    aArgument1
 *    aArgument2 - 입력값 aValue = aArgument1 + aArgument2
 *    aPrecisionMaximum - 자리수
 *
 * Description :
 *    1. exponent계산
 *    2. 자리수대로 덧셈 ; carry 생각
 *    3. 맨앞의 캐리는 자리수+1 해준다
 * ---------------------------------------------------------------------------*/


IDE_RC mtc::addFloat( mtdNumericType *aValue,
                      UInt            aPrecisionMaximum,
                      mtdNumericType *aArgument1,
                      mtdNumericType *aArgument2 )
{
    SInt sCarry;
    SInt sExponent1;
    SInt sExponent2;
    SInt mantissaIndex;
    SInt tempLength;
    SInt longLength;
    SInt shortLength;

    SInt lIndex;
    SInt sIndex;
    SInt checkIndex;

    SInt sLongExponent;
    SInt sShortExponent;

    mtdNumericType *longArg;
    mtdNumericType *shortArg;

//    idlOS::memset( aValue->mantissa , 0 , ID_SIZEOF(aValue->mantissa) );

    if( aArgument1->length == 0 || aArgument2->length == 0 )
    {
        aValue->length = 0;
    }
    else if( aArgument1->length == 1 )
    {
        aValue->length = aArgument2->length;
        aValue->signExponent = aArgument2->signExponent;
        idlOS::memcpy( aValue->mantissa , aArgument2->mantissa , aArgument2->length - 1);
    }
    else if( aArgument2->length == 1 )
    {
        aValue->length = aArgument1->length;
        aValue->signExponent = aArgument1->signExponent;
        idlOS::memcpy( aValue->mantissa , aArgument1->mantissa , aArgument1->length - 1);
    }
    else
    {
        //exponent
        if( (aArgument1->signExponent & 0x80) == 0x80 )
        {
            sExponent1 = (aArgument1->signExponent & 0x7F) - 64;
        }
        else
        {
            sExponent1 = 64 - (aArgument1->signExponent & 0x7F);
        }

        if( (aArgument2->signExponent & 0x80) == 0x80 )
        {
            sExponent2 = (aArgument2->signExponent & 0x7F) - 64;
        }
        else
        {
            sExponent2 = 64 - (aArgument2->signExponent & 0x7F);
        }

        //maxLength
        if( sExponent1 < sExponent2 )
        {
            tempLength = aArgument1->length + (sExponent2 - sExponent1);
            sExponent1 = sExponent2;

            if( tempLength > aArgument2->length )
            {
                longLength = tempLength;
                shortLength = aArgument2->length;
                longArg = aArgument1;
                shortArg = aArgument2;
            }
            else
            {
                longLength = aArgument2->length;
                shortLength = tempLength;
                longArg = aArgument2;
                shortArg = aArgument1;
            }
        }
        else
        {
            tempLength = aArgument2->length + (sExponent1 - sExponent2);

            if( tempLength > aArgument1->length )
            {
                longLength = tempLength;
                shortLength = aArgument1->length;
                longArg = aArgument2;
                shortArg = aArgument1;
            }
            else
            {
                longLength = aArgument1->length;
                shortLength = tempLength;
                longArg = aArgument1;
                shortArg = aArgument2;
            }
        }

        //옮겨져서 0으로 채워진곳이 21번를 넘으면
        if( longLength - longArg->length > (SInt)(aPrecisionMaximum / 2 + 2) )
        {
            aValue->length = shortArg->length;
            aValue->signExponent = shortArg->signExponent;
            idlOS::memcpy( aValue->mantissa ,
                           shortArg->mantissa ,
                           shortArg->length);
            return IDE_SUCCESS;
        }

        if( longLength > (SInt) (aPrecisionMaximum / 2 + 2) )
        {
            // BUG-10557 fix
            // maxLength가 21이 넘을 때, 21로 줄이는데
            // 이때, argument1 또는 argument2의 연산에 포함할 범위를
            // maxLength가 줄어든 만큼 빼줘야 한다.
            //
            // 예를 들어 158/10000*17/365 + 1를 계산할 때,
            // 158/10000*17/365는 0.00073589... (length=20) 인데,
            // maxLength는 22에서 21로 줄일때,
            // 0.00073589...의 길이를 19로 줄여 마지막 한자리는 포기해야 한다.
            // subtractFloat에서도 마찬가지로 이런 처리를 해주어야 한다.
            //
            lIndex = longArg->length - 2 - (longLength - 21);
            longLength = 21;
        }
        else
        {
            lIndex = longArg->length - 2;
        }

        sIndex = shortArg->length - 2;

        sCarry = 0;

        sLongExponent = longArg->signExponent & 0x80;
        sShortExponent = shortArg->signExponent & 0x80;

        //     1  : 2 :   3
        //   xxxxxxxxx________
        //+  ______xxxxxxxxxxx
        //그리고 겹치지 않는 공간
        //빈공간은 POSITIVE : 0
        //         NEGATIVE : 99
        for( mantissaIndex = longLength - 2 ; mantissaIndex >= 0 ; mantissaIndex-- )
        {
            // 3구역?
            if( mantissaIndex > (shortLength -2) && lIndex >= 0)
            {
                if( sShortExponent == 0x80 )
                {
                    aValue->mantissa[mantissaIndex] =
                        longArg->mantissa[lIndex] + sCarry;
                }
                else
                {
                    aValue->mantissa[mantissaIndex] =
                        longArg->mantissa[lIndex] + 99 + sCarry;
                }
                lIndex--;
            }
            //빈공간
            else if( mantissaIndex > (shortLength -2) && lIndex < 0)
            {
                //캐리 생각
                aValue->mantissa[mantissaIndex] = 0;
                if( sShortExponent == 0x00 )
                {
                    aValue->mantissa[mantissaIndex] = 99;
                }
                if( sLongExponent == 0x00 )
                {
                    aValue->mantissa[mantissaIndex] += 99;
                }
                aValue->mantissa[mantissaIndex] += sCarry;
            }
            //2구역
            else if( mantissaIndex <= (shortLength - 2) && lIndex >=0 && sIndex >=0)
            {
                aValue->mantissa[mantissaIndex] =
                    longArg->mantissa[lIndex] +
                    shortArg->mantissa[sIndex] + sCarry;
                lIndex--;
                sIndex--;
            }
            //1구역
            else if( lIndex < 0 )
            {
                if( sLongExponent == 0x80 )
                {
                    aValue->mantissa[mantissaIndex] =
                        shortArg->mantissa[sIndex] + sCarry;
                }
                else
                {
                    aValue->mantissa[mantissaIndex] =
                        shortArg->mantissa[sIndex] + 99 + sCarry;
                }
                sIndex--;
            }
            else if( sIndex < 0 )
            {

                if( sShortExponent == 0x80 )
                {
                    aValue->mantissa[mantissaIndex] =
                        longArg->mantissa[lIndex] + sCarry;
                }
                else
                {
                    aValue->mantissa[mantissaIndex] =
                        longArg->mantissa[lIndex] + 99 + sCarry;
                }
                lIndex--;
            }

            //캐리 설정
            if( aValue->mantissa[mantissaIndex] >= 100 )
            {
                aValue->mantissa[mantissaIndex] -= 100;
                sCarry = 1;
            }
            else
            {
                sCarry = 0;
            }
        }

        //음수로 인한 캐리는 자리수 이동이 없다 그리고 +1을 해준다
        //양수는 자리 이동
        if( sLongExponent != sShortExponent )
        {
            if( sCarry )
            {
                for( mantissaIndex = longLength - 2 ; mantissaIndex >= 0 ; mantissaIndex-- )
                {
                    aValue->mantissa[mantissaIndex] = aValue->mantissa[mantissaIndex] + sCarry;
                    if( aValue->mantissa[mantissaIndex] >= 100 )
                    {
                        sCarry = 1;
                        aValue->mantissa[mantissaIndex] -= 100;
                    }
                    else
                    {
                        break;
                    }
                }
                sCarry = 1;
            }
        }
        //둘다 음수
        //무조건1 더해준다
        else if( sLongExponent == 0x00 && sShortExponent == 0x00 )
        {
            if( sCarry == 0 )
            {
                for( mantissaIndex = longLength - 1 ; mantissaIndex > 0 ; mantissaIndex-- )
                {
                    aValue->mantissa[mantissaIndex] = aValue->mantissa[mantissaIndex - 1];
                }
                aValue->mantissa[0] = 98;
                sExponent1++;
                longLength++;
            }

            sCarry = 1;
            for( mantissaIndex = longLength - 2 ; mantissaIndex >= 0 ; mantissaIndex-- )
            {
                aValue->mantissa[mantissaIndex] = aValue->mantissa[mantissaIndex] + sCarry;
                if( aValue->mantissa[mantissaIndex] >= 100 )
                {
                    sCarry = 1;
                    aValue->mantissa[mantissaIndex] -= 100;
                }
                else
                {
                    sCarry = 0;
                    break;
                }
            }

        }
        else
        {
            if( sCarry )
            {
                for( mantissaIndex = longLength - 1 ; mantissaIndex > 0 ; mantissaIndex-- )
                {
                    aValue->mantissa[mantissaIndex] = aValue->mantissa[mantissaIndex - 1];
                }
                aValue->mantissa[0] = 1;
                sExponent1++;
                longLength++;
            }
        }

        IDE_TEST_RAISE( sExponent1 > 63 , ERR_OVERFLOW);

        checkIndex = 0;

        if( (sShortExponent == 0x80 && sLongExponent == 0x80) ||
            (sShortExponent != sLongExponent &&
             sCarry == 1 ) )
        {

            //앞의 0을 업애준다
            for( mantissaIndex = 0 ; mantissaIndex < longLength - 1 ; mantissaIndex++ )
            {
                if( aValue->mantissa[mantissaIndex] == 0 )
                {
                    checkIndex++;
                }
                else
                {
                    break;
                }
            }
            for( mantissaIndex = 0 ; mantissaIndex < longLength - 1 - checkIndex ; mantissaIndex++)
            {
                aValue->mantissa[mantissaIndex] = aValue->mantissa[mantissaIndex + checkIndex];
            }
            longLength -= checkIndex;
            sExponent1 -= checkIndex;

            //양수
            //뒤의 0
            mantissaIndex = longLength - 2;

            //BUG-fix 4733 - UMR remove
            while( mantissaIndex >= 0 )
            {
                if( aValue->mantissa[mantissaIndex] == 0 )
                {
                    longLength--;
                    mantissaIndex--;
                }
                else
                {
                    break;
                }
            }

            aValue->signExponent = sExponent1 + 192;
        }
        else
        {
            //앞의 99를 업애준다
            for( mantissaIndex = 0 ; mantissaIndex < longLength - 1 ; mantissaIndex++ )
            {
                if( aValue->mantissa[mantissaIndex] == 99 )
                {
                    checkIndex++;
                }
                else
                {
                    break;
                }
            }
            for( mantissaIndex = 0 ; mantissaIndex < longLength - 1 - checkIndex ; mantissaIndex++)
            {
                aValue->mantissa[mantissaIndex] = aValue->mantissa[mantissaIndex + checkIndex];
            }
            longLength -= checkIndex;
            sExponent1 -= checkIndex;

            //음수
            //뒤의 99
            mantissaIndex = longLength - 2;

            //BUG-fix 4733 UMR remove
            while( mantissaIndex > 0 )
            {
                if( aValue->mantissa[mantissaIndex] == 99 )
                {
                    mantissaIndex--;
                    longLength--;
                }
                else
                {
                    break;
                }
            }

            //-0이 되면
            if( longLength == 1 )
            {
                aValue->signExponent = 0x80;
            }
            else
            {
                aValue->signExponent = 64 - sExponent1;
            }
        }

        // To fix BUG-12970
        // 최대 길이가 넘는 경우 보정해 줘야 함.
        if( longLength > 21 )
        {
            aValue->length = 21;
        }
        else
        {
            aValue->length = longLength;
        }

        //BUGBUG
        //precision계산(유효숫자 , length와 연관해서)
        //precision자리에서 반올림해준다.
        //aPrecisionMaximum
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_OVERFLOW );
    IDE_SET(ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *    IDE_RC mtc::subtractFloat()
 *
 * Argument :
 *    aValue - point of value , 계산 되어지는 값
 *    aArgument1
 *    aArgument2 - 입력값 aValue = aArgument1 - aArgument2
 *    aPrecisionMaximum - 자리수
 *
 * Description :
 *    1. exponent계산
 *    2. 자리수대로 뺄셈 ; borrow 생각 음수일경우 99의 보수로 계산
 *    3. 맨앞의 캐리는 자리수+1 해준다
 * ---------------------------------------------------------------------------*/

IDE_RC mtc::subtractFloat( mtdNumericType *aValue,
                           UInt            aPrecisionMaximum,
                           mtdNumericType *aArgument1,
                           mtdNumericType *aArgument2 )
{
    SInt sBorrow;
    SInt sExponent1;
    SInt sExponent2;

    SInt mantissaIndex;
    SInt maxLength;
    SInt sArg1Length;
    SInt sArg2Length;
    SInt sDiffExp;

    SInt sArg1Index;
    SInt sArg2Index;
    SInt lIndex;
    SInt sIndex;

    SInt isZero;

    SInt sArgExponent1;
    SInt sArgExponent2;

    mtdNumericType* longArg;
    mtdNumericType* shortArg;
    SInt longLength;
    SInt shortLength;
    SInt tempLength;

    SInt i;

    if( aArgument1->length == 0 || aArgument2->length == 0 )
    {
        aValue->length = 0;
    }
    else if( aArgument1->length == 1 )
    {
        aValue->length = aArgument2->length;

        //부호 바꾸고 보수( + -> - )
        aValue->signExponent = 256 - aArgument2->signExponent;

        for( mantissaIndex = 0 ;
             mantissaIndex < aArgument2->length - 1 ;
             mantissaIndex++ )
        {
            aValue->mantissa[mantissaIndex] =
                99 - aArgument2->mantissa[mantissaIndex];
        }

    }
    else if( aArgument2->length == 1 )
    {
        aValue->length = aArgument1->length;
        aValue->signExponent = aArgument1->signExponent;
        idlOS::memcpy( aValue->mantissa ,
                       aArgument1->mantissa ,
                       aArgument1->length - 1);
    }

    else
    {
        sArgExponent1 = aArgument1->signExponent & 0x80;
        sArgExponent2 = aArgument2->signExponent & 0x80;

        //exponent
        if( sArgExponent1 == 0x80 )
        {
            sExponent1 = (aArgument1->signExponent & 0x7F) - 64;
        }
        else
        {
            sExponent1 = 64 - (aArgument1->signExponent & 0x7F);
        }

        if( sArgExponent2 == 0x80 )
        {
            sExponent2 = (aArgument2->signExponent & 0x7F) - 64;
        }
        else
        {
            sExponent2 = 64 - (aArgument2->signExponent & 0x7F);
        }


        // To fix BUG-13569 addFloat과 같이
        // long argument와 short argument로 구분.
        //maxLength
        if( sExponent1 < sExponent2 )
        {
            sDiffExp = sExponent2 - sExponent1;
            tempLength = aArgument1->length + sDiffExp;
            sExponent1 = sExponent2;

            if( tempLength > aArgument2->length )
            {
                longLength = tempLength;
                shortLength = aArgument2->length;
                longArg = aArgument1;
                shortArg = aArgument2;
            }
            else
            {
                longLength = aArgument2->length;
                shortLength = tempLength;
                longArg = aArgument2;
                shortArg = aArgument1;
            }
        }
        else
        {
            sDiffExp = sExponent1 - sExponent2;
            tempLength = aArgument2->length + sDiffExp;

            if( tempLength > aArgument1->length )
            {
                longLength = tempLength;
                shortLength = aArgument1->length;
                longArg = aArgument2;
                shortArg = aArgument1;
            }
            else
            {
                longLength = aArgument1->length;
                shortLength = tempLength;
                longArg = aArgument1;
                shortArg = aArgument2;
            }
        }

        //옮겨져서 0으로 채워진곳이 21번를 넘으면
        if( longLength - longArg->length > (SInt)(aPrecisionMaximum / 2 + 2) )
        {
            if( shortArg == aArgument1 )
            {
                aValue->length = shortArg->length;
                aValue->signExponent = shortArg->signExponent;
                idlOS::memcpy( aValue->mantissa ,
                               shortArg->mantissa ,
                               shortArg->length);
                return IDE_SUCCESS;
            }
            else
            {
                aValue->length = shortArg->length;

                aValue->signExponent = 256 - shortArg->signExponent;

                for( mantissaIndex = 0 ;
                     mantissaIndex < shortArg->length - 1 ;
                     mantissaIndex++ )
                {
                    aValue->mantissa[mantissaIndex] =
                        99 - shortArg->mantissa[mantissaIndex];
                }
            }
        }

        if( longLength > (SInt) (aPrecisionMaximum / 2 + 2) )
        {
            // BUG-10557 fix
            // maxLength가 21이 넘을 때, 21로 줄이는데
            // 이때, argument1 또는 argument2의 연산에 포함할 범위를
            // maxLength가 줄어든 만큼 빼줘야 한다.
            //
            // 예를 들어 158/10000*17/365 + 1를 계산할 때,
            // 158/10000*17/365는 0.00073589... (length=20) 인데,
            // maxLength는 22에서 21로 줄일때,
            // 0.00073589...의 길이를 19로 줄여 마지막 한자리는 포기해야 한다.
            // subtractFloat에서도 마찬가지로 이런 처리를 해주어야 한다.
            //
            lIndex = longArg->length - 2 - (longLength - 21);
            longLength = 21;
        }
        else
        {
            lIndex = longArg->length - 2;
        }

        maxLength = longLength;


        sIndex = shortArg->length - 2;


        if( longArg == aArgument1 )
        {
            sArg1Index = lIndex;
            sArg1Length = longLength;
            sArg2Index = sIndex;
            sArg2Length = shortLength;
        }
        else
        {
            sArg1Index = sIndex;
            sArg1Length = shortLength;
            sArg2Index = lIndex;
            sArg2Length = longLength;
        }

        sBorrow = 0;
        //빼기

        //     1  : 2 :   3
        //   xxxxxxxxx________
        //-  ______xxxxxxxxxxx
        //그리고 겹치지 않는 공간
        //빈공간은 POSITIVE : 0
        //         NEGATIVE : 99

        for( mantissaIndex = maxLength - 2 ;
             mantissaIndex >= 0 ;
             mantissaIndex-- )
        {
            //3구역
            if( sArg1Length > sArg2Length && sArg1Index >= 0)
            {
                if( sArgExponent2 == 0x00 )
                {
                    if( aArgument1->mantissa[sArg1Index] - sBorrow < 99 )
                    {
                        aValue->mantissa[mantissaIndex] =
                            (100 + aArgument1->mantissa[sArg1Index]- sBorrow)
                            - 99 ;
                        sBorrow = 1;
                    }
                    else
                    {
                        aValue->mantissa[mantissaIndex] = 0;
                        sBorrow = 0;
                    }
                }
                else
                {
                    aValue->mantissa[mantissaIndex] =
                        aArgument1->mantissa[sArg1Index];
                }
                sArg1Index--;
                sArg1Length--;
            }
            else if( sArg1Length < sArg2Length && sArg2Index >= 0)
            {
                //양수면
                if( sArgExponent1 == 0x80 )
                {
                    aValue->mantissa[mantissaIndex] =
                        100 - sBorrow - aArgument2->mantissa[sArg2Index];
                    sBorrow = 1;
                }
                else
                {
                    aValue->mantissa[mantissaIndex] =
                        99 - aArgument2->mantissa[sArg2Index];
                    sBorrow = 0;
                }
                sArg2Index--;
                sArg2Length--;
            }
            //겹치는 공간(1이 길거나 2가 길거나)
            else if( (sArg1Index == (sArg2Index + sDiffExp) ||
                      sArg2Index == (sArg1Index + sDiffExp)) &&
                     sArg1Index >= 0 && sArg2Index >= 0)
            {
                if( aArgument1->mantissa[sArg1Index] <
                    aArgument2->mantissa[sArg2Index] + sBorrow )
                {
                    aValue->mantissa[mantissaIndex] =
                        (100 + aArgument1->mantissa[sArg1Index]) -
                        aArgument2->mantissa[sArg2Index] - sBorrow;
                    sBorrow = 1;
                }
                else
                {
                    aValue->mantissa[mantissaIndex] =
                        aArgument1->mantissa[sArg1Index] -
                        aArgument2->mantissa[sArg2Index] - sBorrow;
                    sBorrow = 0;
                }
                sArg1Index--;
                sArg2Index--;
            }
            //겹치지 않는 공간
            else if( (sArg2Index < 0 && mantissaIndex > sArg1Index ) ||
                     (sArg1Index < 0 && mantissaIndex > sArg2Index ) )
            {
                if(sBorrow)
                {
                    //양수
                    if( sArgExponent1 == 0x80 )
                    {
                        if( sArgExponent2 == 0x80 )
                        {
                            aValue->mantissa[mantissaIndex] = 99;
                        }
                        else
                        {
                            aValue->mantissa[mantissaIndex] = 0;
                        }
                        sBorrow = 1;

                    }
                    //음수
                    else
                    {
                        if( sArgExponent2 == 0x80 )
                        {
                            aValue->mantissa[mantissaIndex] = 98;
                            sBorrow = 0;
                        }
                        else
                        {
                            aValue->mantissa[mantissaIndex] = 99;
                            sBorrow = 1;
                        }
                    }
                }
                else
                {
                    //양수
                    if( sArgExponent1 == 0x80 )
                    {
                        if( sArgExponent2 == 0x80 )
                        {
                            aValue->mantissa[mantissaIndex] = 0;
                            sBorrow = 0;
                        }
                        else
                        {
                            aValue->mantissa[mantissaIndex] = 1;
                            sBorrow = 1;
                        }
                    }
                    //음수
                    else
                    {
                        if( sArgExponent2 == 0x80 )
                        {
                            aValue->mantissa[mantissaIndex] = 99;
                        }
                        else
                        {
                            aValue->mantissa[mantissaIndex] = 0;
                        }
                        sBorrow = 0;
                    }
                }
            }
            //1구역
            else if( sArg2Index < 0 )
            {
                //음수
                if( sArgExponent2 == 0x00 )
                {
                    if( aArgument1->mantissa[sArg1Index] - sBorrow < 99 )
                    {

                        aValue->mantissa[mantissaIndex] =
                            (100 + aArgument1->mantissa[sArg1Index] - sBorrow)
                            - 99;
                        sBorrow = 1;
                    }
                    else
                    {
                        aValue->mantissa[mantissaIndex] = 0;
                        sBorrow = 0;
                    }
                }
                else
                {
                    if( aArgument1->mantissa[sArg1Index] < sBorrow )
                    {
                        aValue->mantissa[mantissaIndex] =
                            100 - aArgument1->mantissa[sArg1Index] - sBorrow;
                        sBorrow = 1;
                    }
                    else
                    {
                        aValue->mantissa[mantissaIndex] =
                            aArgument1->mantissa[sArg1Index] - sBorrow;
                        sBorrow = 0;
                    }
                }
                sArg1Index--;
            }
            else if( sArg1Index < 0 )
            {
                if( sArgExponent1 == 0x00 )
                {
                    if( 99 - sBorrow < aArgument2->mantissa[sArg2Index] )
                    {
                        aValue->mantissa[mantissaIndex] =
                            100 + 99 - aArgument2->mantissa[sArg2Index]
                            - sBorrow;
                        sBorrow = 1;
                    }
                    else
                    {
                        aValue->mantissa[mantissaIndex] =
                            99 - aArgument2->mantissa[sArg2Index] - sBorrow;
                        sBorrow = 0;
                    }
                }
                else
                {
                    if( sBorrow )
                    {
                        aValue->mantissa[mantissaIndex] =
                            99 - aArgument2->mantissa[sArg2Index];
                    }
                    else
                    {
                        if( aArgument2->mantissa[sArg2Index] == 0 )
                        {
                            aValue->mantissa[mantissaIndex] = 0;

                        }
                        else
                        {
                            aValue->mantissa[mantissaIndex] =
                                100 - aArgument2->mantissa[sArg2Index];
                            sBorrow = 1;
                        }
                    }
                }
                sArg2Index--;
            }
        }

        IDE_TEST_RAISE( sExponent1 > 63 , ERR_OVERFLOW);

        if( sArgExponent1 == sArgExponent2 )
        {
            if( sBorrow )
            {
                //양수
                for( mantissaIndex = maxLength - 2 ;
                     mantissaIndex >= 0 ;
                     mantissaIndex-- )
                {
                    if( aValue->mantissa[mantissaIndex] == 0 )
                    {
                        aValue->mantissa[mantissaIndex] = 99;
                    }
                    else
                    {
                        aValue->mantissa[mantissaIndex]--;
                        break;
                    }
                }


                //맨앞의 바로우 발생햇는데 99일경우만 즉 빼기해서 0이 나올때
                if( aValue->mantissa[0] == 99 )
                {
                    SShort checkIndex = 0;
                    //앞의 99를 업애준다
                    for( mantissaIndex = 0 ;
                         mantissaIndex < maxLength - 1 ;
                         mantissaIndex++ )
                    {
                        if( aValue->mantissa[mantissaIndex] == 99 )
                        {
                            checkIndex++;
                        }
                        else
                        {
                            break;
                        }
                    }
                    for( mantissaIndex = 0 ;
                         mantissaIndex < maxLength - 1 - checkIndex ;
                         mantissaIndex++)
                    {
                        aValue->mantissa[mantissaIndex] =
                            aValue->mantissa[mantissaIndex + checkIndex];
                    }
                    maxLength -= checkIndex;
                    sExponent1 -= checkIndex;

                }
                aValue->signExponent = (64 - sExponent1);

            }
            else
            {

                //음수가 된다.
                isZero = 0;
                mantissaIndex = 0;

                while(mantissaIndex < maxLength - 1)
                {
                    if( aValue->mantissa[mantissaIndex] == 0 )
                    {
                        mantissaIndex++;
                        isZero++;
                    }
                    else
                    {
                        break;
                    }
                }



                //0일때
                if( isZero == maxLength - 1)
                {
                    aValue->length = 1;
                    aValue->signExponent = 0x80;
                    return IDE_SUCCESS;
                }
                else if( isZero )
                {
                    // fix BUG-10360 valgrind error remove

                    // idlOS::memcpy(aValue->mantissa ,
                    //               aValue->mantissa + isZero ,
                    //               maxLength - isZero);
                    for( i = 0; i < ( maxLength - isZero) ; i++ )
                    {
                        *(aValue->mantissa + i)
                            = *(aValue->mantissa + isZero + i);
                    }

                    maxLength = maxLength - isZero ;
                    sExponent1 -= isZero;
                }

                aValue->signExponent = (sExponent1 + 192);

            }
        }
        else
        {
            //양수

            if( sArgExponent1 == 0x80 )
            {

                for( mantissaIndex = maxLength - 2 ;
                     mantissaIndex >= 0 ;
                     mantissaIndex-- )
                {
                    if( aValue->mantissa[mantissaIndex] == 0 )
                    {
                        aValue->mantissa[mantissaIndex] = 99;
                        if( mantissaIndex == 0 )
                        {
                            sBorrow = 1;
                        }
                    }
                    else
                    {
                        aValue->mantissa[mantissaIndex]--;
                        break;
                    }
                }
                if( !sBorrow ){
                    for( mantissaIndex = maxLength - 1 ;
                         mantissaIndex > 0 ;
                         mantissaIndex-- )
                    {
                        aValue->mantissa[mantissaIndex] =
                            aValue->mantissa[mantissaIndex - 1];
                    }
                    aValue->mantissa[0] = 1;
                    sExponent1++;
                    maxLength++;
                }

                aValue->signExponent = (sExponent1 + 192);

            }
            else
            {
                if( sBorrow ){
                    //98더해줌
                    for( mantissaIndex = maxLength - 1 ;
                         mantissaIndex > 0 ;
                         mantissaIndex-- )
                    {
                        aValue->mantissa[mantissaIndex] =
                            aValue->mantissa[mantissaIndex - 1];
                    }
                    aValue->mantissa[0] = 98;
                    sExponent1++;
                    maxLength++;
                }
                aValue->signExponent = (64 - sExponent1);

            }
        }

        // BUG-12334 fix
        // '-' 연산 결과 끝자리가 99로 끝나는 경우 nomalize가 필요하다.
        // 예를 들어,
        // -220 - 80 하면
        // -220: length=3, exponent=62,  mantissa=97,79
        // 80  : length=2, exponent=193, mantissa=80
        // 결과: length=3, exponent=62,  mantissa=96,99
        // 인데, -330은 length=2, exponent=62, mantissa=96이다.
        // 결론적으로 마지막의 99, 0은 없애고 length를 1 줄여야 한다.
        // 양수일 경우엔 0을 음수일 경우엔 99를 없앤다.
        // by kumdory, 2005-07-11
        if( ( aValue->signExponent & 0x80 ) > 0 )
        {
            for( mantissaIndex = maxLength - 2;
                 mantissaIndex > 0;
                 mantissaIndex-- )
            {
                if( aValue->mantissa[mantissaIndex] == 0 )
                {
                    maxLength--;
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            for( mantissaIndex = maxLength - 2;
                 mantissaIndex > 0;
                 mantissaIndex-- )
            {
                if( aValue->mantissa[mantissaIndex] == 99 )
                {
                    maxLength--;
                }
                else
                {
                    break;
                }
            }
        }

/*
  양 양
  바로우 발생 -> -1
  바로우 없음 -> 0

  음 음
  바로우 -> -1
  바로우 없음 -> 0


  양음
  바로우 -> -1
  바로우 없음 -> -1


  음양
  바로우 -> 0 ,98 더해줌
  바로우 없음 -> 0

  //뒤에 0이 연속해서 올경우

  */
        //BUGBUG
        //precision계산(유효숫자 , length와 연관해서)
        //precision자리에서 반올림해준다.

        // To fix BUG-12970
        // 최대 길이가 넘어가는 경우 보정해줘야 함.
        if( maxLength > 21 )
        {
            aValue->length = 21;
        }
        else
        {
            aValue->length = maxLength;
        }

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_OVERFLOW );
    IDE_SET(ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *    IDE_RC mtc::multiplyFloat()
 *
 * Argument :
 *    aValue - point of value , 계산 되어지는 값
 *    aArgument1
 *    aArgument2 - 입력값 aValue = aArgument1 * aArgument2
 *    aPrecisionMaximum - 자리수
 *
 * Description :
 *    사용 algorithm : grade school multiplication( 일명 필산법)
 *
 *    1. exponent계산
 *    2. 자리수대로 곱셈 ; 음수일경우 99의 보수로 계산
 *    2.1 속도를 빠르게 하기 위해 양*양, 음*음, 음*양 인 경우를 따로 만든다
 *    3. 한자리씩 곱해서 더해준다.
 *    4. 캐리 계산은 맨 마지막에 한다.(더할때마다 하게되면 carry가 cascade될 경우 비효율적임)
 *    5. maximum mantissa(20)를 넘어가면 버림
 *    6. exponent는 -63이하면 0, 63이상이면 overflow
 *    7. 음수면 99를 빼서 저장
 * ---------------------------------------------------------------------------*/
IDE_RC mtc::multiplyFloat( mtdNumericType *aValue,
                           UInt            /* aPrecisionMaximum */,
                           mtdNumericType *aArgument1,
                           mtdNumericType *aArgument2 )
{
    SInt sResultMantissa[40];   // 결과는 최대 80자리까지 나올 수 있다.
    SInt i;                     // for multiplication
    SInt j;                     // for multiplication
    SInt sCarry;
    SInt sResultFence;
    SShort sArgExponent1;
    SShort sArgExponent2;
    SShort sResultExponent;
    SShort sMantissaStart;

    // 하나라도 null인 경우 null처리
    if( aArgument1->length == 0 || aArgument2->length == 0 )
    {
        aValue->length = 0;
        aValue->signExponent = 0;
        return IDE_SUCCESS;
    }
    // 하나라도 0인 경우 0으로
    else if( aArgument1->length == 1 || aArgument2->length == 1 )
    {
        aValue->length       = 1;
        aValue->signExponent = 0x80;
        return IDE_SUCCESS;
    }

    // result mantissa최대 길이 설정
    sResultFence = aArgument1->length + aArgument2->length - 2;
    // result mantissa를 0으로 초기화
    idlOS::memset( sResultMantissa, 0, ID_SIZEOF(UInt) * sResultFence );


    // 양수인지 음수인지 부분만 추출
    sArgExponent1 = aArgument1->signExponent & 0x80;
    sArgExponent2 = aArgument2->signExponent & 0x80;

    //exponent 구하기
    if( sArgExponent1 == 0x80 )
    {
        sResultExponent = (aArgument1->signExponent & 0x7F) - 64;
    }
    else
    {
        sResultExponent = 64 - (aArgument1->signExponent & 0x7F);
    }

    if( sArgExponent2 == 0x80 )
    {
        sResultExponent += (aArgument2->signExponent & 0x7F) - 64;
    }
    else
    {
        sResultExponent += 64 - (aArgument2->signExponent & 0x7F);
    }

    // if문 비교를 한번만 하기 위해 모든 경우에 대해 계산
    // 양 * 양
    if( (sArgExponent1 != 0) && (sArgExponent2 != 0) )
    {
        for(i = aArgument1->length-2; i >= 0; i--)
        {
            for(j = aArgument2->length-2; j >= 0; j--)
            {
                sResultMantissa[i + j + 1] +=
                    (SInt)aArgument1->mantissa[i] *
                    (SInt)aArgument2->mantissa[j];
            }
        }
    }
    // 양 * 음
    else if( (sArgExponent1 != 0) /*&& (sArgExponent2 == 0)*/ )
    {
        for(i = aArgument1->length-2; i >= 0; i--)
        {
            for(j = aArgument2->length-2; j >= 0; j--)
            {
                sResultMantissa[i + j + 1] +=
                    (SInt)aArgument1->mantissa[i] *
                    (SInt)( 99 - aArgument2->mantissa[j] );
            }
        }
    }
    // 음 * 양
    else if( /*(sArgExponent1 == 0) &&*/ (sArgExponent2 != 0) )
    {
        for(i = aArgument1->length-2; i >= 0; i--)
        {
            for(j = aArgument2->length-2; j >= 0; j--)
            {
                sResultMantissa[i + j + 1] +=
                    (SInt)( 99 - aArgument1->mantissa[i] ) *
                    (SInt)aArgument2->mantissa[j];
            }
        }
    }
    // 음 * 음
    else
    {
        for(i = aArgument1->length-2; i >= 0; i--)
        {
            for(j = aArgument2->length-2; j >= 0; j--)
            {
                sResultMantissa[i + j + 1] +=
                    (SInt)( 99 - aArgument1->mantissa[i] ) *
                    (SInt)( 99 - aArgument2->mantissa[j] );
            }
        }
    }

    // carry계산
    for( i = sResultFence - 1; i > 0; i-- )
    {
        sCarry = sResultMantissa[i] / 100;
        sResultMantissa[i-1] += sCarry;
        sResultMantissa[i] -= sCarry * 100;
    }

    // 앞 Zero trim
    if( sResultMantissa[0] == 0 )
    {
        sMantissaStart = 1;
        sResultExponent--;
    }
    else
    {
        sMantissaStart = 0;
    }

    // 뒤 Zero trim
    for( i = sResultFence - 1; i >= 0; i-- )
    {
        if( sResultMantissa[i] == 0 )
        {
            sResultFence--;
        }
        else
        {
            break;
        }
    }

    if( ( sResultFence - sMantissaStart ) > 20 )
    {
        sResultFence = 20 + sMantissaStart;
    }
    else
    {
        // Nothing to do.
    }

    // exponent가 63을 넘으면 에러(최대 126이므로)
    IDE_TEST_RAISE( sResultExponent > 63, ERR_VALUE_OVERFLOW );

    // exponent가 -63(-126)보다 작으면 0이 됨
    if( sResultExponent < -63 )
    {
        aValue->length       = 1;
        aValue->signExponent = 0x80;
    }
    else
    {
        // 실제 mantissa에 값 복사
        // 부호가 같은 경우
        if( sArgExponent1 == sArgExponent2 )
        {
            aValue->signExponent = sResultExponent + 192;
            for( i = 0, j = sMantissaStart; j < sResultFence; i++, j++ )
            {
                aValue->mantissa[i] = sResultMantissa[j];
            }
            aValue->length = sResultFence - sMantissaStart + 1;
        }
        // 부호가 다른 경우
        else
        {
            aValue->signExponent = 64 - sResultExponent;
            for( i = 0, j = sMantissaStart; j < sResultFence; i++, j++ )
            {
                aValue->mantissa[i] = 99 - sResultMantissa[j];
            }
            aValue->length = sResultFence - sMantissaStart + 1;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *    IDE_RC mtc::divideFloat()
 *
 * Argument :
 *    aValue - point of value , 계산 되어지는 값
 *    aArgument1
 *    aArgument2 - 입력값 aValue = aArgument1 / aArgument2
 *    aPrecisionMaximum - 자리수
 *
 * Description :
 *    사용 algorithm : David M. Smith의 논문
 *                    'A Multiple-Precision Division Algorithm'
 *
 *    1. exponent계산
 *    2. 피제수, 제수를 integer mantissa array에 저장.(모두 양수로 normalization)
 *    2.1 피제수는 한칸 오른쪽 쉬프트해서 저장.(남은 빈칸에는 몫이 저장된다)
 *    3. 나눗셈 몫을 추측하기 위한 값 저장
 *    3.1 피제수, 제수에서 각각 4자리씩 빼서 integer값에 저장
 *    3.2 추측값을 구함
 *    4. 피제수 = 피제수 - 제수 * 추측값(보수연산 없이 그냥 뺌)
 *    5. 몫이 maximum mantissa + 1이 될때까지 3~4 반복
 *    6. 몫을 normalization
 *
 * ---------------------------------------------------------------------------*/
IDE_RC mtc::divideFloat( mtdNumericType *aValue,
                         UInt            /* aPrecisionMaximum*/,
                         mtdNumericType *aArgument1,
                         mtdNumericType *aArgument2 )

{
    SInt  sMantissa1[42];        // 피제수 및 몫 저장.
    UChar sMantissa2[20] = {0,}; // 제수 저장.(음수라면)
    SInt  sMantissa3[21];        // 제수 * 부분몫. 추측값이 큰 경우 대비.(최대 1byte)

    SInt sNumerator;           // 피제수 샘플값.
    SInt sDenominator;         // 제수 샘플값.
    SInt sPartialQ;            // 부분몫.

    SInt i;                    // for Iterator
    SInt j;                    // for Iterator
    SInt k;                    // for Iterator

    SInt sResultFence;

    SInt sStep;
    SInt sLastStep;

    SShort sArgExponent1;
    SShort sArgExponent2;
    SShort sResultExponent;

    UChar * sMantissa2Ptr;
    SInt    sCarry;
    SInt    sMantissaStart;

    // 하나라도 null이면 null
    if( aArgument1->length == 0 || aArgument2->length == 0 )
    {
        aValue->length = 0;
        return IDE_SUCCESS;
    }
    // 0으로 나눈 경우
    else if ( aArgument2->length == 1 )
    {
        IDE_RAISE(ERR_DIVIDE_BY_ZERO);
    }
    // 0을 나눈 경우
    else if ( aArgument1->length == 1 )
    {
        aValue->length = 1;
        aValue->signExponent = 0x80;
        return IDE_SUCCESS;
    }

    // 양수인지 음수인지 부분만 추출
    sArgExponent1 = aArgument1->signExponent & 0x80;
    sArgExponent2 = aArgument2->signExponent & 0x80;


    // 피제수 복사.
    // 무조건 양수 형태로 바꾸어 mantissa복사
    sMantissa1[0] = 0;

    if( sArgExponent1 == 0x80 )
    {
        // 피제수는 한칸비우고 복사(0번째 칸부터 몫이 저장될것임)
        for( i = 0; i < aArgument1->length - 1; i++ )
        {
            sMantissa1[i+1] = (SInt)aArgument1->mantissa[i];
        }
        idlOS::memset( &sMantissa1[i+1], 0, sizeof(SInt) * (42 - i - 1) );
    }
    else
    {
        // 피제수는 한칸비우고 복사(0번째 칸부터 몫이 저장될것임)
        for( i = 0; i < aArgument1->length - 1; i++ )
        {
            sMantissa1[i+1] = 99 - (SInt)aArgument1->mantissa[i];
        }
        idlOS::memset( &sMantissa1[i+1], 0, sizeof(SInt) * (42 - i - 1) );
    }

    // 제수 복사.
    // 제수는 양수일때는 복사안함.
    if( sArgExponent2 == 0x80 )
    {
        // mantissaptr이 직접 aArgument2->mantissa를 가리킴.
        sMantissa2Ptr = aArgument2->mantissa;
    }
    else
    {
        for( i = 0; i < aArgument2->length - 1; i++ )
        {
            sMantissa2[i] = 99 - (SInt)aArgument2->mantissa[i];
        }
        sMantissa2Ptr = sMantissa2;
    }

    //exponent 구하기
    if( sArgExponent1 == 0x80 )
    {
        sResultExponent = (aArgument1->signExponent & 0x7F) - 64;
    }
    else
    {
        sResultExponent = 64 - (aArgument1->signExponent & 0x7F);
    }

    if( sArgExponent2 == 0x80 )
    {
        sResultExponent -= (aArgument2->signExponent & 0x7F) - 64 - 1;
    }
    else
    {
        sResultExponent -= 64 - (aArgument2->signExponent & 0x7F) - 1;
    }

    // 제수 샘플값 구하기(최대 3자리까지 샘플값을 구하여 오차를 최소화 한다.)
    if( aArgument2->length > 3 )
    {
        sDenominator = (SInt)sMantissa2Ptr[0] * 10000 +
            (SInt)sMantissa2Ptr[1] * 100 +
            (SInt)sMantissa2Ptr[2];
    }
    else if ( aArgument2->length > 2 )
    {
        sDenominator = (SInt)sMantissa2Ptr[0] * 10000 +
            (SInt)sMantissa2Ptr[1] * 100;
    }
    else
    {
        sDenominator = (SInt)sMantissa2Ptr[0] * 10000;
    }
    IDE_TEST_RAISE(sDenominator == 0, ERR_DIVIDE_BY_ZERO);

    sLastStep = 20 + 2; // 최대 21자리까지 나올 수 있도록

    for( sStep = 1; sStep < sLastStep; sStep++ )
    {
        // 피제수 샘플값 구하기(피제수의 자리수는 최대 42자리이기 때문에 abr이 날 수 없음)
        sNumerator = sMantissa1[sStep] * 1000000 +
            sMantissa1[sStep+1] * 10000 +
            sMantissa1[sStep+2] * 100 +
            sMantissa1[sStep+3];

        // 부분몫을 구함.
        sPartialQ = sNumerator / sDenominator;

        // 부분몫이 0이 아니라면 피제수 - (제수*부분몫)
        if( sPartialQ != 0 )
        {
            // sPartialQ * sMantissa2한 것을 sMantissa1에서 뺀다.
            // 자리수를 맞춰서 빼야 함. 보수연산 뺄셈 사용X
            // sPartialQ * sMantissa2
            sMantissa3[0] = 0;

            // 제수*부분몫
            for(k = aArgument2->length-2; k >= 0; k--)
            {
                sMantissa3[k+1] = (SInt)sMantissa2Ptr[k] * sPartialQ;
            }

            sCarry = 0;

            /** BUG-42773
             * 99.999 / 111.112 를 나누게 되면 sPartialQ = 9000 인데
             * sMantissa3에 sCaary를 하면 100진법을 넘게되는데 이에 관련된
             * 처리가 없었다.
             * 즉 100 진법으로 carry은 배열의 끝에서 부터 0 로 carry를 하는데
             * 마지막에 100 진법이 넘어서 carry값이  1 이 남았는데
             * 이와 관련된 처리가 없어서 무시되면서 계산이 꼬이게 된다.
             * 따라서 마지막 0는 carry하지 않고 그대로 처리한다.
             */
            // 곱셈이후의 carry계산
            for ( k = aArgument2->length - 1 ; k > 0; k-- )
            {
                sMantissa3[k] += sCarry;
                sCarry = sMantissa3[k] / 100;
                sMantissa3[k] -= sCarry * 100;
            }
            sMantissa3[0] += sCarry;

            // 피제수 - (제수*부분몫)
            for( k = 0; k < aArgument2->length; k++ )
            {
                sMantissa1[sStep + k] -=  sMantissa3[k];
            }

            sMantissa1[sStep+1] +=sMantissa1[sStep]*100;
            sMantissa1[sStep] = sPartialQ;
        }
        // 부분몫이 0이라면 몫은 그냥 0
        else
        {
            sMantissa1[sStep+1] +=sMantissa1[sStep]*100;
            sMantissa1[sStep] = sPartialQ;
        }
    }

    // Final Normalization
    // carry및 잘못된 값을 normalization한다.
    // 22자리까지 계산해 놓았으므로 resultFence는 21임.
    sResultFence = 21;

    for( i = sResultFence; i > 0; i-- )
    {
        if( sMantissa1[i] >= 100 )
        {
            sCarry = sMantissa1[i] / 100;
            sMantissa1[i-1] += sCarry;
            sMantissa1[i] -= sCarry * 100;
        }
        else if ( sMantissa1[i] < 0 )
        {
            // carry가 100단위로 떨어지는 것 처리를 위해
            // +1을 한 값에서 /100을 하고 여기에 -1을 함
            // ex) -100 => carry는 -1, -101 => carry는 -2
            //     -99  => carry는 -1
            sCarry = ((sMantissa1[i] + 1) / 100) - 1;
            sMantissa1[i-1] += sCarry;
            sMantissa1[i] -= sCarry * 100;
        }
        else
        {
            // Nothing to do.
        }
    }

    // mantissa1[0]이 0인 경우 resultExponent가 1 줄어듬.
    // ex) 나누어질 첫번째 자리부터 제수가 피제수보다 큰 경우임.
    //     1234 / 23 -> 첫째자리 나눌수 없어 0임
    if( sMantissa1[0] == 0 )
    {
        sMantissaStart = 1;
        sResultExponent--;
    }
    // 0이 아닌 경우 애초 예상했던 자리보다 한자리가 많으므로 한자리 감소
    else
    {
        sMantissaStart = 0;
        sResultFence--;
    }

    // 뒤 Zero Trim
    for( i = sResultFence - 1; i > 0; i-- )
    {
        if( sMantissa1[i] == 0 )
        {
            sResultFence--;
        }
        else
        {
            break;
        }
    }

    if( ( sResultFence - sMantissaStart ) > 20 )
    {
        sResultFence = 20 + sMantissaStart;
    }
    else
    {
        // Nothing to do.
    }

    // exponent가 63을 넘으면 에러(최대 126이므로)
    IDE_TEST_RAISE( sResultExponent > 63, ERR_VALUE_OVERFLOW );

    // exponent가 -63(-126)보다 작으면 0이 됨
    if( sResultExponent < -63 )
    {
        aValue->length       = 1;
        aValue->signExponent = 0x80;
    }
    else
    {
        // 부호가 같은 경우
        if( sArgExponent1 == sArgExponent2 )
        {
            aValue->signExponent = sResultExponent + 192;
            for( i = 0, j = sMantissaStart; j < sResultFence; i++, j++ )
            {
                aValue->mantissa[i] = sMantissa1[j];
            }
            aValue->length = sResultFence - sMantissaStart + 1;
        }
        // 부호가 다른 경우
        else
        {
            aValue->signExponent = 64 - sResultExponent;
            for( i = 0, j = sMantissaStart; j < sResultFence; i++, j++ )
            {
                aValue->mantissa[i] = 99 - sMantissa1[j];
            }
            aValue->length = sResultFence - sMantissaStart + 1;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DIVIDE_BY_ZERO );
    {
        IDE_SET(ideSetErrorCode( mtERR_ABORT_DIVIDE_BY_ZERO ));
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * PROJ-1517
 * gets the modula of aArgument1 divided by aArgument2, stores the result in aValue
 */
IDE_RC mtc::modFloat( mtdNumericType *aValue,
                      UInt            /* aPrecisionMaximum*/,
                      mtdNumericType *aArgument1,
                      mtdNumericType *aArgument2 )
{
    SShort  sExponent1;
    SShort  sExponent2;
    UChar   sMantissa1[MTD_FLOAT_MANTISSA_MAXIMUM + 1] = { 0, };
    UChar   sMantissa2[MTD_FLOAT_MANTISSA_MAXIMUM + 1] = { 0, };
    UChar   sMantissa3[MTD_FLOAT_MANTISSA_MAXIMUM + 1] = { 0, };
    UChar   sMantissa4[MTD_FLOAT_MANTISSA_MAXIMUM] = { 0, };
    UShort  sMininum;
    UShort  sBorrow;
    SInt    i;                    // for Iterator
    SInt    j;                    // for Iterator
    SShort  sArgExponent1;
    SShort  sArgExponent2;
    SInt    sMantissaStart;

    /* 하나라도 null이면 null */
    if ( ( aArgument1->length == 0 ) || ( aArgument2->length == 0 ) )
    {
        aValue->length = 0;
        IDE_CONT( NORMAL_EXIT );
    }
    /* 0으로 나눈 경우 */
    else if ( aArgument2->length == 1 )
    {
        IDE_RAISE(ERR_DIVIDE_BY_ZERO);
    }
    /* 0을 나눈 경우 */
    else if ( aArgument1->length == 1 )
    {
        aValue->length = 1;
        aValue->signExponent = 0x80;
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    /* 양수인지 음수인지 부분만 추출 */
    sArgExponent1 = aArgument1->signExponent & 0x80;
    sArgExponent2 = aArgument2->signExponent & 0x80;

    /* exponent 구하기 */
    if ( sArgExponent1 == 0x80 )
    {
        sExponent1 = (aArgument1->signExponent & 0x7F) - 64;
    }
    else
    {
        sExponent1 = 64 - (aArgument1->signExponent & 0x7F);
    }

    if ( sArgExponent2 == 0x80 )
    {
        sExponent2 = (aArgument2->signExponent & 0x7F) - 64;
    }
    else
    {
        sExponent2 = 64 - (aArgument2->signExponent & 0x7F);
    }

    /* 피제수 복사. 무조건 양수 형태로 바꾸어 mantissa복사 */
    sMantissa1[0] = sMantissa2[0] = 0;

    if ( sArgExponent1 == 0x80 )
    {
        for ( i = 0; i < aArgument1->length - 1; i++ )
        {
            sMantissa1[i + 1] = aArgument1->mantissa[i];
        }
    }
    else
    {
        for ( i = 0; i < aArgument1->length - 1; i++ )
        {
            sMantissa1[i + 1] = 99 - aArgument1->mantissa[i];
        }
    }

    /* 제수 복사. */
    if ( sArgExponent2 == 0x80 )
    {
        for ( i = 0; i < aArgument2->length - 1; i++ )
        {
            sMantissa2[i + 1] = aArgument2->mantissa[i];
        }
    }
    else
    {
        for ( i = 0; i < aArgument2->length - 1; i++ )
        {
            sMantissa2[i + 1] = 99 - aArgument2->mantissa[i];
        }
    }

    while ( sExponent1 >= sExponent2 )
    {
        while ( ( sMantissa1[0] != 0 ) || ( sMantissa1[1] >= sMantissa2[1] ) )
        {
            sMininum = ( sMantissa1[0] * 100 + sMantissa1[1] ) / ( sMantissa2[1] + 1 );
            if ( sMininum )
            {
                sMantissa3[0] = 0;
                for ( j = 1; j < (SInt)ID_SIZEOF(sMantissa3); j++ )
                {
                    sMantissa3[j - 1] += ((UShort)sMantissa2[j]) * sMininum / 100;
                    sMantissa3[j]      = ((UShort)sMantissa2[j]) * sMininum % 100;
                }

                for ( j = ID_SIZEOF(sMantissa3) - 1; j > 0; j-- )
                {
                    if ( sMantissa3[j] >= 100 )
                    {
                        sMantissa3[j]     -= 100;
                        sMantissa3[j - 1] += 1;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }

                sBorrow = 0;
                for ( j = ID_SIZEOF(sMantissa3) - 1; j >= 0; j-- )
                {
                    sMantissa3[j] += sBorrow;
                    sBorrow = 0;
                    if ( sMantissa1[j] < sMantissa3[j] )
                    {
                        sMantissa1[j] += 100;
                        sBorrow = 1;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    sMantissa1[j] -= sMantissa3[j];
                }
            }
            else
            {
                if ( idlOS::memcmp( sMantissa1, sMantissa2, ID_SIZEOF(sMantissa2) ) >= 0 )
                {
                    sBorrow = 0;
                    for ( j = ID_SIZEOF(sMantissa3) - 1; j >= 0; j-- )
                    {
                        sMantissa3[j] = sMantissa2[j] + sBorrow;
                        sBorrow = 0;
                        if ( sMantissa1[j] < sMantissa3[j] )
                        {
                            sMantissa1[j] += 100;
                            sBorrow = 1;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                        sMantissa1[j] -= sMantissa3[j];
                    }
                }
                else
                {
                    /* Nothing to do */
                }
                break;
            }
        }

        if ( sExponent1 <= sExponent2 )
        {
            break;
        }
        else
        {
            /* Nothing to do */
        }

        for ( j = 0; j < (SInt)ID_SIZEOF(sMantissa1) - 1; j++ )
        {
            sMantissa1[j] = sMantissa1[j + 1];
        }

        sMantissa1[j] = 0;
        sExponent1--;
    }

    sMantissaStart = ID_SIZEOF(sMantissa1);
    for ( i = 1; i < (SInt)ID_SIZEOF(sMantissa1); i++ )
    {
        if ( sMantissa1[i] != 0 )
        {
            sMantissaStart = i;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    sExponent1 -= sMantissaStart - 1;

    /* exponent가 63을 넘으면 에러(최대 126이므로) */
    IDE_TEST_RAISE( sExponent1 > 63, ERR_VALUE_OVERFLOW );

    /* exponent가 -63(-126)보다 작으면 0이 됨 */
    if ( ( sExponent1 < -63 ) || ( sMantissaStart == ID_SIZEOF(sMantissa1) ) )
    {
        aValue->length       = 1;
        aValue->signExponent = 0x80;
    }
    else
    {
        for ( i = 0, j = sMantissaStart; j < (SInt)ID_SIZEOF(sMantissa1); i++, j++ )
        {
            sMantissa4[i] = sMantissa1[j];
        }
        /* 기존 makenumeric 호환성 */
        for ( i = 0, aValue->length = 1; i < (SInt)ID_SIZEOF(sMantissa4) - 1; i++ )
        {
            if ( sMantissa4[i] != 0 )
            {
                aValue->length = i + 2;
            }
            else
            {
                /* Nothing to do */
            }
        }

        /* 피제수의 부호를 따른다. */
        if ( sArgExponent1 == 0x80 )
        {
            aValue->signExponent = sExponent1 + 192;
            for ( i = 0; i < aValue->length - 1; i++ )
            {
                aValue->mantissa[i] = sMantissa4[i];
            }
        }
        else
        {
            aValue->signExponent = 64 - sExponent1;
            for ( i = 0; i < aValue->length - 1; i++ )
            {
                aValue->mantissa[i] = 99 - sMantissa4[i];
            }
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DIVIDE_BY_ZERO );
    {
        IDE_SET(ideSetErrorCode( mtERR_ABORT_DIVIDE_BY_ZERO ));
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * PROJ-1517
 * gets sign of aArgument1
 */
IDE_RC mtc::signFloat( mtdNumericType *aValue,
                       mtdNumericType *aArgument1 )
{
    SShort sArgExponent1;

    /* null 이면 null */
    if ( aArgument1->length == 0)
    {
        aValue->length = 0;
    }
    /* zero 이면 zero */
    else if ( aArgument1->length == 1 )
    {
        aValue->length       = 1;
        aValue->signExponent = 0x80;
    }
    else
    {
        sArgExponent1 = aArgument1->signExponent & 0x80;

        /* 양수 */
        if ( sArgExponent1 == 0x80 )
        {
            aValue->signExponent = 193;
            aValue->length = 2;
            aValue->mantissa[0] = 1;
        }
        /* 음수 */
        else
        {
            aValue->signExponent = 63;
            aValue->length = 2;
            aValue->mantissa[0] = 98;
        }
    }

    return IDE_SUCCESS;
}

/*
 * PROJ-1517
 * gets absolute value of aArgument1 and store it on aValue
 */
IDE_RC mtc::absFloat( mtdNumericType *aValue,
                      mtdNumericType *aArgument1 )
{
    SShort sArgExponent1;
    SInt   i;

    /* null 이면 null */
    if ( aArgument1->length == 0)
    {
        aValue->length = 0;
    }
    else
    {
        sArgExponent1 = aArgument1->signExponent & 0x80;

        /* 양수 */
        if ( sArgExponent1 == 0x80 )
        {
            aValue->length = aArgument1->length;
            aValue->signExponent = aArgument1->signExponent;
            for ( i = 0; i < aArgument1->length - 1; i++ )
            {
                aValue->mantissa[i] = aArgument1->mantissa[i];
            }
        }
        /* 음수 */
        else
        {
            aValue->length = aArgument1->length;
            aValue->signExponent = (64 - (aArgument1->signExponent & 0x7F)) + 192;
            for ( i = 0; i < aArgument1->length - 1; i++ )
            {
                aValue->mantissa[i] = 99 - aArgument1->mantissa[i];
            }
        }
    }

    return IDE_SUCCESS;
}
/*
 * PROJ-1517
 * gets the smallest integral value not less than aArgument1 and store it on aValue
 */
IDE_RC mtc::ceilFloat( mtdNumericType *aValue,
                       mtdNumericType *aArgument1 )
{
    SShort sExponent;
    UShort sCarry;
    idBool sHasFraction;
    SInt   i;
    SShort sArgExponent1;
    UChar  sMantissa[MTD_FLOAT_MANTISSA_MAXIMUM] = { 0, };

    /* null 이면 null */
    if ( aArgument1->length == 0 )
    {
        aValue->length = 0;
    }
    else if ( aArgument1->length == 1 )
    {
        aValue->length = 1;
        aValue->mantissa[0] = 0;
        aValue->signExponent = 0x80;
    }
    else
    {
        aValue->signExponent = aArgument1->signExponent;
        aValue->length = aArgument1->length;
        for ( i = 0; i < aArgument1->length - 1; i++ )
        {
            sMantissa[i] = aArgument1->mantissa[i];
            aValue->mantissa[i] = aArgument1->mantissa[i];
        }

        sArgExponent1 = aArgument1->signExponent & 0x80;

        /* 양수 */
        if ( sArgExponent1 == 0x80 )
        {
            sExponent = (aArgument1->signExponent & 0x7F) - 64;

            if ( sExponent < 0)
            {
                sExponent = 1;
                aValue->mantissa[0] = 1;
                aValue->length = 2;
                aValue->signExponent = sExponent + 192;
            }
            else
            {
                if ( sExponent < MTD_FLOAT_MANTISSA_MAXIMUM )
                {
                    sHasFraction = ID_FALSE;

                    for ( i = sExponent; i < MTD_FLOAT_MANTISSA_MAXIMUM; i++ )
                    {
                        if ( sMantissa[i] > 0 )
                        {
                            sHasFraction = ID_TRUE;
                            break;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }

                    if ( sHasFraction == ID_TRUE )
                    {
                        sCarry = 1;
                        for ( i = sExponent - 1; i >= 0; i-- )
                        {
                            sMantissa[i] += sCarry;
                            sCarry = 0;
                            if ( sMantissa[i] >= 100 )
                            {
                                sMantissa[i] -= 100;
                                sCarry = 1;
                            }
                            else
                            {
                                /* Nothing to do */
                            }
                        }
                        if ( sCarry != 0 )
                        {
                            for ( i = sExponent; i > 0; i-- )
                            {
                                sMantissa[i] = sMantissa[i - 1];
                            }
                            sMantissa[0] = 1;
                            sExponent++;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }
                    else
                    {
                        /* No Fraction */
                    }

                    if ( sExponent < aValue->length )
                    {
                        for ( i = sExponent; i < MTD_FLOAT_MANTISSA_MAXIMUM; i++ )
                        {
                            sMantissa[i] = 0;
                        }

                        aValue->length = sExponent + 1;
                        aValue->signExponent = sExponent + 192;
                        for ( i = 0; i < aValue->length - 1; i++ )
                        {
                            aValue->mantissa[i] = sMantissa[i];
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
        /* 음수 */
        else
        {
            sExponent = 64 - (aArgument1->signExponent & 0x7F);

            if ( sExponent <= 0 )
            {
                aValue->length       = 1;
                aValue->signExponent = 0x80;
            }
            else
            {
                if ( sExponent < aValue->length )
                {
                    for ( i = sExponent; i < MTD_FLOAT_MANTISSA_MAXIMUM; i++ )
                    {
                        sMantissa[i] = 99;
                    }

                    aValue->length = sExponent + 1;
                    aValue->signExponent = 64 - sExponent;
                    for ( i = 0; i < aValue->length - 1; i++ )
                    {
                        aValue->mantissa[i] = sMantissa[i];
                    }
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
    }

    return IDE_SUCCESS;
}

/*
 * PROJ-1517
 * gets the largest integral value not greater than aArgument1 and store it on aValue
 */
IDE_RC mtc::floorFloat( mtdNumericType *aValue,
                        mtdNumericType *aArgument1 )
{
    SShort sExponent;
    UShort sCarry;
    SInt   i;
    SShort sArgExponent1;
    UChar  sMantissa[MTD_FLOAT_MANTISSA_MAXIMUM] = { 0, };

    /* null 이면 null */
    if ( aArgument1->length == 0 )
    {
        aValue->length = 0;
    }
    else if ( aArgument1->length == 1 )
    {
        aValue->length = 1;
        aValue->mantissa[0]= 0;
        aValue->signExponent = 0x80;
    }
    else
    {
        aValue->signExponent = aArgument1->signExponent;
        aValue->length = aArgument1->length;
        sArgExponent1 = aArgument1->signExponent & 0x80;
        if ( sArgExponent1 == 0x80 )
        {
            for ( i = 0; i < aArgument1->length - 1; i++ )
            {
                sMantissa[i] = aArgument1->mantissa[i];
                aValue->mantissa[i] = aArgument1->mantissa[i];
            }
        }
        else
        {
            for ( i = 0; i < aArgument1->length - 1; i++ )
            {
                sMantissa[i] = 99 - aArgument1->mantissa[i];
                aValue->mantissa[i] = aArgument1->mantissa[i];
            }
        }

        /* 양수 */
        if ( sArgExponent1 == 0x80 )
        {
            sExponent = (aArgument1->signExponent & 0x7F) - 64;

            if ( sExponent <= 0 )
            {
                aValue->length       = 1;
                aValue->signExponent = 0x80;
            }
            else
            {
                /* BUG-42098 in floor function, zero trim of numeric type is needed */
                if ( sExponent < aValue->length )
                {
                    for ( i = sExponent; i < MTD_FLOAT_MANTISSA_MAXIMUM; i++ )
                    {
                        sMantissa[i] = 0;
                    }
                    aValue->length = sExponent + 1;
                    aValue->signExponent = sExponent + 192;
                    for ( i = 0; i < aValue->length - 1; ++i )
                    {
                        aValue->mantissa[i] = sMantissa[i];
                    }
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
        /* 음수 */
        else
        {
            sExponent = 64 - (aArgument1->signExponent & 0x7F);

            if ( sExponent < 0)
            {
                sExponent = 1;
                aValue->mantissa[0] = 98;
                aValue->length = 2;
                aValue->signExponent = 64 - sExponent;
            }
            else
            {
                if ( sExponent < MTD_FLOAT_MANTISSA_MAXIMUM )
                {
                    if ( sMantissa[sExponent] > 0 )
                    {
                        sCarry = 1;
                        for ( i = sExponent - 1; i >= 0; i--)
                        {
                            sMantissa[i] += sCarry;
                            sCarry = 0;
                            if ( sMantissa[i] >= 100 )
                            {
                                sMantissa[i] -= 100;
                                sCarry = 1;
                            }
                            else
                            {
                                /* nothing to do. */
                            }
                        }

                        if ( sCarry != 0 )
                        {
                            for ( i = sExponent; i > 0; i-- )
                            {
                                sMantissa[i] = sMantissa[i - 1];
                            }
                            sMantissa[0] = 1;
                            sExponent++;
                        }
                        else
                        {
                            /* nothing to do. */
                        }
                    }
                    else
                    {
                        /* nothing to do. */
                    }

                    /* BUG-42098 in floor function, zero trim of numeric type is needed */
                    if ( sExponent < aValue->length )
                    {
                        for ( i = sExponent; i < MTD_FLOAT_MANTISSA_MAXIMUM; i++ )
                        {
                            sMantissa[i] = 0;
                        }

                        aValue->length = sExponent + 1;
                        aValue->signExponent = 64 - sExponent;
                        for ( i = 0; i < aValue->length - 1; i++ )
                        {
                            aValue->mantissa[i] = 99 - sMantissa[i];
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
    }/* else */

    return IDE_SUCCESS;
}

/*
 * PROJ-1517
 * converts mtdNumericType to SLong
 */
IDE_RC mtc::numeric2Slong( SLong          *aValue,
                           mtdNumericType *aArgument1 )
{
    ULong  sULongValue = 0;
    SShort sArgExponent1;
    SShort sExponent;
    UChar  sMantissa[MTD_FLOAT_MANTISSA_MAXIMUM] = { 0, };
    SInt   i;

    static const ULong sPower[] = { ID_ULONG(1),
                                    ID_ULONG(100),
                                    ID_ULONG(10000),
                                    ID_ULONG(1000000),
                                    ID_ULONG(100000000),
                                    ID_ULONG(10000000000),
                                    ID_ULONG(1000000000000),
                                    ID_ULONG(100000000000000),
                                    ID_ULONG(10000000000000000),
                                    ID_ULONG(1000000000000000000) };
    /* null 이면 에러 */
    IDE_TEST_RAISE( aArgument1->length == 0, ERR_NULL_VALUE );

    if ( aArgument1->length == 1 )
    {
        *aValue = 0;
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* nothing to do. */
    }

    sArgExponent1 = aArgument1->signExponent & 0x80;

    /* 양수이면 그대로 음수이면 양수로 변환 */
    if ( sArgExponent1 == 0x80 )
    {
        sExponent = (aArgument1->signExponent & 0x7F) - 64;

        for ( i = 0; i < aArgument1->length - 1; i++ )
        {
            sMantissa[i] = aArgument1->mantissa[i];
        }
    }
    else
    {
        sExponent = 64 - (aArgument1->signExponent & 0x7F);

        for ( i = 0; i < aArgument1->length - 1; i++ )
        {
            sMantissa[i] = 99 - aArgument1->mantissa[i];
        }
    }

    if ( sExponent > 0 )
    {
        IDE_TEST_RAISE( (UShort)sExponent
                        > ( ID_SIZEOF(sPower) / ID_SIZEOF(sPower[0]) ),
                        ERR_OVERFLOW );
        for ( i = 0; i < sExponent; i++ )
        {
            sULongValue += sMantissa[i] * sPower[sExponent - i - 1];
        }
    }
    else
    {
        /* nothing to do. */
    }

    //양수
    if ( sArgExponent1 == 0x80 )
    {
        IDE_TEST_RAISE( sULongValue > ID_ULONG(9223372036854775807),
                        ERR_OVERFLOW );
        *aValue = sULongValue;
    }
    //음수
    else
    {
        IDE_TEST_RAISE( sULongValue > ID_ULONG(9223372036854775808),
                        ERR_OVERFLOW );

        if ( sULongValue == ID_ULONG(9223372036854775808) )
        {
            *aValue = ID_LONG(-9223372036854775807) - ID_LONG(1);
        }
        else
        {
            *aValue = -(SLong)sULongValue;
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_OVERFLOW );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }

    IDE_EXCEPTION( ERR_NULL_VALUE );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_NULL_VALUE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * PROJ-1517
 * converts mtdNumericType to SDouble (mtdDoubleType)
 */
void mtc::numeric2Double( mtdDoubleType  * aValue,
                          mtdNumericType * aArgument1 )
{
    UChar     sMantissaLen;
    SShort    sScale;
    SShort    sExponent;
    UChar     i;
    SShort    sArgExponent1;
    UChar   * sMantissa;
    SDouble   sDoubleValue = 0.0;

    sMantissaLen = aArgument1->length - 1;

    sArgExponent1        = ( aArgument1->signExponent & 0x80 );
    sScale               = ( aArgument1->signExponent & 0x7F );

    if ( (sScale != 0) && (sMantissaLen > 0) )
    {
        if ( sArgExponent1 == 0x80 )
        {
            for ( i = 0, sMantissa = aArgument1->mantissa;
                  i < sMantissaLen;
                  i++, sMantissa++ )
            {
                sDoubleValue = sDoubleValue * 100.0 + *sMantissa;
            }

            sExponent = sScale - 64 - sMantissaLen;
        }
        else
        {
            for ( i = 0, sMantissa = aArgument1->mantissa;
                  i < sMantissaLen;
                  i++, sMantissa++ )
            {
                sDoubleValue = sDoubleValue * 100.0 + (99 - *sMantissa);
            }
            sExponent = 64 - sScale - sMantissaLen;
        }

        if (sExponent >= 0)
        {
            sDoubleValue = sDoubleValue * gDoublePow[sExponent];
        }
        else
        {
            if (sExponent < -63)
            {
                sDoubleValue = 0.0;
            }
            else
            {
                sDoubleValue = sDoubleValue / gDoublePow[-sExponent];
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    if (sArgExponent1 == 0x80)
    {
        *aValue = sDoubleValue;
    }
    else
    {
        *aValue = -sDoubleValue;
    }
}

// BUG-41194 double to numeric 변환 성능개선
IDE_RC mtc::double2Numeric( mtdNumericType *aValue,
                            mtdDoubleType   aArgument1 )
{
    SLong    sLongValue;
    SShort   sExponent2;  // 2^e
    SChar    sSign;
    UChar  * sDoubles   = (UChar*) &aArgument1;
    UChar  * sExponents = (UChar*) &sExponent2;
    UChar  * sFractions = (UChar*) &sLongValue;
    UChar    sMantissa[MTD_FLOAT_MANTISSA_MAXIMUM];
    SShort   sExponent10;   // 10^e
    SShort   sExponent100;  // 100^e
    SShort   sExponent;
    SShort   sLength;
    SShort   i;

    IDE_TEST_RAISE( gIEEE754Double == ID_FALSE,
                    ERR_NOT_APPLICABLE );
    
    //------------------------------------------
    // decompose double
    //------------------------------------------
    
    // IEEE 754-1985 표준에 맞춰 double type을 분리한다.
    // 64 bit = 1 bit sign + 11 bit exponent + 52 bit fraction
    
#if defined(ENDIAN_IS_BIG_ENDIAN)
    
    // sign
    sSign = sDoubles[0] & 0x80;
    sSign = (sSign >> 7) & 0x01;  // 0 혹은 1

    // exponent
    sExponents[0] = sDoubles[0] & 0x7f;
    sExponents[1] = sDoubles[1] & 0xf0;
    sExponent2 = (sExponent2 >> 4) & 0x0fff;  // 0 ~ 2047

    // fraction
    sFractions[0] = 0;
    sFractions[1] = (sDoubles[1] & 0x0f) | 0x10;  // 앞에 1추가
    sFractions[2] = sDoubles[2];
    sFractions[3] = sDoubles[3];
    sFractions[4] = sDoubles[4];
    sFractions[5] = sDoubles[5];
    sFractions[6] = sDoubles[6];
    sFractions[7] = sDoubles[7];

#else

    // sign
    sSign = sDoubles[7] & 0x80;
    sSign = (sSign >> 7) & 0x01;  // 0 혹은 1
    
    // exponent
    sExponents[0] = sDoubles[6] & 0xf0;
    sExponents[1] = sDoubles[7] & 0x7f;
    sExponent2 = (sExponent2 >> 4) & 0x0fff;  // 0 ~ 2047

    // fraction
    sFractions[0] = sDoubles[0];
    sFractions[1] = sDoubles[1];
    sFractions[2] = sDoubles[2];
    sFractions[3] = sDoubles[3];
    sFractions[4] = sDoubles[4];
    sFractions[5] = sDoubles[5];
    sFractions[6] = (sDoubles[6] & 0x0f) | 0x10;  // 앞에 1추가
    sFractions[7] = 0;

#endif

    //------------------------------------------
    // check zero, infinity, NaN (Not a Number)
    //------------------------------------------

    if ( sExponent2 == 0x00 )
    {
        // zero
        aValue->length = 1;
        aValue->signExponent = 0x80;
        
        IDE_CONT( normal_exit );
    }
    else
    {
        // Nothing to do.
    }

    if ( sExponent2 == 0x7ff )
    {
        if ( sLongValue == 0x10000000000000 )
        {
            // -infinity, +infinity
            IDE_RAISE( ERR_VALUE_OVERFLOW );
        }
        else
        {
            // NaN
            IDE_RAISE( ERR_INVALID_LITERAL );
        }
    }
    else
    {
        // Nothing to do.
    }
    
    //------------------------------------------
    // binary to decimal
    //------------------------------------------

    // double_value
    // = sign * (1*2^52 + (f0)*2^51 + (f1)*2^50 + ... + (f51)*2^0) * 2^(e-1023-52)
    // = sign * (long_value) * 2^(e-1023-52)
    // = sign * (long_value * x) * 10^y
    sLongValue = (SLong)(sLongValue * gConvMantissa[sExponent2 - 1023 - 52]);
    sExponent10 = gConvExponent[sExponent2 - 1023 - 52];

    //------------------------------------------
    // make numeric
    //------------------------------------------

    // 홀수 지수 보정
    if ( sExponent10 % 2 != 0 )
    {
        sLongValue *= 10;
        sExponent10--;
    }
    else
    {
        // Nothing to do.
    }
        
    for ( i = 9; i >= 0; i-- )
    {
        sMantissa[i] = (UChar)(sLongValue % 100);
        sLongValue /= 100;
    }

    for ( sExponent100 = 0; sExponent100 < 10; sExponent100++ )
    {
        if ( sMantissa[sExponent100] != 0 )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }
        
    for ( i = 0; i + sExponent100 < 10; i++ )
    {
        sMantissa[i] = sMantissa[i + sExponent100];
    }
    for ( ; i < 10; i++)
    {
        sMantissa[i] = 0;
    }
        
    sLength = 1;
    for ( i = 0; i < 10; i++ )
    {
        if ( sMantissa[i] != 0 )
        {
            sLength = i + 2;
        }
        else
        {
            // Nothing to do.
        }
    }

    sExponent = 10 - sExponent100 + (sExponent10 / 2);
    
    IDE_TEST_RAISE( sExponent > 63, ERR_VALUE_OVERFLOW );

    if ( sExponent < -63 )
    {
        // zero
        aValue->length = 1;
        aValue->signExponent = 0x80;
    }
    else
    {
        if ( sSign == 0 )
        {
            aValue->length = sLength;
            aValue->signExponent = 128 + 64 + sExponent;
        
            for ( i = 0; i < sLength - 1; i++ )
            {
                aValue->mantissa[i] = sMantissa[i];
            }
        }
        else
        {
            aValue->length = sLength;
            aValue->signExponent = 64 - sExponent;

            // 음수 보수 변환
            for ( i = 0; i < sLength - 1; i++ )
            {
                aValue->mantissa[i] = 99 - sMantissa[i];
            }
        }
    }

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * PROJ-1517
 * rounds up aArgument1 and store it on aValue
 */
IDE_RC mtc::roundFloat( mtdNumericType *aValue,
                        mtdNumericType *aArgument1,
                        mtdNumericType *aArgument2 )
{
    SShort sExponent;
    SLong  sRound;
    UShort sCarry;
    SShort sArgExponent1;
    UChar  sMantissa[MTD_FLOAT_MANTISSA_MAXIMUM] = { 0, };
    SInt   i;

    /* null 이면 null */
    if ( aArgument1->length == 0 )
    {
        aValue->length = 0;
    }
    else if ( aArgument1->length == 1 )
    {
        aValue->length = 1;
        aValue->mantissa[0]= 0;
        aValue->signExponent = 0x80;
    }
    else
    {
        IDE_TEST( numeric2Slong( &sRound, aArgument2 ) != IDE_SUCCESS );

        sRound = -sRound;

        /* exponent 구하기 */
        sArgExponent1 = aArgument1->signExponent & 0x80;
        if ( sArgExponent1 == 0x80 )
        {
            sExponent = (aArgument1->signExponent & 0x7F) - 64;
        }
        else
        {
            sExponent = 64 - (aArgument1->signExponent & 0x7F);
        }

        /* aArgument1 복사 */
        aValue->signExponent = aArgument1->signExponent;
        aValue->length = aArgument1->length;

        if ( sArgExponent1 == 0x80 )
        {
            for ( i = 0; i < aArgument1->length - 1; i++ )
            {
                sMantissa[i] = aArgument1->mantissa[i];
                aValue->mantissa[i] = aArgument1->mantissa[i];
            }
        }
        else
        {
            for ( i = 0; i < aArgument1->length - 1; i++ )
            {
                sMantissa[i] = 99 - aArgument1->mantissa[i];
                aValue->mantissa[i] = aArgument1->mantissa[i];
            }
        }

        if ( sExponent * 2 <= sRound )
        {
            if ( ( sExponent * 2 == sRound ) && ( sMantissa[0] >= 50 ) )
            {
                sExponent++;
                IDE_TEST_RAISE( sExponent > 63, ERR_OVERFLOW );
                aValue->length = 2;
                /* exponent 설정 */
                if ( sArgExponent1 == 0x80 )
                {
                    aValue->signExponent = sExponent + 192;
                    aValue->mantissa[0] = 1;
                }
                else
                {
                    aValue->signExponent = 64 - sExponent;
                    aValue->mantissa[0] = 98;
                }
            }
            else
            {
                //zero
                aValue->length = 1;
                aValue->mantissa[0]= 0;
                aValue->signExponent = 0x80;
            }
        }
        else
        {
#if defined(HP_HPUX) || defined(IA64_HP_HPUX)
            switch ( (SInt)( sRound % 2 ) )
#else
            switch ( sRound % 2 )
#endif
            {
                case 1:
                    if ( ( sExponent - ( sRound / 2 ) - 1 )
                        < (SShort)ID_SIZEOF(sMantissa) )
                    {
                        if ( sMantissa[sExponent - ( sRound / 2 ) - 1] % 10 >= 5 )
                        {
                            sMantissa[sExponent - ( sRound / 2 ) - 1]
                                     -= sMantissa[sExponent - ( sRound / 2 ) - 1] % 10;
                            sMantissa[sExponent - ( sRound / 2 ) - 1] += 10;
                        }
                        else
                        {
                            sMantissa[sExponent - ( sRound / 2 ) - 1]
                                     -= sMantissa[sExponent - ( sRound / 2 ) - 1] % 10;
                        }

                        for ( i = sExponent - ( sRound / 2 );
                              i < (SInt)ID_SIZEOF(sMantissa); i++ )
                        {
                            sMantissa[i] = 0;
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    break;

                case -1:
                    if ( ( sExponent - ( sRound / 2 ) ) < (SShort)ID_SIZEOF(sMantissa) )
                    {
                        if ( sMantissa[sExponent - ( sRound / 2 )] % 10 >= 5 )
                        {
                            sMantissa[sExponent - ( sRound / 2 )]
                                     -= sMantissa[sExponent - ( sRound / 2 ) ] % 10;
                            sMantissa[sExponent - ( sRound / 2 )] += 10;
                        }
                        else
                        {
                            sMantissa[sExponent - ( sRound / 2 )]
                                     -= sMantissa[sExponent - ( sRound / 2 )] % 10;
                        }

                        for ( i = sExponent - ( sRound / 2 ) + 1;
                              i < (SInt)ID_SIZEOF(sMantissa); i++ )
                        {
                            sMantissa[i] = 0;
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    break;

                default:
                    if ( ( sExponent - ( sRound / 2 ) ) < (SShort)ID_SIZEOF(sMantissa) )
                    {
                        if ( sMantissa[sExponent - ( sRound / 2 )] >= 50 )
                        {
                            sMantissa[sExponent - ( sRound / 2 ) - 1]++;
                        }
                        else
                        {
                            /* Nothing to do */
                        }

                        for ( i = sExponent - ( sRound / 2 ); i < (SInt)ID_SIZEOF(sMantissa); i++ )
                        {
                            sMantissa[i] = 0;
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    break;
            }/* switch */

            sCarry = 0;
            for ( i = ID_SIZEOF(sMantissa) - 1; i >= 0; i-- )
            {
                sMantissa[i] += sCarry;
                sCarry = 0;
                if ( sMantissa[i] >= 100 )
                {
                    sMantissa[i] -= 100;
                    sCarry = 1;
                }
                else
                {
                    /* Nothing to do */
                }
            }

            if ( sCarry != 0 )
            {
                for ( i = ID_SIZEOF(sMantissa) - 1; i > 0; i-- )
                {
                    sMantissa[i] = sMantissa[i - 1];
                }
                sMantissa[0] = 1;
                sExponent++;
                IDE_TEST_RAISE( sExponent > 63, ERR_OVERFLOW );
                /* exponent 설정 */
                if ( sArgExponent1 == 0x80 )
                {
                    aValue->signExponent = sExponent + 192;
                }
                else
                {
                    aValue->signExponent = 64 - sExponent;
                }
            }
            else
            {
                /* Nothing to do */
            }

            /* length 설정 */
            for ( i = 0, aValue->length = 1; i < (SInt)ID_SIZEOF(sMantissa) - 1; i++ )
            {
                if ( sMantissa[i] != 0 )
                {
                    aValue->length = i + 2;
                }
                else
                {
                    /* Nothing to do */
                }
            }

            if ( sArgExponent1 == 0x80 )
            {
                for ( i = 0; i < aValue->length - 1; i++ )
                {
                    aValue->mantissa[i] = sMantissa[i];
                }
            }
            else
            {
                for ( i = 0; i < aValue->length - 1; i++ )
                {
                    aValue->mantissa[i] = 99 - sMantissa[i];
                }
            }

            if ( sMantissa[0] == 0 )
            {
                //zero
                aValue->length = 1;
                aValue->mantissa[0]= 0;
                aValue->signExponent = 0x80;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }/* else */

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_OVERFLOW );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * PROJ-1517
 * gets the integral part of aArgument1 and store it on aValue
 */
IDE_RC mtc::truncFloat( mtdNumericType *aValue,
                        mtdNumericType *aArgument1,
                        mtdNumericType *aArgument2 )
{
    SShort sExponent;
    SLong  sTrunc;
    SShort sArgExponent1;
    UChar  sMantissa[MTD_FLOAT_MANTISSA_MAXIMUM] = { 0, };
    SInt   i;

    /* null 이면 null */
    if ( aArgument1->length == 0 )
    {
        aValue->length = 0;
    }
    else if ( aArgument1->length == 1 )
    {
        aValue->length = 1;
        aValue->mantissa[0]= 0;
        aValue->signExponent = 0x80;
    }
    else
    {
        IDE_TEST( numeric2Slong( &sTrunc, aArgument2 ) != IDE_SUCCESS );

        sTrunc = -sTrunc;

        /* exponent 구하기 */
        sArgExponent1 = aArgument1->signExponent & 0x80;
        if ( sArgExponent1 == 0x80 )
        {
            sExponent = (aArgument1->signExponent & 0x7F) - 64;
        }
        else
        {
            sExponent = 64 - (aArgument1->signExponent & 0x7F);
        }

        /* aArgument1 복사 */
        aValue->signExponent = aArgument1->signExponent;
        aValue->length = aArgument1->length;

        if ( sArgExponent1 == 0x80 )
        {
            for ( i = 0; i < aArgument1->length - 1; i++ )
            {
                sMantissa[i] = aArgument1->mantissa[i];
                aValue->mantissa[i] = aArgument1->mantissa[i];
            }
        }
        else
        {
            for ( i = 0; i < aArgument1->length - 1; i++ )
            {
                sMantissa[i] = 99 - aArgument1->mantissa[i];
                aValue->mantissa[i] = aArgument1->mantissa[i];
            }
        }

        if ( ( sExponent * 2 ) <= sTrunc )
        {
            /* zero */
            aValue->length = 1;
            aValue->mantissa[0]= 0;
            aValue->signExponent = 0x80;
        }
        else
        {
#if defined(HP_HPUX) || defined(IA64_HP_HPUX)
            switch ( (SInt)(sTrunc % 2) )
#else
            switch ( sTrunc % 2 )
#endif
            {
                case 1:
                    if ( ( sExponent - ( sTrunc / 2 ) - 1 ) < (SShort)ID_SIZEOF(sMantissa) )
                    {

                        sMantissa[sExponent - ( sTrunc / 2 ) - 1]
                                  -= sMantissa[sExponent - ( sTrunc / 2 ) - 1] % 10;

                        for ( i = sExponent - ( sTrunc / 2 ); i < (SInt)ID_SIZEOF(sMantissa); i++ )
                        {
                            sMantissa[i] = 0;
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    break;

                case -1:
                    if ( ( sExponent - ( sTrunc / 2 ) ) < (SShort)ID_SIZEOF(sMantissa) )
                    {
                        sMantissa[sExponent - ( sTrunc / 2 )]
                                  -= sMantissa[sExponent - ( sTrunc / 2 )] % 10;

                        for ( i = sExponent - ( sTrunc / 2 ) + 1; i < (SInt)ID_SIZEOF(sMantissa); i++ )
                        {
                            sMantissa[i] = 0;
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    break;

                default:
                    if ( ( sExponent - ( sTrunc / 2 ) ) < (SShort)ID_SIZEOF(sMantissa) )
                    {
                        for ( i = sExponent - ( sTrunc / 2 ); i < (SInt)ID_SIZEOF(sMantissa); i++ )
                        {
                            sMantissa[i] = 0;
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    break;
            }/* switch */

            //length 설정
            for ( i = 0, aValue->length = 1; i < (SInt)ID_SIZEOF(sMantissa) - 1; i++ )
            {
                if ( sMantissa[i] != 0 )
                {
                    aValue->length = i + 2;
                }
                else
                {
                    /* Nothing to do */
                }
            }

            //복사
            if ( sArgExponent1 == 0x80 )
            {
                for ( i = 0; i < aValue->length - 1; i++ )
                {
                    aValue->mantissa[i] = sMantissa[i];
                }
            }
            else
            {
                for ( i = 0; i < aValue->length - 1; i++ )
                {
                    aValue->mantissa[i] = 99 - sMantissa[i];
                }
            }

            if ( sMantissa[0] == 0 )
            {
                /* zero */
                aValue->length = 1;
                aValue->mantissa[0]= 0;
                aValue->signExponent = 0x80;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }/* else */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::getPrecisionScaleFloat( const mtdNumericType * aValue,
                                    SInt                 * aPrecision,
                                    SInt                 * aScale )
{
// To fix BUG-12944
// precision과 scale을 정확히 구해주는 함수.
// 만약 maximum precision을 넘는 경우는 최대 39, 40이므로,
// precision, scale을 강제로 38로 넘겨준다.(최대 mantissa배열 크기를 넘지 않으므로 무관함)
// 무조건 성공함. return IDE_SUCCESS;
    SInt sExponent;

    if ( aValue->length > 1 )
    {
        sExponent  = (aValue->signExponent&0x7F) - 64;
        if ( (aValue->signExponent & 0x80) == 0x00 )
        {
            sExponent *= -1;
        }
        else
        {
            /* Nothing to do */
        }

        *aPrecision = (aValue->length-1)*2;
        *aScale = *aPrecision - (sExponent << 1);

        if ( (aValue->signExponent & 0x80) == 0x00 )
        {
            if ( (99 - *(aValue->mantissa)) / 10 == 0 )
            {
                (*aPrecision)--;
            }
            else
            {
                // Nothing to do.
            }

            if ( (99 - *(aValue->mantissa + aValue->length - 2)) % 10 == 0 )
            {
                (*aPrecision)--;
                (*aScale)--;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if( *(aValue->mantissa) / 10 == 0 )
            {
                (*aPrecision)--;
            }
            else
            {
                // Nothing to do.
            }

            if ( *(aValue->mantissa + aValue->length - 2) % 10 == 0 )
            {
                (*aPrecision)--;
                (*aScale)--;
            }
            else
            {
                // Nothing to do.
            }
        }


        if ( (*aPrecision) > MTD_NUMERIC_PRECISION_MAXIMUM )
        {
            // precision이 감소하면서 scale도 마찬가지로 감소해야 함.
            (*aScale)  -= (*aPrecision) - MTD_NUMERIC_PRECISION_MAXIMUM;
            (*aPrecision) = MTD_NUMERIC_PRECISION_MAXIMUM;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        *aPrecision = 1;
        *aScale = 0;
    }

    return IDE_SUCCESS;
}

IDE_RC mtc::isSamePhysicalImage( const mtcColumn * aColumn,
                                 const void      * aRow1,
                                 UInt              aFlag1,
                                 const void      * aRow2,
                                 UInt              aFlag2,
                                 idBool          * aOutIsSame )
{
/***********************************************************************
 *
 * Description :
 *
 *    BUG-16531
 *    Column이 포함한 Data가 동일한 Image인지 검사
 *
 * Implementation :
 *
 *    NULL인 경우 Garbage Data가 존재할 수 있으므로 모두 NULL인지 판단
 *    Data의 길이가 동일한지 검사
 *    Data 길이가 동일할 경우 memcmp 로 검사
 *
 ***********************************************************************/

    UInt              sLength1;
    UInt              sLength2;

    const void *      sValue1;
    const void *      sValue2;

    const mtdModule * sTypeModule;

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ASSERT( aColumn != NULL );
    IDE_ASSERT( aRow1 != NULL );
    IDE_ASSERT( aRow2 != NULL );
    IDE_ASSERT( aOutIsSame != NULL );

    //------------------------------------------
    // Initialize
    //------------------------------------------

    *aOutIsSame = ID_FALSE;

    /* PROJ-2118 */
    /* Data Type 모듈 획득 */
    IDE_TEST( mtd::moduleById( &sTypeModule,
                               aColumn->type.dataTypeId )
              != IDE_SUCCESS );

    //------------------------------------------
    // Image Comparison
    //------------------------------------------

    sValue1 = mtd::valueForModule( (smiColumn*) aColumn,
                                   aRow1,
                                   aFlag1,
                                   aColumn->module->staticNull );
    sValue2 = mtd::valueForModule( (smiColumn*) aColumn,
                                   aRow2,
                                   aFlag2,
                                   aColumn->module->staticNull );

    // check both data is NULL
    if ( ( sTypeModule->isNull( aColumn, sValue1 ) == ID_TRUE ) &&
         ( sTypeModule->isNull( aColumn, sValue2 ) == ID_TRUE ) )
    {
        // both are NULL value
        *aOutIsSame = ID_TRUE;
    }
    else
    {
        // Data Length
        sLength1 = sTypeModule->actualSize( aColumn, sValue1 );
        sLength2 = sTypeModule->actualSize( aColumn, sValue2 );

        if ( sLength1 == sLength2 )
        {
            if ( idlOS::memcmp( sValue1,
                                sValue2,
                                sLength1 ) == 0 )
            {
                // same image
                *aOutIsSame = ID_TRUE;
            }
            else
            {
                // different image
            }
        }
        else
        {
            // different length
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::initializeColumn( mtcColumn       * aColumn,
                              const mtdModule * aModule,
                              UInt              aArguments,
                              SInt              aPrecision,
                              SInt              aScale )
{
/***********************************************************************
 *
 * Description : mtcColumn의 초기화
 *              ( mtdModule 및 mtlModule을 지정해준 경우 )
 *
 * Implementation :
 *
 ***********************************************************************/
    const mtdModule * sModule;

    // smiColumn 정보 설정
    aColumn->column.flag = SMI_COLUMN_TYPE_FIXED;

    if ( aModule->id == MTD_NUMBER_ID )
    {
        if( aArguments == 0 )
        {
            IDE_TEST( mtd::moduleById( & sModule, MTD_FLOAT_ID )
                      != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST( mtd::moduleById( & sModule, MTD_NUMERIC_ID )
                      != IDE_SUCCESS);
        }
    }
    else
    {
        sModule = aModule;
    }

    // data type module 정보 설정
    aColumn->type.dataTypeId = sModule->id;
    aColumn->module = sModule;

    aColumn->flag = aArguments;
    aColumn->precision = aPrecision;
    aColumn->scale = aScale;

    // 추가정보 초기화
    idlOS::memset( &(aColumn->mColumnAttr),
                   0x00,
                   ID_SIZEOF( mtcColumnAttr ) );

    // data type module의 semantic 검사
    IDE_TEST( sModule->estimate( & aColumn->column.size,
                                 & aColumn->flag,
                                 & aColumn->precision,
                                 & aColumn->scale )
              != IDE_SUCCESS );

    if( (mtl::mDBCharSet != NULL) && (mtl::mNationalCharSet != NULL) )
    {
        // create database의 경우 validation 과정에서
        // mtl::mDBCharSet, mtl::mNationalCharSet을 세팅하게 된다.

        // Nothing to do
        if( (sModule->id == MTD_NCHAR_ID) ||
            (sModule->id == MTD_NVARCHAR_ID) )
        {
            aColumn->type.languageId = mtl::mNationalCharSet->id;
            aColumn->language = mtl::mNationalCharSet;
        }
        else
        {
            aColumn->type.languageId = mtl::mDBCharSet->id;
            aColumn->language = mtl::mDBCharSet;
        }
    }
    else
    {
        // create database가 아닌 경우
        // 임시로 세팅 후, META 단계에서 재설정한다.
        // fixed table, PV는 ASCII로 설정해도 상관없다.
        // (ASCII이외의 값이 없기 때문)
        mtl::mDBCharSet = & mtlAscii;
        mtl::mNationalCharSet = & mtlUTF16;

        aColumn->type.languageId = mtl::mDBCharSet->id;
        aColumn->language = mtl::mDBCharSet;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::initializeColumn( mtcColumn       * aColumn,
                              UInt              aDataTypeId,
                              UInt              aArguments,
                              SInt              aPrecision,
                              SInt              aScale )
{
/***********************************************************************
 *
 * Description : mtcColumn의 초기화
 *               ( mtdModule 및 mtlModule을 찾아야 하는 경우 )
 *
 * Implementation :
 *
 ***********************************************************************/
    const mtdModule * sModule;
    const mtdModule * sRealModule;

    // smiColumn 정보 설정
    aColumn->column.flag = SMI_COLUMN_TYPE_FIXED;

    // 해당 mtdModule 찾아서 mtcColumn::module에 설정
    IDE_TEST( mtd::moduleById( & sModule, aDataTypeId ) != IDE_SUCCESS);

    if ( sModule->id == MTD_NUMBER_ID )
    {
        if( aArguments == 0 )
        {
            IDE_TEST( mtd::moduleById( & sRealModule, MTD_FLOAT_ID )
                      != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST( mtd::moduleById( & sRealModule, MTD_NUMERIC_ID )
                      != IDE_SUCCESS);
        }
    }
    else
    {
        sRealModule = sModule;
    }

    aColumn->type.dataTypeId = sRealModule->id;
    aColumn->module = sRealModule;

    // flag, precision, scale 정보 설정
    aColumn->flag = aArguments;
    aColumn->precision = aPrecision;
    aColumn->scale = aScale;

    // 추가정보 초기화
    idlOS::memset( &(aColumn->mColumnAttr),
                   0x00,
                   ID_SIZEOF( mtcColumnAttr ) );

    // data type module의 semantic 검사
    IDE_TEST( sRealModule->estimate( & aColumn->column.size,
                                     & aColumn->flag,
                                     & aColumn->precision,
                                     & aColumn->scale )
              != IDE_SUCCESS );

    if( (mtl::mDBCharSet != NULL) && (mtl::mNationalCharSet != NULL) )
    {
        // create database의 경우 validation 과정에서
        // mtl::mDBCharSet, mtl::mNationalCharSet을 세팅하게 된다.

        // Nothing to do
        if( (sModule->id == MTD_NCHAR_ID) ||
            (sModule->id == MTD_NVARCHAR_ID) )
        {
            aColumn->type.languageId = mtl::mNationalCharSet->id;
            aColumn->language = mtl::mNationalCharSet;
        }
        else
        {
            aColumn->type.languageId = mtl::mDBCharSet->id;
            aColumn->language = mtl::mDBCharSet;
        }
    }
    else
    {
        // create database가 아닌 경우
        // 임시로 세팅 후, META 단계에서 재설정한다.
        // fixed table, PV는 ASCII로 설정해도 상관없다.
        // (ASCII이외의 값이 없기 때문)
        mtl::mDBCharSet = & mtlAscii;
        mtl::mNationalCharSet = & mtlUTF16;

        aColumn->type.languageId = mtl::mDBCharSet->id;
        aColumn->language = mtl::mDBCharSet;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void mtc::initializeColumn( mtcColumn  * aDestColumn,
                            mtcColumn  * aSrcColumn )
{
/***********************************************************************
 *
 * Description : mtcColumn의 초기화
 *               ( src column으로 dest column을 초기화하는 경우)
 *
 * Implementation :
 *
 ***********************************************************************/
    // type, module 설정
    aDestColumn->type.dataTypeId = aSrcColumn->type.dataTypeId;
    aDestColumn->module          = aSrcColumn->module;

    // language, module 설정
    aDestColumn->type.languageId = aSrcColumn->type.languageId;
    aDestColumn->language        = aSrcColumn->language;

    // smiColumn size, flag 정보 설정
    aDestColumn->column.size = aSrcColumn->column.size;
    aDestColumn->column.flag = aSrcColumn->column.flag;

    aDestColumn->column.flag &= ~SMI_COLUMN_TYPE_MASK;
    aDestColumn->column.flag |= SMI_COLUMN_TYPE_FIXED;

    // BUG-38785
    aDestColumn->column.flag &= ~SMI_COLUMN_COMPRESSION_MASK;
    aDestColumn->column.flag |= SMI_COLUMN_COMPRESSION_FALSE;

    // mtcColumn flag, precision, scale 정보 설정
    aDestColumn->flag      = aSrcColumn->flag;
    // BUG-36836
    // not null 속성은 복사하지 않는다.
    aDestColumn->flag      &= ~MTC_COLUMN_NOTNULL_MASK;
    aDestColumn->precision = aSrcColumn->precision;
    aDestColumn->scale     = aSrcColumn->scale;

    // echar, evarchar
    if (( aDestColumn->module->flag & MTD_ENCRYPT_TYPE_MASK)
                 == MTD_ENCRYPT_TYPE_TRUE )
    {
        // 보안 정보 설정
        aDestColumn->mColumnAttr.mEncAttr.mEncPrecision = aSrcColumn->mColumnAttr.mEncAttr.mEncPrecision;
    }
    else
    {
        // 보안 정보 설정
        aDestColumn->mColumnAttr.mEncAttr.mEncPrecision = 0;
    }
    
    idlOS::memcpy( (void*) aDestColumn->mColumnAttr.mEncAttr.mPolicy,
                   (void*) aSrcColumn->mColumnAttr.mEncAttr.mPolicy,
                   MTC_POLICY_NAME_SIZE + 1 );
}

IDE_RC mtc::initializeEncryptColumn( mtcColumn    * aColumn,
                                     const SChar  * aPolicy,
                                     UInt           aEncryptedSize,
                                     UInt           aECCSize )
{
/***********************************************************************
 * PROJ-2002 Column Security
 *
 * Description : 컬럼의 암호화를 위한 mtcColumn의 초기화
 *              ( mtdModule 및 mtlModule을 지정해준 경우 )
 *
 * Implementation :
 *
 ***********************************************************************/
    IDE_ASSERT( aColumn->module != NULL );

    // echar, evarchar만 가능함
    IDE_ASSERT( (aColumn->module->flag & MTD_ENCRYPT_TYPE_MASK)
                 == MTD_ENCRYPT_TYPE_TRUE );

    // 보안 정보 설정
    aColumn->mColumnAttr.mEncAttr.mEncPrecision = (SInt) ( aEncryptedSize + aECCSize );
    idlOS::snprintf( aColumn->mColumnAttr.mEncAttr.mPolicy,
                     MTC_POLICY_NAME_SIZE + 1,
                     "%s",
                     aPolicy );

    // data type module의 semantic 검사
    IDE_TEST( aColumn->module->estimate( & aColumn->column.size,
                                         & aColumn->flag,
                                         & aColumn->mColumnAttr.mEncAttr.mEncPrecision,
                                         & aColumn->scale )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::makeSmallint( mtdSmallintType* aSmallint,
                          const UChar*     aString,
                          UInt             aLength )
{
    SLong  sLong;
    UChar *sEndPtr = NULL;

    IDE_TEST_RAISE( aLength > MTC_MAXIMUM_CHAR_SIZE_FOR_NUMBER_GROUP,
                    ERR_INVALID_LITERAL );

    // BUG-29170
    errno = 0;
    sLong = idlOS::strtol( (const char*)aString, (char**)&sEndPtr, 10 );

    IDE_TEST_RAISE( errno == ERANGE, ERR_VALUE_OVERFLOW );

    if ( sEndPtr != (aString + aLength) )
    {
        sLong = (SLong)idlOS::strtod( (const char*)aString, (char**)&sEndPtr );

        IDE_TEST_RAISE( errno == ERANGE, ERR_VALUE_OVERFLOW );
        IDE_TEST_RAISE( sEndPtr != (aString + aLength), ERR_INVALID_LITERAL );
    }

    /* PATCH(BEGIN): GNU MATH LIBRARY - REMOVE MINUS ZERO */
    if( sLong == 0 )
    {
        sLong = 0;
    }
    /* PATCH(END): GNU MATH LIBRARY - REMOVE MINUS ZERO */

    IDE_TEST_RAISE( sLong < MTD_SMALLINT_MINIMUM,
                    ERR_VALUE_OVERFLOW );
    IDE_TEST_RAISE( sLong > MTD_SMALLINT_MAXIMUM,
                    ERR_VALUE_OVERFLOW );

    *aSmallint = (mtdSmallintType)sLong;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::makeInteger( mtdIntegerType* aInteger,
                         const UChar*    aString,
                         UInt            aLength )
{
    SLong  sLong;
    UChar *sEndPtr = NULL;

    IDE_TEST_RAISE( aLength > MTC_MAXIMUM_CHAR_SIZE_FOR_NUMBER_GROUP,
                    ERR_INVALID_LITERAL );

    // BUG-29170
    errno = 0;
    sLong = idlOS::strtol( (const char*)aString, (char**)&sEndPtr, 10 );

    IDE_TEST_RAISE( errno == ERANGE, ERR_VALUE_OVERFLOW );

    if ( sEndPtr != (aString + aLength) )
    {
        sLong = (SLong)idlOS::strtod( (const char*)aString, (char**)&sEndPtr );

        IDE_TEST_RAISE( errno == ERANGE, ERR_VALUE_OVERFLOW );
        IDE_TEST_RAISE( sEndPtr != (aString + aLength), ERR_INVALID_LITERAL );
    }

    /* PATCH(BEGIN): GNU MATH LIBRARY - REMOVE MINUS ZERO */
    if( sLong == 0 )
    {
        sLong = 0;
    }
    /* PATCH(END): GNU MATH LIBRARY - REMOVE MINUS ZERO */

    IDE_TEST_RAISE( sLong < MTD_INTEGER_MINIMUM,
                    ERR_VALUE_OVERFLOW );
    IDE_TEST_RAISE( sLong > MTD_INTEGER_MAXIMUM,
                    ERR_VALUE_OVERFLOW );

    *aInteger = (mtdIntegerType)sLong;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::makeBigint( mtdBigintType* aBigint,
                        const UChar*   aString,
                        UInt           aLength )
{
    SLong           sLong;
    mtdNumericType* sNumeric;
    UChar           sNumericBuffer[MTD_NUMERIC_SIZE_MAXIMUM];

    IDE_TEST_RAISE( aLength > MTC_MAXIMUM_CHAR_SIZE_FOR_NUMBER_GROUP,
                    ERR_INVALID_LITERAL );

    sNumeric = (mtdNumericType*)sNumericBuffer;

    IDE_TEST( makeNumeric( sNumeric,
                           MTD_NUMERIC_MANTISSA_MAXIMUM,
                           aString,
                           aLength ) != IDE_SUCCESS );

    if( sNumeric->length > 1 )
    {
        IDE_TEST( numeric2Slong( &sLong,
                                 sNumeric )
                  != IDE_SUCCESS );
    }
    else
    {
        sLong = 0;
    }

    IDE_TEST_RAISE( sLong < MTD_BIGINT_MINIMUM,
                    ERR_VALUE_OVERFLOW );
    IDE_TEST_RAISE( sLong > MTD_BIGINT_MAXIMUM,
                    ERR_VALUE_OVERFLOW );

    *aBigint = (mtdBigintType)sLong;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::makeReal( mtdRealType* aReal,
                      const UChar* aString,
                      UInt         aLength )
{
    UChar *sEndPtr = NULL;

    IDE_TEST_RAISE( aLength > MTC_MAXIMUM_CHAR_SIZE_FOR_NUMBER_GROUP,
                    ERR_INVALID_LITERAL );

    errno = 0;

    *aReal = idlOS::strtod( (const char*)aString, (char**)&sEndPtr );

    /* PATCH(BEGIN): GNU MATH LIBRARY - REMOVE MINUS ZERO */
    if( *aReal == 0 )
    {
        *aReal = 0;
    }
    /* PATCH(END): GNU MATH LIBRARY - REMOVE MINUS ZERO */

    IDE_TEST_RAISE( sEndPtr != (aString + aLength), ERR_INVALID_LITERAL );

    IDE_TEST_RAISE( (*aReal == +HUGE_VAL) || (*aReal == -HUGE_VAL),
                    ERR_VALUE_OVERFLOW );

    if( ( idlOS::fabs(*aReal) < MTD_REAL_MINIMUM ) &&
        ( *aReal != 0 ) )
    {
        *aReal = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::makeDouble( mtdDoubleType* aDouble,
                        const UChar*   aString,
                        UInt           aLength )
{
    UChar *sEndPtr = NULL;

    IDE_TEST_RAISE( aLength > MTC_MAXIMUM_CHAR_SIZE_FOR_NUMBER_GROUP,
                    ERR_INVALID_LITERAL );

    errno = 0;

    *aDouble = idlOS::strtod( (const char*)aString, (char**)&sEndPtr );

    /* PATCH(BEGIN): GNU MATH LIBRARY - REMOVE MINUS ZERO */
    if( *aDouble == 0 )
    {
        *aDouble = 0;
    }
    /* PATCH(END): GNU MATH LIBRARY - REMOVE MINUS ZERO */

    IDE_TEST_RAISE( sEndPtr != (aString + aLength), ERR_INVALID_LITERAL );

    IDE_TEST_RAISE( (*aDouble == +HUGE_VAL) || (*aDouble == -HUGE_VAL),
                    ERR_VALUE_OVERFLOW );

    if( ( idlOS::fabs(*aDouble) < MTD_DOUBLE_MINIMUM ) &&
        ( *aDouble != 0 ) )
    {
        *aDouble = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::makeInterval( mtdIntervalType* aInterval,
                          const UChar*     aString,
                          UInt             aLength )
{
    SDouble  sDouble;
    UChar   *sEndPtr = NULL;
    SDouble  sIntegerPart;
    SDouble  sFractionalPart;

    IDE_TEST_RAISE( aLength > MTC_MAXIMUM_CHAR_SIZE_FOR_NUMBER_GROUP,
                    ERR_INVALID_LITERAL );

    errno = 0;

    sDouble = idlOS::strtod( (const char*)aString, (char**)&sEndPtr );

    /* PATCH(BEGIN): GNU MATH LIBRARY - REMOVE MINUS ZERO */
    if( sDouble == 0 )
    {
        sDouble = 0;
    }
    /* PATCH(END): GNU MATH LIBRARY - REMOVE MINUS ZERO */

    IDE_TEST_RAISE( sEndPtr != (aString + aLength), ERR_INVALID_LITERAL );

    sDouble *= 864e2;

    sFractionalPart        = modf( sDouble, &sIntegerPart );
    aInterval->second      = (SLong)sIntegerPart;
    aInterval->microsecond = (SLong)( sFractionalPart * 1E6 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1597 Temp record size 제약제거
// bit, varbit, nibble 타입들은
// varchar, char 등의 타입과는 다르게 DISK DB에서도 header를 저장해야 한다.
// 하지만 getMtdHeaderSize()는 이들 타입을 고려하지 않고 header 길이를
// 반환하기 때문에 column 값의 actual size - header size 만큼 공간을
// 확보하면 문제가 된다.
// 이를 해결하기 위해 bit, varbit, nibble 등의 타입을 고려한
// headerSize를 반환하는 함수를 별도로 만들었다.
// 이 함수는 SM과 qdbCommon에서 사용된다.
// 이 함수의 의미는 컬럼이 DISK DB에 저장될 때,
// 저장되지 않는 영역의 길이를 말한다.
// bit 등의 타입은 컬럼 정보 모두가 저장되어야 하기 때문에
// 이 값이 0이다.
IDE_RC mtc::getNonStoringSize( const smiColumn *aColumn, UInt * aOutSize )
{
    const mtcColumn* sColumn;
    const mtdModule* sModule;

    sColumn = (const mtcColumn*)aColumn;

    /* PROJ-2118 */
    IDE_TEST( mtd::moduleById( &sModule,
                               sColumn->type.dataTypeId )
              != IDE_SUCCESS);

    if ( (sModule->flag & MTD_DATA_STORE_MTDVALUE_MASK)
        == MTD_DATA_STORE_MTDVALUE_TRUE )
    {
        *aOutSize = 0;
    }
    else
    {
        *aOutSize = sModule->headerSize();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::makeBinary( mtdBinaryType* aBinary,
                        const UChar*   aString,
                        UInt           aLength )
{
    UInt sBinaryLength = 0;

    IDE_TEST( mtc::makeBinary( aBinary->mValue,
                               aString,
                               aLength,
                               &sBinaryLength,
                               NULL)
              != IDE_SUCCESS );

    aBinary->mLength = sBinaryLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::makeBinary( UChar*       aBinaryValue,
                        const UChar* aString,
                        UInt         aLength,
                        UInt*        aBinaryLength,
                        idBool*      aOddSizeFlag )
{
    const UChar* sToken;
    const UChar* sTokenFence;
    UChar*       sIterator;
    UChar        sBinary;
    UInt         sBinaryLength = *aBinaryLength;

    const UChar* sSrcText = aString;
    UInt         sSrcLen = aLength;

    static const UChar sHexByte[256] = {
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 99, 99, 99, 99, 99, 99,
        99, 10, 11, 12, 13, 14, 15, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 10, 11, 12, 13, 14, 15, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99
    };

    sIterator = aBinaryValue;

    /* bug-31397: large sql_byte params make buffer overflow.
     * If data is divided into several pieces due to the CM block limit,
     * the last odd byte of the 1st piece is combined with
     * the 1st byte of the 2nd piece.
     *  ex) 1st piece: 1 2 3 4 5     2nd piece: 6 7 8
     *  convert the 1st piece: 0x12 0x34 0x50 (set aOddSizeFlag to 1)
     *  convert the 2nd piece: 0x12 0x34 0x56 0x78 */
    if ((aOddSizeFlag != NULL) && (*aOddSizeFlag == ID_TRUE) &&
        (*aBinaryLength > 0) && (sSrcLen > 0))
    {
        /* get the last byte of the target buffer */
        sBinary = *(sIterator - 1);
        IDE_TEST_RAISE( sHexByte[sSrcText[0]] > 15, ERR_INVALID_LITERAL );
        sBinary |= sHexByte[sSrcText[0]];
        *(sIterator - 1) = sBinary;

        sSrcText++;
        sSrcLen--;
        *aOddSizeFlag = ID_FALSE;
    }

    for( sToken = sSrcText, sTokenFence = sToken + sSrcLen;
         sToken < sTokenFence;
         sIterator++, sToken += 2 )
    {
        IDE_TEST_RAISE( sHexByte[sToken[0]] > 15, ERR_INVALID_LITERAL );
        sBinary = sHexByte[sToken[0]] << 4;
        if( sToken + 1 < sTokenFence )
        {
            IDE_TEST_RAISE( sHexByte[sToken[1]] > 15,
                            ERR_INVALID_LITERAL );
            sBinary |= sHexByte[sToken[1]];
        }
        /* bug-31397: large sql_byte params make buffer overflow.
         * If the size is odd, set aOddSizeFlag to 1 */
        else if ((sToken + 1 == sTokenFence) && (aOddSizeFlag != NULL))
        {
            *aOddSizeFlag = ID_TRUE;
        }
        sBinaryLength++;
        *sIterator = sBinary;
    }
    *aBinaryLength = sBinaryLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::makeByte( mtdByteType* aByte,
                      const UChar* aString,
                      UInt         aLength )
{
    UInt sByteLength = 0;

    IDE_TEST( mtc::makeByte( aByte->value,
                             aString,
                             aLength,
                             &sByteLength,
                             NULL)
              != IDE_SUCCESS );

    aByte->length = sByteLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::makeByte( UChar*       aByteValue,
                      const UChar* aString,
                      UInt         aLength,
                      UInt*        aByteLength,
                      idBool*      aOddSizeFlag)
{
    const UChar* sToken;
    const UChar* sTokenFence;
    UChar*       sIterator;
    UChar        sByte;
    UInt         sByteLength = *aByteLength;

    const UChar* sSrcText = aString;
    UInt         sSrcLen = aLength;

    static const UChar sHexByte[256] = {
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 99, 99, 99, 99, 99, 99,
        99, 10, 11, 12, 13, 14, 15, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 10, 11, 12, 13, 14, 15, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99
    };

    sIterator = aByteValue;

    /* bug-31397: large sql_byte params make buffer overflow.
     * If data is divided into several pieces due to the CM block limit,
     * the last odd byte of the 1st piece is combined with
     * the 1st byte of the 2nd piece.
     *  ex) 1st piece: 1 2 3 4 5     2nd piece: 6 7 8
     *  convert the 1st piece: 0x12 0x34 0x50 (set aOddSizeFlag to 1)
     *  convert the 2nd piece: 0x12 0x34 0x56 0x78 */
    if ((aOddSizeFlag != NULL) && (*aOddSizeFlag == ID_TRUE) &&
        (*aByteLength > 0) && (sSrcLen > 0))
    {
        /* get the last byte of the target buffer */
        sByte = *(sIterator - 1);
        IDE_TEST_RAISE( sHexByte[sSrcText[0]] > 15, ERR_INVALID_LITERAL );
        sByte |= sHexByte[sSrcText[0]];
        *(sIterator - 1) = sByte;

        sSrcText++;
        sSrcLen--;
        *aOddSizeFlag = ID_FALSE;
    }

    for( sToken = sSrcText, sTokenFence = sToken + sSrcLen;
         sToken < sTokenFence;
         sIterator++, sToken += 2 )
    {
        IDE_TEST_RAISE( sHexByte[sToken[0]] > 15, ERR_INVALID_LITERAL );
        sByte = sHexByte[sToken[0]] << 4;
        if( sToken + 1 < sTokenFence )
        {
            IDE_TEST_RAISE( sHexByte[sToken[1]] > 15,
                            ERR_INVALID_LITERAL );
            sByte |= sHexByte[sToken[1]];
        }
        /* bug-31397: large sql_byte params make buffer overflow.
         * If the size is odd, set aOddSizeFlag to 1 */
        else if ((sToken + 1 == sTokenFence) && (aOddSizeFlag != NULL))
        {
            *aOddSizeFlag = ID_TRUE;
        }
        sByteLength++;
        IDE_TEST_RAISE( sByteLength > MTD_BYTE_PRECISION_MAXIMUM,
                        ERR_INVALID_LITERAL );
        *sIterator = sByte;
    }
    *aByteLength = sByteLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::makeNibble( mtdNibbleType* aNibble,
                        const UChar*   aString,
                        UInt           aLength )
{
    UInt sNibbleLength = 0;

    IDE_TEST( mtc::makeNibble( aNibble->value,
                               0,
                               aString,
                               aLength,
                               &sNibbleLength )
              != IDE_SUCCESS );

    aNibble->length = sNibbleLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::makeNibble( UChar*       aNibbleValue,
                        UChar        aOffsetInByte,
                        const UChar* aString,
                        UInt         aLength,
                        UInt*        aNibbleLength )
{
    const UChar* sToken;
    const UChar* sTokenFence;
    UChar*       sIterator;
    UChar        sNibble;
    UInt         sNibbleLength = *aNibbleLength;

    static const UChar sHexNibble[256] = {
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 99, 99, 99, 99, 99, 99,
        99, 10, 11, 12, 13, 14, 15, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 10, 11, 12, 13, 14, 15, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99
    };

    sIterator = aNibbleValue;

    sToken = (const UChar*)aString;
    sTokenFence = sToken + aLength;

    if( (aOffsetInByte == 1) && (sToken < sTokenFence) )
    {
        IDE_TEST_RAISE( sHexNibble[sToken[0]] > 15, ERR_INVALID_LITERAL );
        sNibble = (*sIterator) | sHexNibble[sToken[0]];
        sNibbleLength++;
        IDE_TEST_RAISE( sNibbleLength > MTD_NIBBLE_PRECISION_MAXIMUM,
                        ERR_INVALID_LITERAL );

        *sIterator = sNibble;
        sIterator++;
        sToken += 1;
    }

    for( ;sToken < sTokenFence; sIterator++, sToken += 2 )
    {
        IDE_TEST_RAISE( sHexNibble[sToken[0]] > 15, ERR_INVALID_LITERAL );
        sNibble = sHexNibble[sToken[0]] << 4;
        sNibbleLength++;
        IDE_TEST_RAISE( sNibbleLength > MTD_NIBBLE_PRECISION_MAXIMUM,
                        ERR_INVALID_LITERAL );

        if( sToken + 1 < sTokenFence )
        {
            IDE_TEST_RAISE( sHexNibble[sToken[1]] > 15, ERR_INVALID_LITERAL );
            sNibble |= sHexNibble[sToken[1]];
            sNibbleLength++;
            IDE_TEST_RAISE( sNibbleLength > MTD_NIBBLE_PRECISION_MAXIMUM,
                            ERR_INVALID_LITERAL );
        }

        *sIterator = sNibble;
    }

    *aNibbleLength = sNibbleLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::makeBit( mtdBitType*  aBit,
                     const UChar* aString,
                     UInt         aLength )
{
    UInt sBitLength = 0;

    IDE_TEST( mtc::makeBit( aBit->value,
                            0,
                            aString,
                            aLength,
                            &sBitLength )
              != IDE_SUCCESS );

    aBit->length = sBitLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::makeBit( UChar*       aBitValue,
                     UChar        aOffsetInBit,
                     const UChar* aString,
                     UInt         aLength,
                     UInt*        aBitLength )
{
    const UChar* sToken;
    const UChar* sTokenFence;
    UChar*       sIterator;
    UChar        sBit;
    UInt         sSubIndex;
    UInt         sBitLength = *aBitLength;
    UInt         sTokenOffset = 0;

    static const UChar sHexBit[256] = {
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
         0,  1, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99
    };

    sIterator = aBitValue;
    sToken = (const UChar*)aString;
    sTokenFence = sToken + aLength;

    if( aOffsetInBit > 0 )
    {
        sBit = *sIterator;

        IDE_TEST_RAISE( sHexBit[sToken[0]] > 1, ERR_INVALID_LITERAL );

        sSubIndex    = aOffsetInBit;

        while( (sToken + sTokenOffset < sTokenFence) && (sSubIndex < 8) )
        {
            IDE_TEST_RAISE( sHexBit[sToken[sTokenOffset]] > 1, ERR_INVALID_LITERAL );
            sBit |= ( sHexBit[sToken[sTokenOffset]] << ( 7 - sSubIndex ) );
            sBitLength++;
            IDE_TEST_RAISE( sBitLength > MTD_BIT_PRECISION_MAXIMUM,
                            ERR_INVALID_LITERAL );
            sTokenOffset++;
            sSubIndex++;
        }

        sToken += sTokenOffset;
        *sIterator = sBit;

        sIterator++;
    }

    for( ; sToken < sTokenFence; sIterator++, sToken += 8 )
    {
        IDE_TEST_RAISE( sHexBit[sToken[0]] > 1, ERR_INVALID_LITERAL );

        // 값 넣기 전에 0으로 초기화
        idlOS::memset( sIterator,
                       0x00,
                       1 );

        sBit = sHexBit[sToken[0]] << 7;
        sBitLength++;
        IDE_TEST_RAISE( sBitLength > MTD_BIT_PRECISION_MAXIMUM,
                        ERR_INVALID_LITERAL );

        sSubIndex = 1;
        while( sToken + sSubIndex < sTokenFence && sSubIndex < 8 )
        {
            IDE_TEST_RAISE( sHexBit[sToken[sSubIndex]] > 1, ERR_INVALID_LITERAL );
            sBit |= ( sHexBit[sToken[sSubIndex]] << ( 7 - sSubIndex ) );
            sBitLength++;
            IDE_TEST_RAISE( sBitLength > MTD_BIT_PRECISION_MAXIMUM,
                            ERR_INVALID_LITERAL );
            sSubIndex++;
        }

        *sIterator = sBit;
    }

    *aBitLength = sBitLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool mtc::compareOneChar( UChar * aValue1,
                            UChar   aValue1Size,
                            UChar * aValue2,
                            UChar   aValue2Size )
{
    idBool sIsSame;

    if( aValue1Size != aValue2Size )
    {
        sIsSame = ID_FALSE;
    }
    else
    {
        sIsSame = ( idlOS::memcmp( aValue1, aValue2,
                                   aValue1Size ) == 0
                    ? ID_TRUE : ID_FALSE );
    }

    return sIsSame;
}

IDE_RC mtc::getLikeEcharKeySize( mtcTemplate * aTemplate,
                                 UInt        * aECCSize,
                                 UInt        * aKeySize )
{
    UInt  sECCSize;

    IDE_TEST( aTemplate->getECCSize( MTC_LIKE_KEY_PRECISION,
                                     & sECCSize )
              != IDE_SUCCESS );

    *aKeySize = idlOS::align( ID_SIZEOF(UShort)
                              + ID_SIZEOF(UShort)
                              + MTC_LIKE_KEY_PRECISION + sECCSize,
                              8 );

    if ( aECCSize != NULL )
    {
        *aECCSize = sECCSize;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::getLobLengthLocator( smLobLocator   aLobLocator,
                                 idBool       * aIsNull,
                                 UInt         * aLobLength,
                                 idvSQL       * aStatistics )
{
    SLong  sLobLength;
    idBool sIsNulLob;

    // BUG-41525
    if ( aLobLocator == MTD_LOCATOR_NULL )
    {
        *aIsNull    = ID_TRUE;
        *aLobLength = 0;
    }
    else
    {
        IDE_TEST( mtc::getLobLengthWithLocator( aStatistics,
                                                aLobLocator,
                                                & sLobLength,
                                                & sIsNulLob )
                  != IDE_SUCCESS );
    
        if ( sIsNulLob == ID_TRUE )
        {
            *aIsNull    = ID_TRUE;
            *aLobLength = 0;
        }
        else
        {
            IDE_TEST_RAISE( (sLobLength < 0) || (sLobLength > ID_UINT_MAX),
                            ERR_LOB_SIZE );
    
            *aIsNull    = ID_FALSE;
            *aLobLength = (UInt)sLobLength;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LOB_SIZE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_STORED_DATA_LENGTH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtc::isNullLobRow( const void      * aRow,
                          const smiColumn * aColumn,
                          idBool          * aIsNull )
{
    if ( mtc::isNullLobColumnWithRow(aRow, aColumn) == ID_TRUE )
    {
        *aIsNull = ID_TRUE;
    }
    else
    {
        *aIsNull = ID_FALSE;
    }

    return IDE_SUCCESS;
}

/* PROJ-2209 DBTIMEZONE */
SLong mtc::getSystemTimezoneSecond( void )
{
    PDL_Time_Value sTimevalue;
    time_t         sTime;
    struct tm      sLocaltime;
    struct tm      sGlobaltime;
    SLong          sLocalSecond;
    SLong          sGlobalSecond;
    SLong          sHour;
    SLong          sMin;
    SInt           sDayOffset;

    sTimevalue = idlOS::gettimeofday();
    sTime = (time_t)sTimevalue.sec();

    idlOS::localtime_r( &sTime, &sLocaltime );
    idlOS::gmtime_r( &sTime, &sGlobaltime );

    sHour = (SLong)sLocaltime.tm_hour;
    sMin  = (SLong)sLocaltime.tm_min;
    sLocalSecond  = ( sHour * 60 * 60 ) + ( sMin * 60 );

    sHour = (SLong)sGlobaltime.tm_hour;
    sMin  = (SLong)sGlobaltime.tm_min;
    sGlobalSecond = ( sHour * 60 * 60 ) + ( sMin * 60 );

    if ( sLocaltime.tm_mday != sGlobaltime.tm_mday )
    {
        sDayOffset = sLocaltime.tm_mday - sGlobaltime.tm_mday;
        if ( ( sDayOffset == -1 ) || ( sDayOffset > 2 ) )
        {
            /* -1 day */
            sLocalSecond -= ( 24 * 60 * 60 );
        }
        else
        {
            /* +1 day */
            sLocalSecond += ( 24 * 60 * 60 );
        }
    }
    else
    {
        /* nothing to do. */
    }

    return ( sLocalSecond - sGlobalSecond );
}

SChar * mtc::getSystemTimezoneString( SChar * aTimezoneString )
{
    SLong sTimezoneSecond;
    SLong sAbsTimezoneSecond;

    sTimezoneSecond = getSystemTimezoneSecond();

    if ( sTimezoneSecond < 0 )
    {
        sAbsTimezoneSecond = sTimezoneSecond * -1;
        idlOS::snprintf( aTimezoneString,
                         MTC_TIMEZONE_VALUE_LEN + 1,
                         "-%02"ID_INT64_FMT":%02"ID_INT64_FMT,
                         sAbsTimezoneSecond / 3600, ( sAbsTimezoneSecond % 3600 ) / 60 );
    }
    else
    {
        idlOS::snprintf( aTimezoneString,
                         MTC_TIMEZONE_VALUE_LEN + 1,
                         "+%02"ID_INT64_FMT":%02"ID_INT64_FMT,
                         sTimezoneSecond / 3600, ( sTimezoneSecond % 3600 ) / 60 );
    }

    return aTimezoneString;
}

/* PROJ-1517 */
void mtc::makeFloatConversionModule( mtcStack         *aStack,
                                     const mtdModule **aModule )
{
    if ( aStack->column->module->id == MTD_NUMERIC_ID )
    {
        *aModule = &mtdNumeric;
    }
    else
    {
        *aModule = &mtdFloat;
    }
}

// BUG-37484
idBool mtc::needMinMaxStatistics( const smiColumn* aColumn )
{
    const mtcColumn * sColumn;
    idBool            sRc;

    sColumn = (const mtcColumn*)aColumn;

    if( sColumn->type.dataTypeId == MTD_GEOMETRY_ID )
    {
        sRc = ID_FALSE;
    }
    else
    {
        sRc = ID_TRUE;
    }

    return sRc;
}

/* PROJ-1071 Parallel query */
IDE_RC mtc::cloneTemplate4Parallel( iduMemory    * aMemory,
                                    mtcTemplate  * aSource,
                                    mtcTemplate  * aDestination )
{
    UShort sTupleIndex;

    aDestination->stackCount            = aSource->stackCount;
    aDestination->dataSize              = aSource->dataSize;
    aDestination->execInfoCnt           = aSource->execInfoCnt;
    aDestination->initSubquery          = aSource->initSubquery;
    aDestination->fetchSubquery         = aSource->fetchSubquery;
    aDestination->finiSubquery          = aSource->finiSubquery;
    aDestination->setCalcSubquery       = aSource->setCalcSubquery;
    aDestination->getOpenedCursor       = aSource->getOpenedCursor;
    aDestination->addOpenedLobCursor    = aSource->addOpenedLobCursor;
    aDestination->isBaseTable           = aSource->isBaseTable;
    aDestination->closeLobLocator       = aSource->closeLobLocator;
    aDestination->getSTObjBufSize       = aSource->getSTObjBufSize;
    aDestination->rowCount              = aSource->rowCount;
    aDestination->rowCountBeforeBinding = aSource->rowCountBeforeBinding;
    aDestination->variableRow           = aSource->variableRow;
    aDestination->encrypt               = aSource->encrypt;
    aDestination->decrypt               = aSource->decrypt;
    aDestination->encodeECC             = aSource->encodeECC;
    aDestination->getDecryptInfo        = aSource->getDecryptInfo;
    aDestination->getECCInfo            = aSource->getECCInfo;
    aDestination->getECCSize            = aSource->getECCSize;
    aDestination->dateFormat            = aSource->dateFormat;
    aDestination->dateFormatRef         = aSource->dateFormatRef;
    aDestination->nlsCurrency           = aSource->nlsCurrency;
    aDestination->nlsCurrencyRef        = aSource->nlsCurrencyRef;
    aDestination->groupConcatPrecisionRef = aSource->groupConcatPrecisionRef;
    aDestination->arithmeticOpMode      = aSource->arithmeticOpMode;
    aDestination->arithmeticOpModeRef   = aSource->arithmeticOpModeRef;
    aDestination->funcDataCnt           = aSource->funcDataCnt;

    idlOS::memcpy(aDestination->currentRow,
                  aSource->currentRow,
                  MTC_TUPLE_TYPE_MAXIMUM * ID_SIZEOF(UShort) );

    // stack buffer is set on allocation

    // data 는 나중에 복사한다.

    // execInfo 는 나중에 복사한다.

    for ( sTupleIndex = 0;
          sTupleIndex < aDestination->rowCount;
          sTupleIndex++ )
    {
        IDE_TEST( cloneTuple4Parallel( aMemory,
                                       aSource,
                                       sTupleIndex,
                                       aDestination,
                                       sTupleIndex )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-1071 Parallel query */
IDE_RC mtc::cloneTuple4Parallel( iduMemory   * /*aMemory*/,
                                 mtcTemplate * aSrcTemplate,
                                 UShort        aSrcTupleID,
                                 mtcTemplate * aDstTemplate,
                                 UShort        aDstTupleID )
{
    ULong sFlag;

    sFlag = aSrcTemplate->rows[aSrcTupleID].lflag;

    aDstTemplate->rows[aDstTupleID].modify
        = aSrcTemplate->rows[aSrcTupleID].modify;
    aDstTemplate->rows[aDstTupleID].lflag
        = aSrcTemplate->rows[aSrcTupleID].lflag;
    aDstTemplate->rows[aDstTupleID].columnCount
        = aSrcTemplate->rows[aSrcTupleID].columnCount;
    aDstTemplate->rows[aDstTupleID].columnMaximum
        = aSrcTemplate->rows[aSrcTupleID].columnMaximum;
    aDstTemplate->rows[aDstTupleID].rowOffset
        = aSrcTemplate->rows[aSrcTupleID].rowOffset;
    aDstTemplate->rows[aDstTupleID].rowMaximum
        = aSrcTemplate->rows[aSrcTupleID].rowMaximum;

    // columns 와 execute 는 원본 템플릿의 것을 공유해서 사용한다.
    aDstTemplate->rows[aDstTupleID].columns
        = aSrcTemplate->rows[aSrcTupleID].columns;
    aDstTemplate->rows[aDstTupleID].execute
        = aSrcTemplate->rows[aSrcTupleID].execute;

    // constant, variable, table tuple 은 row 를 복사해야 한다.
    // Intermediate tuple 을 제외한 나머지는 ROW ALLOC 이 FALSE 이다.
    if ( (sFlag & MTC_TUPLE_ROW_ALLOCATE_MASK)
         == MTC_TUPLE_ROW_ALLOCATE_FALSE )
    {
        // 보통 row 의 복사는 MTC_TUPLE_ROW_SET_MASK 에 의해 결정된다.
        // 이 mask 는 prepare 종료 시점을 기준으로 설정된 mask 이다.
        // 하지만 parallel 을 위한 template clone 은
        // execution 도중에 일어나기 때문에
        // 현재 실행 중인 template 을 그대로 사용하기 위해 row 를 새로 할당하는
        // intermediate tuple 을 제외한 나머지 tuple 들은 row 를 복사해야 한다.
        aDstTemplate->rows[aDstTupleID].row
            = aSrcTemplate->rows[aSrcTupleID].row;
    }
    else
    {
        /* Nothing to do. */
    }

    aDstTemplate->rows[aDstTupleID].partitionTupleID
        = aSrcTemplate->rows[aSrcTupleID].partitionTupleID;
    aDstTemplate->rows[aDstTupleID].cursorInfo
        = aSrcTemplate->rows[aSrcTupleID].cursorInfo;
    aDstTemplate->rows[aDstTupleID].ridExecute =
        aSrcTemplate->rows[aSrcTupleID].ridExecute;

    return IDE_SUCCESS;
}

/* PROJ-2399 */
IDE_RC mtc::getColumnStoreLen( const smiColumn * aColumn,
                               UInt            * aActualColLen )
{
    mtcColumn       * sColumn;
    const mtdModule * sModule;

    sColumn = (mtcColumn*)aColumn;

    if ( (sColumn->flag & MTC_COLUMN_NOTNULL_MASK ) == MTC_COLUMN_NOTNULL_FALSE )
    {
        /* not null이 아닌 column은 variable column으로 간주 한다. */
        *aActualColLen = ID_UINT_MAX;
    }
    else
    {
        if ( (sColumn->column.flag & SMI_COLUMN_COMPRESSION_MASK) 
             != SMI_COLUMN_COMPRESSION_TRUE )
        {
            /* sm에 실제로 저장된 column length를 구한다.
             * variable column인 경우 ID_UINT_MAX가 반환된다. */
            IDE_TEST( mtd::moduleById( &sModule, sColumn->type.dataTypeId )
                      != IDE_SUCCESS );
 
            *aActualColLen = sModule->storeSize( aColumn );
        }
        else
        {
            // PROJ-2429 Dictionary based data compress for on-disk DB
            // Dictionary column의 크기는 ID_SIZEOF(smOID)이다.
            *aActualColLen = ID_SIZEOF(smOID);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    ISO 8601 표준 주차를 구합니다.
 *      주차는 월요일부터 시작합니다.
 *      1월 1일이 목요일 이전이면, 첫 주와 전년도의 마지막 주는 당년도의 1주차입니다.
 *      1월 1일이 금요일 이후이면, 첫 주는 전년도의 마지막 주차입니다.
 *
 * Implementation :
 *    1. 연도를 구합니다.
 *      1월 1일 : 1월 1일이 금토일인 경우, 전년도 12월 31일의 주차를 구합니다. (Step 2)
 *      1월 2일 : 1월 1일이 금토인 경우, 전년도 12월 31일의 주차를 구합니다. (Step 2)
 *      1월 3일 : 1월 1일이 금인 경우, 전년도 12월 31일의 주차를 구합니다. (Step 2)
 *      12월 29일 : 12월 31일이 수인 경우, 차년도 1주차입니다.
 *      12월 30일 : 12월 31일이 화수인 경우, 차년도 1주차입니다.
 *      12월 31일 : 12월 31일이 월화수인 경우, 차년도 1주차입니다.
 *      나머지 : 당년도 주차를 구합니다. (Step 2)
 *
 *    2. 주차를 구합니다.
 *      (1) 첫 주가 주차에 포함되는지 확인합니다.
 *          1월 1일이 월화수목인 경우, 첫 주가 주차에 포함됩니다.
 *          첫 주의 마지막일자(일요일)를 구합니다. (firstSunday)
 *
 *      (2) 나머지 주의 주차를 구합니다.
 *          ( dayOfYear - firstSunday + 6 ) / 7
 *
 *      (3) 1과 2를 더해서 주차를 구합니다.
 *
 ***********************************************************************/
SInt mtc::weekOfYearForStandard( SInt aYear,
                                 SInt aMonth,
                                 SInt aDay )
{
    SInt    sWeekOfYear  = 0;
    SInt    sDayOfWeek   = 0;
    SInt    sDayOfYear   = 0;
    SInt    sFirstSunday = 0;
    idBool  sIsPrevYear  = ID_FALSE;

    /* Step 1. 연도를 구합니다. */
    if ( aMonth == 1 )
    {
        switch ( aDay )
        {
            case 1 :
                sDayOfWeek = dayOfWeek( aYear, 1, 1 );
                if ( ( sDayOfWeek >= 5 ) || ( sDayOfWeek == 0 ) )   // 금토일
                {
                    sIsPrevYear = ID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
                break;

            case 2 :
                sDayOfWeek = dayOfWeek( aYear, 1, 1 );
                if ( sDayOfWeek >= 5 )                              // 금토
                {
                    sIsPrevYear = ID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
                break;

            case 3 :
                sDayOfWeek = dayOfWeek( aYear, 1, 1 );
                if ( sDayOfWeek == 5 )                              // 금
                {
                    sIsPrevYear = ID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
                break;

            default :
                break;
        }
    }
    else if ( aMonth == 12 )
    {
        switch ( aDay )
        {
            case 29 :
                sDayOfWeek = dayOfWeek( aYear, 12, 31 );
                if ( sDayOfWeek == 3 )                                                      // 수
                {
                    sWeekOfYear = 1;
                }
                else
                {
                    /* Nothing to do */
                }
                break;

            case 30 :
                sDayOfWeek = dayOfWeek( aYear, 12, 31 );
                if ( ( sDayOfWeek == 2 ) || ( sDayOfWeek == 3 ) )                           // 화수
                {
                    sWeekOfYear = 1;
                }
                else
                {
                    /* Nothing to do */
                }
                break;

            case 31 :
                sDayOfWeek = dayOfWeek( aYear, 12, 31 );
                if ( ( sDayOfWeek == 1 ) || ( sDayOfWeek == 2 ) || ( sDayOfWeek == 3 ) )    // 월화수
                {
                    sWeekOfYear = 1;
                }
                else
                {
                    /* Nothing to do */
                }
                break;

            default :
                break;
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* Step 2. 주차를 구합니다. */
    if ( sWeekOfYear == 0 )
    {
        if ( sIsPrevYear == ID_TRUE )
        {
            sDayOfWeek = dayOfWeek( aYear - 1, 1, 1 );
            sDayOfYear = dayOfYear( aYear - 1, 12, 31 );
        }
        else
        {
            sDayOfWeek = dayOfWeek( aYear, 1, 1 );
            sDayOfYear = dayOfYear( aYear, aMonth, aDay );
        }

        sFirstSunday = 8 - sDayOfWeek;
        if ( sFirstSunday == 8 )    // 1월 1일이 일요일
        {
            sFirstSunday = 1;
        }
        else
        {
            /* Nothing to do */
        }

        if ( ( sDayOfWeek >= 1 ) && ( sDayOfWeek <= 4 ) )   // 첫 주가 월화수목
        {
            sWeekOfYear = ( sDayOfYear - sFirstSunday + 6 ) / 7 + 1;
        }
        else
        {
            sWeekOfYear = ( sDayOfYear - sFirstSunday + 6 ) / 7;
        }
    }
    else
    {
        /* Nothing to do */
    }

    return sWeekOfYear;
}

/***********************************************************************
 *
 * Description :
 *    ISO 8601 표준 연도를 구합니다.
 *      주차는 월요일부터 시작합니다.
 *      1월 1일이 목요일 이전이면, 첫 주와 전년도의 마지막 주는 당년도의 1주차입니다.
 *      1월 1일이 금요일 이후이면, 첫 주는 전년도의 마지막 주차입니다.
 *
 * Implementation :
 *    1. 연도를 구합니다.
 *      1월 1일   : 1월 1일이 금토일인 경우, 전년도입니다.
 *      1월 2일   : 1월 1일이 금토인 경우, 전년도입니다.
 *      1월 3일   : 1월 1일이 금인 경우, 전년도입니다.
 *      12월 29일 : 12월 31일이 수인 경우, 차년도입니다.
 *      12월 30일 : 12월 31일이 화수인 경우, 차년도입니다.
 *      12월 31일 : 12월 31일이 월화수인 경우, 차년도입니다.
 *      나머지    : 당년도입니다.
 *
 ***********************************************************************/
SInt mtc::yearForStandard( SInt aYear,
                           SInt aMonth,
                           SInt aDay )
{
    SInt    sYear  = 0;
    SInt    sDayOfWeek   = 0;

    /* Step 1. 연도를 구합니다. */
    sYear = aYear;

    if ( aMonth == 1 )
    {
        switch ( aDay )
        {
            case 1 :
                sDayOfWeek = dayOfWeek( aYear, 1, 1 );
                if ( ( sDayOfWeek >= 5 ) || ( sDayOfWeek == 0 ) )   // 금토일
                {
                    sYear = aYear - 1;
                }
                else
                {
                    /* Nothing to do */
                }
                break;

            case 2 :
                sDayOfWeek = dayOfWeek( aYear, 1, 1 );
                if ( sDayOfWeek >= 5 )                              // 금토
                {
                    sYear = aYear - 1;
                }
                else
                {
                    /* Nothing to do */
                }
                break;

            case 3 :
                sDayOfWeek = dayOfWeek( aYear, 1, 1 );
                if ( sDayOfWeek == 5 )                              // 금
                {
                    sYear = aYear - 1;
                }
                else
                {
                    /* Nothing to do */
                }
                break;

            default :
                break;
        }
    }
    else if ( aMonth == 12 )
    {
        switch ( aDay )
        {
            case 29 :
                sDayOfWeek = dayOfWeek( aYear, 12, 31 );
                if ( sDayOfWeek == 3 )                                                      // 수
                {
                    sYear = aYear + 1;
                }
                else
                {
                    /* Nothing to do */
                }
                break;

            case 30 :
                sDayOfWeek = dayOfWeek( aYear, 12, 31 );
                if ( ( sDayOfWeek == 2 ) || ( sDayOfWeek == 3 ) )                           // 화수
                {
                    sYear = aYear + 1;
                }
                else
                {
                    /* Nothing to do */
                }
                break;

            case 31 :
                sDayOfWeek = dayOfWeek( aYear, 12, 31 );
                if ( ( sDayOfWeek == 1 ) || ( sDayOfWeek == 2 ) || ( sDayOfWeek == 3 ) )    // 월화수
                {
                    sYear = aYear + 1;
                }
                else
                {
                    /* Nothing to do */
                }
                break;

            default :
                break;
        }
    }
    else
    {
        /* Nothing to do */
    }

    return sYear;
}

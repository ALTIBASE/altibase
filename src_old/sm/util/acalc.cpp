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
 * $Id: acalc.cpp 37115 2010-12-04 03:06:08Z elcarim $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <smu.h>
#include <sdp.h>
#include <sdn.h>
#include <sdptb.h>
#include <mtdTypes.h>

SChar     gInputValue[256] = {0};   //사용자로부터 입력받은 값
SChar     gInputType[256] = "DECI"; //입력 값의 데이터 타입(Default는 10진수)
ULong     gConvertedInputValue;     //데이터 타입에 따라 변환된 값

SChar     gConversion[256] = {0};   //수행할 변환(Default는 모두 Display)

/**************************************************
 * output 
 * 데이터 변환에 관한 함수들입니다.
 * CHECK_AND_CONVERT 매크로를 통해 사용자로부터 입력
 * 받은 인자와 이름을 비교하여, 맞으면 변환을 수행합니다.
 **************************************************/
void convertOutputValue( );

void convertRID();
void convertPID();
void convertCharacter();
void convertNumeralSystem();
void convertMtdValue();



/**************************************************
 * input
 * 입력된 값이 10진수(Decimal)인지 16진수(Hexa deci-
 * mal)인지 판단하여 값을 읽습니다.
 **************************************************/
void  convertInputValue( );

ULong getByDeci( );
ULong getByHexa( );


void usage( );

#define CHECK_AND_CONVERT( aFuncName )                                  \
    if( ( gConversion[0] == '\0' ) ||                                   \
        ( idlOS::strcasecmp( #aFuncName, gConversion ) == 0 ) )         \
    {                                                                   \
        idlOS::printf("\t[%s]\n", #aFuncName );                         \
        convert ## aFuncName( );                                        \
    }                                                                   \
    else                                                                \
    {                                                                   \
        /* nothing to do... */                                          \
    }

#define CHECK_AND_GET_BY( aFuncName )                                   \
    if( ( idlOS::strcasecmp( #aFuncName, gInputType ) == 0 ) )          \
    {                                                                   \
        gConvertedInputValue = getBy ## aFuncName( );                   \
        idlOS::printf("[%s] %"ID_UINT64_FMT"\n",                        \
                      #aFuncName, gConvertedInputValue  );              \
    }                                                                   \
    else                                                                \
    {                                                                   \
        /* nothing to do... */                                          \
    }

//--------------------------------------------------
// Output value
//--------------------------------------------------

// 입력된 값을 RID로 여기고 PID, Offset 등으로 변환하여 출력합니다.
void convertRID( )
{
    idlOS::printf("\t\t%-16s %"ID_UINT32_FMT"\n", 
                  "DISK PID", 
                  SD_MAKE_PID( gConvertedInputValue ) );
    idlOS::printf("\t\t%-16s %"ID_UINT32_FMT"\n", 
                  "DISK Offset", 
                  SD_MAKE_OFFSET( gConvertedInputValue ) );
    idlOS::printf( "\n" );
    idlOS::printf("\t\t%-16s %"ID_UINT32_FMT"\n", 
                  "MEM  PID", 
                  SM_MAKE_PID( gConvertedInputValue ) );
    idlOS::printf("\t\t%-16s %"ID_UINT32_FMT"\n", 
                  "MEM  Offset", 
                  SM_MAKE_OFFSET( gConvertedInputValue ) );
}

// 입력된 값을 PID로 여기고 FID, FPID, RID, OID등으로 변환하여 출력합니다.
void convertPID( )
{
    idlOS::printf("\t\t%-16s %"ID_UINT64_FMT"\n", 
                  "DISK FID", 
                  SD_MAKE_FID( gConvertedInputValue ) );
    idlOS::printf("\t\t%-16s %"ID_UINT64_FMT"\n", 
                  "DISK FPID", 
                  SD_MAKE_FPID( gConvertedInputValue ) );
    idlOS::printf("\t\t%-16s %"ID_UINT64_FMT"\n", 
                  "DISK RID", 
                  SD_MAKE_RID( gConvertedInputValue, 0 ) );
    idlOS::printf( "\n" );
    idlOS::printf("\t\t%-16s %"ID_UINT64_FMT"\n", 
                  "MEM  OID", 
                  SM_MAKE_OID( gConvertedInputValue, 0 ) );
}

// Character로 변환하여 출력합니다. %s대신 %c로 하나하나 출력하여
// 중간중간 '\0'이 있어도 뒤쪽까지 출력하도록 합니다.
void convertCharacter( )
{
    SChar * sSrcPtr;
    UInt    i;

    sSrcPtr = (SChar*) &gConvertedInputValue;

    idlOS::printf("\t\t%-16s ", "Character" );
    for( i = 0 ; i< ID_SIZEOF( ULong ) ; i++ )
    {
        idlOS::printf("%c", sSrcPtr[i] );
    }
    idlOS::printf( "\n" );
}

// 기수법에 따라, 10진수 또는 16진수로 출력합니다.
void convertNumeralSystem ( )
{
    idlOS::printf("\t\t%-16s %"ID_UINT64_FMT"\n", "Decimal", gConvertedInputValue );
    idlOS::printf("\t\t%-16s %016"ID_XINT64_FMT"\n", "Hexa Decimal", gConvertedInputValue );
}

// mtdValue형태로 변환하여 출력합니다.
void convertMtdValue ( )
{
    mtdCharType    *sChar;
    mtdNumericType *sNumeric;
    mtdDateType    *sDate;
    ULong           sTempBuf[2] = {0,};

    idlOS::memcpy( sTempBuf, (void*)&gConvertedInputValue, ID_SIZEOF(ULong) );
    sTempBuf[1] = '\0';

    sChar    = (mtdCharType*)    &sTempBuf;
    sNumeric = (mtdNumericType*) &sTempBuf;
    sDate    = (mtdDateType*)    &sTempBuf;
    idlOS::printf("\t\t%-16s len:0x%"ID_UINT32_FMT" val:[%s]\n", 
                  "mtdChar", 
                  sChar->length,
                  sChar->value );
    idlOS::printf("\t\t%-16s len:0x%"ID_UINT32_FMT" signexp:0x%"ID_UINT32_FMT" mantissa:%s\n", 
                  "mtdNumeric", 
                  sNumeric->length,
                  sNumeric->signExponent,
                  sNumeric->mantissa );

    idlOS::printf("\t\t%-16s year:%"ID_INT32_FMT" mon_day_hour:%"ID_UINT32_FMT" min_sec_mic:%"ID_UINT32_FMT"\n",
                  "mtdDateType", 
                  sDate->year,
                  sDate->mon_day_hour,
                  sDate->min_sec_mic
                   );

    idlOS::printf("\t\t%-16s "
                  "year:%-4"ID_INT32_FMT" "
                  "month:%-4"ID_UINT32_FMT" "
                  "day:%-4"ID_UINT32_FMT" "
                  "hour:%-4"ID_UINT32_FMT" "
                  "minute:%-4"ID_UINT32_FMT" "
                  "second:%-4"ID_UINT32_FMT" "
                  "microSecond:%-4"ID_UINT32_FMT" \n",
                  " ",
                  mtdDateInterface::year( sDate ),
                  mtdDateInterface::month( sDate ),
                  mtdDateInterface::day( sDate ),
                  mtdDateInterface::hour( sDate ),
                  mtdDateInterface::minute( sDate ),
                  mtdDateInterface::second( sDate ),
                  mtdDateInterface::microSecond( sDate ) );
}

void convertOutputValue( )
{
    CHECK_AND_CONVERT( RID );
    CHECK_AND_CONVERT( PID );
    CHECK_AND_CONVERT( Character );
    CHECK_AND_CONVERT( NumeralSystem );
    CHECK_AND_CONVERT( MtdValue );
}



//--------------------------------------------------
// Input value
//--------------------------------------------------

// String을 10진수로 읽습니다. 
ULong getByDeci()
{
    ULong   sRet = 0;
    char  * sSrc;

    sSrc = gInputValue;

    while((*sSrc) != '\0')
    {
        sRet = sRet * 10;
        if( ( (*sSrc) < '0'   )|| 
            ( '9' < (*sSrc) ) )
        {
            break;
        }
        else
        {
            // nothing to do...
        }

        sRet += (ULong)( (*sSrc) - '0');
        sSrc ++;
    }

    return sRet;
}

UInt htoi( UChar *aSrc )
{
    UInt sRet = 0;

    if( ( '0' <= (*aSrc) ) && ( (*aSrc) <= '9' ) )
    {
        sRet = ( (*aSrc) - '0' );
    }
    else if( ( 'a' <= (*aSrc) ) &&  ( (*aSrc) <= 'f' ) )
    {
        sRet = ( (*aSrc) - 'a' ) + 10;
    }
    else if( ( 'A' <= (*aSrc) ) &&  ( (*aSrc) <= 'F' ) )
    {
        sRet = ( (*aSrc) - 'A' ) + 10;
    }

    return sRet;
}

// 문자열을 16진수로 읽습니다.
ULong getByHexa()
{
    ULong   sRet = 0;
    UChar   sTempValue;
    UChar  *sSrc;
    UChar  *sDest;

    sSrc  = (UChar*) gInputValue;
    sDest = (UChar*) &sRet;

    while((*sSrc) != '\0')
    {
        sTempValue = htoi( sSrc );
        sSrc++;

        if( (*sSrc) != '\0' )
        {
            sTempValue = sTempValue*16 + htoi( sSrc );
            sSrc++;
        }
        *sDest = sTempValue;
        sDest ++;
    }

    return sRet;
}

void convertInputValue( )
{
    CHECK_AND_GET_BY( Deci );
    CHECK_AND_GET_BY( Hexa );
}

void usage()
{
    idlOS::printf( "\n"
"Usage: acalc       [ {-g LGID} {-w which_LG} {-e pages_per_ext} ]\n"
"                   [ {-v input_value} [-i input_type] [-c conversion_type] ]\n\n" );
    idlOS::printf(" %-8s : %s\n", "-gwe", "convert LGID to PID" );
    idlOS::printf("       %-8s : %s\n", "-g", "specify LGID" );
    idlOS::printf("       %-8s : %s\n", "-w", "which LG" );
    idlOS::printf("       %-8s : %s\n", "-e", "specify pages per ext" );
    idlOS::printf("\n");
    idlOS::printf(" %-8s : %s\n", "-vic", "convert input value to specified type" );
    idlOS::printf("       %-8s : %s\n", "-v", "specify input value" );
    idlOS::printf("       %-8s : %s\n", "-i", "Numeral system of input value" );
    idlOS::printf("               %-8s : %s\n", "DECI", "decimal integer (DEFAULT)" );
    idlOS::printf("               %-8s : %s\n", "HEXA", "hexadecimal integer " );
    idlOS::printf("       %-8s : %s\n", "-c", "conversion type" );
    idlOS::printf("               %-8s : %s\n", "<none> ", "display all (DEFAULT)" );
    idlOS::printf("               %-8s : %s\n", "RID",     "convert RID to PID and Offset" );
    idlOS::printf("               %-8s : %s\n", "PID",     "convert PID to FPID, FID, RID" );
    idlOS::printf("               %-8s : %s\n", "CHAR",    "convert Value to null-terminated character" );
    idlOS::printf("               %-8s : %s\n", "NUMERALSYSTEM", "convert value to deci, hexa" );
}

int main( int argc, char *argv[] )
{
    SInt      sOpr;
    UInt      sLGIDArgCnt = 0;
    UInt      sTotalArgCnt = 0;

    SInt      sLGID        = 0; // LGID
    SInt      sPagesPerExt = 0; // Extent당 Page수(기본 32)
    SInt      sWhichLG     = 0; // 앞의 LG냐 ,뒤의 LG냐.

    sOpr = idlOS::getopt( argc, argv, "g:w:e:v:i:c:" );
    
    IDE_TEST_RAISE( sOpr == EOF, err_invalid_use );

    do
    {
        switch( sOpr )
        {
            case 'g':
                sLGIDArgCnt++;
                sLGID = idlOS::atoi( optarg );
                break;
            case 'w':
                sLGIDArgCnt++;
                sWhichLG = idlOS::atoi( optarg );
                break;
            case 'e':
                sLGIDArgCnt++;
                sPagesPerExt = idlOS::atoi( optarg );
                break;
            case 'v':
                idlOS::strncpy( gInputValue, optarg, 256 );
                break;
            case 'i':
                idlOS::strncpy( gInputType, optarg, 256 );
                break;
            case 'c':
                idlOS::strncpy( gConversion, optarg, 256 );
                break;
            default:
                IDE_RAISE( err_invalid_use );
                break;
        }
        sTotalArgCnt++;
    }                                     
    while( ( sOpr = idlOS::getopt( argc, argv, "g:w:e:v:i:c:" ) ) != EOF );

    IDE_TEST_RAISE( sTotalArgCnt == 0, err_invalid_use );

    if( sLGIDArgCnt != 0 ) //LGID 관련 인자가 입력되었을 경우
    {
        IDE_TEST_RAISE( sLGIDArgCnt != 3, err_need_more_argument ); //인자가 부족할 경우

        idlOS::printf("%"ID_UINT32_FMT"\n",
            SDPTB_GG_HDR_PAGE_CNT + sLGID *SDPTB_PAGES_PER_LG(sPagesPerExt)
                + sWhichLG );
    }
    else
    {
        //일반적인 변환 과정을 수행
        convertInputValue();
        convertOutputValue();                                           
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_use);
    {
        (void)usage();
    }
    IDE_EXCEPTION(err_need_more_argument);
    {
        idlOS::printf( "-gwe : need more argument\n" );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

